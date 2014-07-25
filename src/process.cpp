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
#include <cstdlib> // std::atoi(), std::atof()
#include <memory> // std::unique_ptr<>
#include <vector> // std::vector<>

#include <TString.h>
#include <TTree.h>
#include <TFile.h>
#include <TH1F.h>
#include <TMath.h>

#define FL_EPS 0.1 // epsilon for flavor comparisons

/**
 * @note Assumptions:
 *  - one tree, one file
 *  - flavors, and pt and eta ranges hardcoded
 *  @todo
 *  - find a way to produce N bash scripts
 *  - sew histograms from different files together
 *  - show csv distribution for different flavors of the same \
 *    variable range on a single canvas
 *  - normalize the distributions somehow
 *  - distant future: sample them
 */

// keep them constants out in the open
std::string flavorStrings  [3] = 	{"c", "b", "l"};
std::string ptRangeStrings [6] = 	{"[20,30]", "[30,40]", "[40,60]", "[60,100]", "[100,160]", "[160,inf]"};
std::string etaRangeStrings[3] = 	{"[0,0.8]", "[0.8,1.6]", "[1.6,2.5]"};

int getFlavorIndex(Float_t flavor);
int getPtIndex(Float_t pt);
int getEtaIndex(Float_t eta);
std::string getName(int flavorIndex, int ptIndex, int etaIndex);

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	using boost::property_tree::ptree; // ptree, read_ini
	
	// command line option parsing
	std::string configFile, cmd_outputFilename, cmd_dir;
	Long64_t beginEvent, endEvent;
	bool enableVerbose = false;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,I", po::value<std::string>(&configFile) -> default_value("config.ini"), "read config file")
			("begin,b", po::value<Long64_t>(&beginEvent) -> default_value(0), "the event number to start with")
			("end,e", po::value<Long64_t>(&endEvent) -> default_value(-1), "the event number to end with\ndefault (-1) means all events")
			("output,o", po::value<std::string>(&cmd_outputFilename), "output file name\nif not set, read from config file")
			("verbose,v", "verbose mode (enables progressbar)")
		;
		po::positional_options_description p;
		p.add("input", -1);
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
		po::notify(vm);
		
		if(vm.count("help")) {
			std::cout << desc << std::endl;
			std::exit(0); // ugly
		}
		if(vm.count("verbose") != 0) {
			enableVerbose = true;
		}
	}
	catch(std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		std::exit(0); // ugly
	}
	catch(...) {
		std::cerr << "exception of unkown type" << std::endl;
	}
	
	// sanity check
	if((endEvent >=0 && beginEvent > endEvent) || beginEvent < 0) {
		std::cerr << "incorrect values for begin and/or end" << std::endl;
		std::exit(0);
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
	
	const TString treeName = trim(pt_ini.get<std::string>("tree.val")).c_str(); // single tree assumed
	std::string config_inputFilename = trim(pt_ini.get<std::string>("input.in")).c_str(); // single file assumed
	std::string config_csvRanges = trim(pt_ini.get<std::string>("histogram.csvrange"));
	std::string config_bins = trim(pt_ini.get<std::string>("histogram.bins"));
	std::string config_outputFile = trim(pt_ini.get<std::string>("histogram.output"));
	
	// casting
	const Int_t bins = std::atoi(config_bins.c_str());
	int i = config_csvRanges.find(",");
	std::string s_minCSV = config_csvRanges.substr(0, i);
	std::string s_maxCSV = config_csvRanges.substr(i + 1);
	const Float_t minCSV = std::atof(s_minCSV.c_str());
	const Float_t maxCSV = std::atof(s_maxCSV.c_str());
	if(minCSV >= maxCSV) { // sanity check v2
		std::cerr << "wrong values for csv range" << std::endl;
		std::exit(0);
	}
	std::string outputFilename = cmd_outputFilename.empty() ? config_outputFile : cmd_outputFilename;
	outputFilename.append(".root");
	config_inputFilename.append(".root");
	
	// open the file and tree
	if(enableVerbose) std::cout << "Reading " << config_inputFilename << " ... " << std::endl;
	std::unique_ptr<TFile> in(TFile::Open(config_inputFilename.c_str(), "read"));
	if(in -> IsZombie() || ! in -> IsOpen()) {
		std::cerr << "error on opening the root file" << std::endl;
		std::exit(0);
	}
	if(enableVerbose) std::cout << "Accessing TTree " << treeName << " ... " << std::endl;
	TTree * t; // std::unique_ptr can't handle TTree .. 
	t = dynamic_cast<TTree *>(in -> Get(treeName));
	std::unique_ptr<TFile> out(new TFile(outputFilename.c_str(), "recreate"));
	
	// set up the variables
	// variables to be used are commented out for obv performance reasons
	if(enableVerbose) std::cout << "Setting up branch addresses ... " << std::endl;
	const int maxNumberOfHJets = 2;
	const int maxNumberOfAJets = 20;
	
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
	
	// initialize histogram map
	if(enableVerbose) std::cout << "Initializing histograms ... " << std::endl;
	std::map<const TString, TH1F *> histoMap; // no smart ptr for u
	for(int i = 0; i < 3; ++i) {
		for(int j = 0; j < 6; ++j) {
			for(int k = 0; k < 3; ++k) {
				std::string ss = "csv_";
				ss.append(flavorStrings[i]);
				ss.append("_");
				ss.append(ptRangeStrings[j]);
				ss.append("_");
				ss.append(etaRangeStrings[k]);
				TString s = ss.c_str();
				histoMap[s] = new TH1F(s, s, bins, minCSV, maxCSV);
				histoMap[s] -> SetDirectory(out.get());
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
				Float_t flavor, pt, eta, csv;
				//Float_t ptGen, phi, e, m2, m;
				
				//if(isHJet && hJet_genPt[j] > 0.0) ptGen = hJet_genPt[j];
				//if(!isHJet && aJet_genPt[j] > 0.0) ptGen = aJet_genPt[j];
				
				pt = isHJet ? hJet_pt[j] : aJet_pt[j];
				eta = isHJet ? hJet_eta[j] : aJet_eta[j];
				flavor = isHJet ? hJet_flavour[j] : aJet_flavour[j];
				csv = isHJet ? hJet_csv[j] : aJet_csv[j];
				//phi = isHJet ? hJet_phi[j] : aJet_phi[j];
				//e = isHJet ? hJet_e[j] : aJet_e[j];
				//m2 = e*e - TMath::Power(pt*TMath::CosH(eta), 2);
				//if(m2 < 0.0) m2 = 0;
				//m = std::sqrt(m2);
				
				int flavorIndex, ptIndex, etaIndex;
				if((flavorIndex = getFlavorIndex(flavor)) == -1) continue;
				if((ptIndex = getPtIndex(pt)) == -1) continue;
				if((etaIndex = getEtaIndex(eta)) == -1) continue;
				
				histoMap[getName(flavorIndex, ptIndex, etaIndex).c_str()] -> Fill(csv, 1); // for under/overflow
			}
		}
		if(enableVerbose) ++(*show_progress);
	}
	
	// write them histograms
	if(enableVerbose) std::cout << "Writing histograms to " << outputFilename << " ... " << std::endl;
	for(const auto & kv: histoMap) {
		kv.second -> Write();
	}
	
	// close the files
	if(enableVerbose) std::cout << "Closing " << config_inputFilename << " and " << outputFilename << " ... " << std::endl;
	in -> Close();
	out -> Close();
	
	return 0;
}

std::string getName(int flavorIndex, int ptIndex, int etaIndex) {
	std::string s = "csv_";
	s.append(flavorStrings[flavorIndex]);
	s.append("_");
	s.append(ptRangeStrings[ptIndex]);
	s.append("_");
	s.append(etaRangeStrings[etaIndex]);
	return s;
}

int getFlavorIndex(Float_t flavor) {
	if		(TMath::AreEqualAbs(flavor, 4, FL_EPS)) return 0;
	else if	(TMath::AreEqualAbs(flavor, 5, FL_EPS)) return 1;
	else if	(TMath::Abs(flavor) < 4 || TMath::AreEqualAbs(flavor, 21, FL_EPS))	return 2;
	return -1;
}

int getPtIndex(Float_t pt) {
	if		(20.0 <= pt && pt < 30.0) 	return 0;
	else if	(30.0 <= pt && pt < 40.0) 	return 1;
	else if	(40.0 <= pt && pt < 60.0) 	return 2;
	else if (60.0 <= pt && pt < 100.0)	return 3;
	else if (100.0 <= pt && pt < 160.0)	return 4;
	else if (160.0 <= pt) 				return 5;
	return -1;
}

int getEtaIndex(Float_t eta) {
	if		(0.0 <= eta && eta < 0.8)	return 0;
	else if	(0.8 <= eta && eta < 1.6)	return 1;
	else if	(1.6 <= eta && eta < 2.5)	return 2;
	return -1;
}