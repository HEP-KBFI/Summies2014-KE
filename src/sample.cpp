#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/progress.hpp>
#include <boost/timer.hpp>

#include <cmath> // std::pow(), std::cosh()
#include <map> // std::map<>
#include <string> // std::string
#include <iostream> // std::cout, std::cerr, std::endl
#include <cstdlib> // std::atoi(), std::atof(), EXIT_SUCCESS
#include <memory> // std::unique_ptr<>

#include <TString.h>
#include <TTree.h>
#include <TFile.h>
#include <TH1F.h>

#include "common.hpp"

/**
 * @todo
 *  - open all histogram files (a map of pointers) (CL flag??)
 *  - a map of histogram pointers
 *  - generate csv out of it
 *  - write the tree
 * @note Assumptions:
 *  - one tree, one file
 *  - flavors, and pt and eta ranges hardcoded
 */

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	using boost::property_tree::ptree; // ptree, read_ini
	
	// command line option parsing
	std::string configFile, cmd_output, cmd_input, cmd_hinput;
	Long64_t beginEvent, endEvent;
	Float_t cmd_workingPoint = -1;
	Int_t cmd_maxSamples = -1;
	bool enableVerbose = false, sampleALot = false;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,i", po::value<std::string>(&cmd_input), "input *.root file\nif not set, read from config file")
			("config,c", po::value<std::string>(&configFile), "read config file")
			("histograms,K", po::value<std::string>(&cmd_hinput), "input histograms (*.root file)")
			("begin,b", po::value<Long64_t>(&beginEvent) -> default_value(0), "the event number to start with")
			("end,e", po::value<Long64_t>(&endEvent) -> default_value(-1), "the event number to end with\ndefault (-1) means all events")
			("output,o", po::value<std::string>(&cmd_output), "output file name")
			("working-point,w", po::value<Float_t>(&cmd_workingPoint), "working point of the CSV value")
			("max-samples,s", po::value<Int_t>(&cmd_maxSamples), "maximum number of samples")
			("multiple-sampling,m", "sample N times")
			("verbose,v", "verbose mode (enables progressbar)")
		;
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		
		if(vm.count("help")) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS); // ugly
		}
		if(vm.count("verbose") != 0) {
			enableVerbose = true;
		}
		if(vm.count("config") == 0 || vm.count("output") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS);
		}
		if(vm.count("multiple-sampling")) {
			sampleALot = true;
		}
	}
	catch(std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		std::exit(EXIT_FAILURE); // ugly
	}
	catch(...) {
		std::cerr << "exception of unkown type" << std::endl;
	}
	
	// sanity check
	if((endEvent >=0 && beginEvent > endEvent) || beginEvent < 0) {
		std::cerr << "incorrect values for begin and/or end" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	// parse config file
	// if the config file doesn't exists, the program throws an error and exits
	if(enableVerbose) std::cout << "Parsing configuration file " << configFile << " ... " << std::endl;
	ptree pt_ini;
	read_ini(configFile, pt_ini);
	auto trim = [] (std::string s) -> std::string {
		s = s.substr(0, s.find(";")); // remove the comment
		boost::algorithm::trim(s); // remove whitespaces around the string
		return s;
	};
	
	const TString treeName = trim(pt_ini.get<std::string>("histogram.tree")).c_str(); // single tree assumed
	std::string config_input = trim(pt_ini.get<std::string>("histogram.in")).c_str(); // single file assumed
	std::string config_hinput = trim(pt_ini.get<std::string>("sample.in")).c_str();
	std::string newTreeName = trim(pt_ini.get<std::string>("sample.tree")).c_str();
	Float_t cfg_workingPoint = std::atof(trim(pt_ini.get<std::string>("sample.wp")).c_str());
	Int_t cfg_maxSamples = std::atoi(trim(pt_ini.get<std::string>("sample.max")).c_str());
	
	// casting
	std::string inputFilename = cmd_input.empty() ? config_input : cmd_input;
	std::string histoInputName = cmd_hinput.empty() ? config_hinput : cmd_hinput;
	Float_t workingPoint = (cmd_workingPoint == -1) ? cfg_workingPoint : cmd_workingPoint;
	Int_t max_samples = (cmd_maxSamples == -1) ? cfg_maxSamples : cmd_maxSamples;
	
	/******************************************************************************************************/
	
	// open the file and tree
	if(enableVerbose) std::cout << "Reading " << inputFilename << " ... " << std::endl;
	std::unique_ptr<TFile> in(TFile::Open(inputFilename.c_str(), "read"));
	if(in -> IsZombie() || ! in -> IsOpen()) {
		std::cerr << "error on opening " << inputFilename << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if(enableVerbose) std::cout << "Accessing TTree " << treeName << " ... " << std::endl;
	TTree * t; // std::unique_ptr can't handle TTree .. 
	t = dynamic_cast<TTree *>(in -> Get(treeName));
	
	// open them histograms
	if(enableVerbose) std::cout << "Reading " << histoInputName << " ... " << std::endl;
	std::unique_ptr<TFile> histograms(TFile::Open(histoInputName.c_str(), "read"));
	if(histograms -> IsZombie() || ! histograms -> IsOpen()) {
		std::cerr << "error on opening " << histoInputName << std::endl;
	}
	std::map<const TString, TH1F *> histoMap;
	if(enableVerbose) std::cout << "Reading all histograms ... " << std::endl;
	for(int i = 0; i < 3; ++i) {
		for(int j = 0; j < 6; ++j) {
			for(int k  = 0; k < 3; ++k) {
				TString tkey = getName(i, j, k).c_str();
				histoMap[tkey] = dynamic_cast<TH1F *> (histograms -> Get(tkey));
			}
		}
	}
	
	// create the output file
	if(enableVerbose) std::cout << "Creating " << cmd_output << " ... " << std::endl;
	std::unique_ptr<TFile> out(new TFile(cmd_output.c_str(), "recreate"));
	TTree * u = new TTree(newTreeName.c_str(), "Tree with generated CSV values according to the histograms."); // output tree
	u -> SetDirectory(out.get());
	
	// set up the variables
	// variables to be used are commented out for obv performance reasons
	if(enableVerbose) std::cout << "Setting up branch addresses ... " << std::endl;
	const int maxNumberOfHJets = 2;
	const int maxNumberOfAJets = 20;
	
	/******************************************************************************************************/
	
	// variables for the old tree
	Int_t nhJets;
	Int_t naJets;
	
	Float_t hJet_pt[maxNumberOfHJets];
	Float_t hJet_eta[maxNumberOfHJets];
	Float_t hJet_csv[maxNumberOfHJets];
	Float_t hJet_flavour[maxNumberOfHJets];
	//Float_t hJet_phi[maxNumberOfHJets];
	//Float_t hJet_e[maxNumberOfHJets];
	//Float_t hJet_genPt[maxNumberOfHJets];
	Float_t aJet_pt[maxNumberOfAJets];
	Float_t aJet_eta[maxNumberOfAJets];
	Float_t aJet_csv[maxNumberOfAJets];
	Float_t aJet_flavour[maxNumberOfAJets];
	//Float_t aJet_phi[maxNumberOfAJets];
	//Float_t aJet_e[maxNumberOfAJets];
	//Float_t aJet_genPt[maxNumberOfAJets];
	
	t -> SetBranchAddress("nhJets", &nhJets);
	t -> SetBranchAddress("naJets", &naJets);
	
	t -> SetBranchAddress("hJet_pt", &hJet_pt);
	t -> SetBranchAddress("hJet_eta", &hJet_eta);
	t -> SetBranchAddress("hJet_csv", &hJet_csv);
	t -> SetBranchAddress("hJet_flavour", &hJet_flavour);
	//t -> SetBranchAddress("hJet_phi", &hJet_phi);
	//t -> SetBranchAddress("hJet_e", &hJet_e);
	//t -> SetBranchAddress("hJet_genPt", &hJet_genPt);
	t -> SetBranchAddress("aJet_pt", &aJet_pt);
	t -> SetBranchAddress("aJet_eta", &aJet_eta);
	t -> SetBranchAddress("aJet_csv", &aJet_csv);
	t -> SetBranchAddress("aJet_flavour", &aJet_flavour);
	//t -> SetBranchAddress("aJet_phi", &aJet_phi);
	//t -> SetBranchAddress("aJet_e", &aJet_e);
	//t -> SetBranchAddress("aJet_genPt", &aJet_genPt);
	
	// variables for the new tree (with prefix 'n_')
	Int_t n_nhJets;
	Int_t n_naJets;
	
	Float_t n_hJet_pt[maxNumberOfHJets];
	Float_t n_hJet_eta[maxNumberOfHJets];
	Float_t n_hJet_csv[maxNumberOfHJets];
	Float_t n_hJet_flavour[maxNumberOfHJets];
	//Float_t n_hJet_phi[maxNumberOfHJets];
	//Float_t n_hJet_e[maxNumberOfHJets];
	//Float_t n_hJet_genPt[maxNumberOfHJets];
	Float_t n_aJet_pt[maxNumberOfAJets];
	Float_t n_aJet_eta[maxNumberOfAJets];
	Float_t n_aJet_csv[maxNumberOfAJets];
	Float_t n_aJet_flavour[maxNumberOfAJets];
	//Float_t n_aJet_phi[maxNumberOfAJets];
	//Float_t n_aJet_e[maxNumberOfAJets];
	//Float_t n_aJet_genPt[maxNumberOfAJets];
	
	Float_t n_aJet_csvGen[maxNumberOfAJets]; // NEW!
	Float_t n_hJet_csvGen[maxNumberOfHJets]; // NEW!
	Long64_t n_aJet_csvN[maxNumberOfAJets]; // NEW!
	Long64_t n_hJet_csvN[maxNumberOfHJets]; // NEW!
	
	u -> Branch("nhJets", &n_nhJets, "nhJets/I");
	u -> Branch("hJet_pt", &n_hJet_pt, "hJet_pt[nhJets]/F");
	u -> Branch("hJet_eta", &n_hJet_eta, "hJet_eta[nhJets]/F");
	u -> Branch("hJet_csv", &n_hJet_csv, "hJet_csv[nhJets]/F");
	u -> Branch("hJet_csvGen", &n_hJet_csvGen, "hJet_csvGen[nhJets]/F");
	u -> Branch("hJet_flavour", &n_hJet_flavour, "hJet_flavour[nhJets]/F");
	//u -> Branch("hJet_phi", &n_hJet_phi, "hJet_phi[nhJets]/F");
	//u -> Branch("hJet_e", &n_hJet_e, "hJet_e[nhJets]/F");
	//u -> Branch("hJet_genPt", &n_hJet_genPt, "hJet_genPt[nhJets]/F");
	
	u -> Branch("naJets", &n_naJets, "naJets/I");
	u -> Branch("aJet_pt", &n_aJet_pt, "aJet_pt[naJets]/F");
	u -> Branch("aJet_eta", &n_aJet_eta, "aJet_eta[naJets]/F");
	u -> Branch("aJet_csv", &n_aJet_csv, "aJet_csv[naJets]/F");
	u -> Branch("aJet_csvGen", &n_aJet_csvGen, "aJet_csvGen[naJets]/F");
	u -> Branch("aJet_flavour", &n_aJet_flavour, "aJet_flavour[naJets]/F");
	//u -> Branch("aJet_phi", &n_aJet_phi, "aJet_phi[naJets]/F");
	//u -> Branch("aJet_e", &n_aJet_e, "aJet_e[naJets]/F");
	//u -> Branch("aJet_genPt", &n_aJet_genPt, "aJet_genPt[naJets]/F");
	
	if(sampleALot) {
		u -> Branch("aJet_csvN", &n_aJet_csvN, "aJet_csvN[naJets]/L");
		u -> Branch("hJet_csvN", &n_hJet_csvN, "hJet_csvN[nhJets]/L");
	}
	
	// if endEvent greater set by the user greater than the number of entries in a tree
	// use the latter value
	endEvent = (endEvent > t -> GetEntries() || endEvent == -1) ? (t -> GetEntries()) : endEvent;
	
	// set up progress bar
	boost::progress_display * show_progress;
	if(enableVerbose) {
		Long64_t dif = endEvent - beginEvent;
		std::cout << "Looping over " << dif << " events ... " << std::endl;
		show_progress = new boost::progress_display(endEvent - beginEvent);
	}
	
	// loop over the events
	for(Long64_t i = beginEvent; i < endEvent; ++i) {
		t -> GetEntry(i);
		
		n_naJets = naJets;
		n_nhJets = nhJets;
		
		// loop over hJets
		for(int j = 0; j < nhJets; ++j) {
			n_hJet_pt[j] = hJet_pt[j];
			n_hJet_eta[j] = hJet_eta[j];
			n_hJet_csv[j] = hJet_csv[j];
			n_hJet_flavour[j] = hJet_flavour[j];
			//n_hJet_e[j] = hJet_e[j];
			//n_hJet_phi[j] = hJet_phi[j];
			//n_hJet_genPt[j] = hJet_genPt[j];
			
			Float_t absEta = TMath::Abs(hJet_eta[j]); // only the absolute value matters
			
			int flavorIndex = getFlavorIndex(hJet_flavour[j]);
			int ptIndex = getPtIndex(hJet_pt[j]);
			int etaIndex = getEtaIndex(absEta);
			
			if(flavorIndex == -1 || ptIndex == -1 || etaIndex == -1) {
				n_hJet_csvGen[j] = -1;
				if(sampleALot) n_hJet_csvN[j] = -1;
			}
			else {
				TString key = getName(flavorIndex, ptIndex, etaIndex).c_str();
				if(sampleALot) {
					Long64_t iterations = 1;
					Double_t randomCSV = -2;
					for(; iterations <= max_samples; ++iterations) {
						randomCSV = histoMap[key] -> GetRandom();
						if(randomCSV >= workingPoint) break;
					}
					if(randomCSV < workingPoint) {
						n_hJet_csvGen[j] = -2;
						n_hJet_csvN[j] = -2;
					}
					else {
						n_hJet_csvGen[j] = randomCSV;
						n_hJet_csvN[j] = iterations;
					}
				}
				else {
					n_hJet_csvGen[j] = histoMap[key] -> GetRandom();
				}
			}
		}
		
		// loop over aJets
		for(int j = 0; j < naJets; ++j) {
			n_aJet_pt[j] = aJet_pt[j];
			n_aJet_eta[j] = aJet_eta[j];
			n_aJet_csv[j] = aJet_csv[j];
			n_aJet_flavour[j] = aJet_flavour[j];
			//n_hJet_e[j] = hJet_e[j];
			//n_hJet_phi[j] = hJet_phi[j];
			//n_hJet_genPt[j] = hJet_genPt[j];
			
			Float_t absEta = TMath::Abs(aJet_eta[j]); // only the absolute value matters
			
			int flavorIndex = getFlavorIndex(aJet_flavour[j]);
			int ptIndex = getPtIndex(aJet_pt[j]);
			int etaIndex = getEtaIndex(absEta);
			
			if(flavorIndex == -1 || ptIndex == -1 || etaIndex == -1) {
				n_aJet_csvGen[j] = -1; // default value if not in the range
				if(sampleALot) n_aJet_csvN[j] = -1; // default value if not in the range
			}
			else {
				TString key = getName(flavorIndex, ptIndex, etaIndex).c_str();
				if(sampleALot) {
					Long64_t iterations = 1;
					Double_t randomCSV = -2;
					for(iterations = 1; iterations <= max_samples; ++iterations) {
						randomCSV = histoMap[key] -> GetRandom();
						if(randomCSV >= workingPoint) break;
					}
					if(randomCSV < workingPoint) {
						n_aJet_csvGen[j] = -2;
						n_aJet_csvN[j] = -2;
					}
					else {
						n_aJet_csvGen[j] = randomCSV;
						n_aJet_csvN[j] = iterations;
					}
				}
				else {
					n_aJet_csvGen[j] = histoMap[key] -> GetRandom();
				}
			}
		}
		
		u -> Fill();
		
		if(enableVerbose) ++(*show_progress);
	}
	
	if(enableVerbose) std::cout << "Writing to " << cmd_output << " ... " << std::endl;
	u -> Write();
	
	// close the files
	if(enableVerbose) {
		std::cout << "Closing " << inputFilename << "," << histoInputName << " and ";
		std::cout << cmd_output << " ... " << std::endl;
	}
	histograms -> Close();
	in -> Close();
	out -> Close();
	
	return EXIT_SUCCESS;
}
