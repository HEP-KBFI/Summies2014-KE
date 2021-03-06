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
 * @note Assumptions:
 *  - one tree, one file
 *  - flavors, and pt and eta ranges hardcoded
 */

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	using boost::property_tree::ptree; // ptree, read_ini
	
	// command line option parsing
	std::string configFile, cmd_output, cmd_input, cmd_treeName; // cmd_mBins;
	Long64_t beginEvent, endEvent;
	bool enableVerbose = false, plotGeneratedCSV = false, plotSampleTries = false;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("config,c", po::value<std::string>(&configFile), "read config file")
			("begin,b", po::value<Long64_t>(&beginEvent) -> default_value(0), "the event number to start with")
			("end,e", po::value<Long64_t>(&endEvent) -> default_value(-1), "the event number to end with\ndefault (-1) means all events")
			("output,o", po::value<std::string>(&cmd_output), "output file name")
			("input,i", po::value<std::string>(&cmd_input), "input *.root file\nif not set, read from config file")
			("tree,t", po::value<std::string>(&cmd_treeName), "name of the tree\nif not set, read from config file")
			("use-CSVgen,g", "plot generated CSV value (default = use original CSV value); or")
			("use-CSVN,n", "plot the number of sample tries (default = use original CSV value)")
			("verbose,v", "verbose mode (enables progressbar)")
		;
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		
		if(vm.count("help")) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS); // ugly
		}
		if(vm.count("verbose")) {
			enableVerbose = true;
		}
		if(vm.count("use-CSVgen")) {
			plotGeneratedCSV = true;
		}
		if(vm.count("use-CSVN")) {
			plotSampleTries = true;
		}
		if(plotGeneratedCSV && plotSampleTries) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("config") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
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
	
	std::string cfg_treeName; // single tree assumed
	if(plotGeneratedCSV) {
		cfg_treeName = trim(pt_ini.get<std::string>("sample.tree"));
	}
	else if(plotSampleTries) {
		cfg_treeName = trim(pt_ini.get<std::string>("sample.tree"));
	}
	else {
		cfg_treeName = trim(pt_ini.get<std::string>("histogram.tree"));
	}
	std::string config_inputFilename = trim(pt_ini.get<std::string>("histogram.in")); // single file assumed
	std::string config_csvRanges = trim(pt_ini.get<std::string>("histogram.csvrange"));
	std::string config_bins = trim(pt_ini.get<std::string>("histogram.bins"));
	
	// casting
	const Int_t bins = std::atoi(config_bins.c_str());
	int i = config_csvRanges.find(",");
	std::string s_minCSV = config_csvRanges.substr(0, i);
	std::string s_maxCSV = config_csvRanges.substr(i + 1);
	const Float_t minCSV = std::atof(s_minCSV.c_str());
	const Float_t maxCSV = std::atof(s_maxCSV.c_str());
	if(minCSV >= maxCSV) { // sanity check v2
		std::cerr << "wrong values for csv range" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	std::string inputFilename = cmd_input.empty() ? config_inputFilename : cmd_input;
	std::string treeName = cmd_treeName.empty() ? cfg_treeName : cmd_treeName;
	//Int_t mBins = cmd_mBins.empty() ? std::atoi(cfg_mBins.c_str()) : std::atoi(cmd_mBins.c_str());
	
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
	t = dynamic_cast<TTree *>(in -> Get(treeName.c_str()));
	std::unique_ptr<TFile> out(new TFile(cmd_output.c_str(), "recreate"));
	
	// set up the variables
	// variables to be used are commented out for obv performance reasons
	if(enableVerbose) std::cout << "Setting up branch addresses ... " << std::endl;
	const int maxNumberOfHJets = 2;
	const int maxNumberOfAJets = 20;
	
	/******************************************************************************************************/
	
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
	
	Float_t hJet_csvGen[maxNumberOfHJets];
	Float_t aJet_csvGen[maxNumberOfAJets];
	
	Long64_t hJet_csvN[maxNumberOfHJets];
	Long64_t aJet_csvN[maxNumberOfAJets];
	
	t -> SetBranchAddress("nhJets", &nhJets);
	t -> SetBranchAddress("hJet_pt", &hJet_pt);
	t -> SetBranchAddress("hJet_eta", &hJet_eta);
	t -> SetBranchAddress("hJet_flavour", &hJet_flavour);
	//t -> SetBranchAddress("hJet_phi", &hJet_phi);
	//t -> SetBranchAddress("hJet_e", &hJet_e);
	//t -> SetBranchAddress("hJet_genPt", &hJet_genPt);
	if(plotGeneratedCSV) {
		t -> SetBranchAddress("hJet_csvGen", &hJet_csvGen);
	}
	else if(plotSampleTries) {
		t -> SetBranchAddress("hJet_csvN", &hJet_csvN);
	}
	else {
		t -> SetBranchAddress("hJet_csv", &hJet_csv);
	}
	
	t -> SetBranchAddress("naJets", &naJets);
	t -> SetBranchAddress("aJet_pt", &aJet_pt);
	t -> SetBranchAddress("aJet_eta", &aJet_eta);
	t -> SetBranchAddress("aJet_flavour", &aJet_flavour);
	//t -> SetBranchAddress("aJet_phi", &aJet_phi);
	//t -> SetBranchAddress("aJet_e", &aJet_e);
	//t -> SetBranchAddress("aJet_genPt", &aJet_genPt);
	if(plotGeneratedCSV) {
		t -> SetBranchAddress("aJet_csvGen", &aJet_csvGen);
	}
	else if(plotSampleTries) {
		t -> SetBranchAddress("aJet_csvN", &aJet_csvN);
	}
	else {
		t -> SetBranchAddress("aJet_csv", &aJet_csv);
	}
	
	
	// initialize histogram map
	if(enableVerbose) std::cout << "Initializing histograms ... " << std::endl;
	std::map<const TString, TH1F *> histoMap; // no smart ptr for u
	for(int i = 0; i < 3; ++i) {
		for(int j = 0; j < 6; ++j) {
			for(int k = 0; k < 3; ++k) {
				std::string name;
				if(plotGeneratedCSV) 		name = getName(i, j, k, "csvGen_");
				else if(plotSampleTries) 	name = getName(i, j, k, "csvN_");
				else				 		name = getName(i, j, k, "csv_");
				TString s = name.c_str();
				if(plotSampleTries) histoMap[s] = new TH1F(s, s, XendpointMultisample[i] + 3, -3, XendpointMultisample[i]);
				else 				histoMap[s] = new TH1F(s, s, bins, minCSV, maxCSV);
				histoMap[s] -> SetDirectory(out.get());
				histoMap[s] -> Sumw2();
			}
		}
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
		for(int coll = 0; coll < 2; ++coll) {
			bool isHJet = (coll == 0);
			for(int j = 0; j < (isHJet ? nhJets : naJets); ++j) {
				Float_t flavor, pt, eta, X;
				//Float_t ptGen, phi, e, m2, m;
				
				//if(isHJet && hJet_genPt[j] > 0.0) ptGen = hJet_genPt[j];
				//if(!isHJet && aJet_genPt[j] > 0.0) ptGen = aJet_genPt[j];
				
				pt = isHJet ? hJet_pt[j] : aJet_pt[j];
				eta = isHJet ? hJet_eta[j] : aJet_eta[j];
				flavor = isHJet ? hJet_flavour[j] : aJet_flavour[j];
				
				if(plotSampleTries) 		X = isHJet ? hJet_csvN[j] : aJet_csvN[j]; // should be Long64_t tho
				else if(plotGeneratedCSV) 	X = isHJet ? hJet_csvGen[j] : aJet_csvGen[j];
				else 						X = isHJet ? hJet_csv[j] : aJet_csv[j];
				//phi = isHJet ? hJet_phi[j] : aJet_phi[j];
				//e = isHJet ? hJet_e[j] : aJet_e[j];
				//m2 = e*e - TMath::Power(pt*TMath::CosH(eta), 2);
				//if(m2 < 0.0) m2 = 0;
				//m = std::sqrt(m2);
				
				Float_t absEta = TMath::Abs(eta); // only the absolute value matters
				Float_t absFlavor = TMath::Abs(flavor); // antiparticles too
				int flavorIndex, ptIndex, etaIndex;
				if((flavorIndex = getFlavorIndex(absFlavor)) == -1) continue;
				if((ptIndex = getPtIndex(pt)) == -1) continue;
				if((etaIndex = getEtaIndex(absEta)) == -1) continue;
				
				std::string name;
				if(plotGeneratedCSV) 		name = getName(flavorIndex, ptIndex, etaIndex, "csvGen_");
				else if(plotSampleTries)  	name = getName(flavorIndex, ptIndex, etaIndex, "csvN_");
				else				 		name = getName(flavorIndex, ptIndex, etaIndex, "csv_");
				
				histoMap[name.c_str()] -> Fill(X, 1); // for under/overflow
			}
		}
		if(enableVerbose) ++(*show_progress);
	}
	
	// write them histograms
	if(enableVerbose) std::cout << "Writing histograms to " << cmd_output << " ... " << std::endl;
	for(const auto & kv: histoMap) {
		kv.second -> Write();
	}
	
	// close the files
	if(enableVerbose) std::cout << "Closing " << inputFilename << " and " << cmd_output << " ... " << std::endl;
	in -> Close();
	out -> Close();
	
	return EXIT_SUCCESS;
}
