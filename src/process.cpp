#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <cmath> // std::pow, std::cosh
#include <map> // std::map
#include <string> // std::string
#include <iostream> // std::cout

#include <TString.h>
#include <TTree.h>
#include <TFile.h>
#include <TBranch.h>
#include <TH1F.h>
#include <TMath.h>

#define FL_EPS 0.1 // epsilon for flavor comparisons

const std::string flavorStrings  [3] = 	{"c", "b", "l"};
const std::string ptRangeStrings [6] = 	{"[20,30]", "[30,40]", "[40,60]", "[60,100]", "[100,160]", "[160,inf]"};
const std::string etaRangeStrings[3] = 	{"[0,0.8]", "[0.8,1.6]", "[1.6,2.5]"};

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

std::string getName(int flavorIndex, int ptIndex, int etaIndex) {
	std::string s = "csv_";
	s.append(flavorStrings[flavorIndex]);
	s.append("_");
	s.append(ptRangeStrings[ptIndex]);
	s.append("_");
	s.append(etaRangeStrings[etaIndex]);
	return s;
}

int main(int argc, char ** argv) { // begin, end, config file (filename, bins, csv range)
	
	namespace po = boost::program_options;
	using boost::property_tree::ptree; // ptree, read_ini
	
	// command line option parsing
	std::string fileName;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,I", po::value<std::string>(&fileName) -> default_value("config.ini"), "read config file")
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
		
		if(vm.count("input")) {
			std::cout << "Parsing configuration file " << fileName << " ... " << std::endl;
		}
		else {
			// dummy
		}
	}
	catch(std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		std::exit(0); // ugly
	}
	catch(...) {
		std::cerr << "exception of unkown type" << std::endl;
	}
	/*
	// parse config file
	ptree pt;
	read_ini(fileName, pt);
	auto trim = [] (std::string s) -> std::string {
		s = s.substr(0, s.find(";")); // lose the comment
		boost::algorithm::trim(s); // lose whitespaces around the string
		return s;
	};
	*/
	
	const Int_t bins = 120;
	const Float_t minCSV = -0.1;
	const Float_t maxCSV = 1.1;
	
	TFile * f = TFile::Open("TT10k.root", "read");
	TTree * t = new TTree();
	t = (TTree *) (f -> Get("tree"));
	
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
	
	TFile * out = new TFile("TT_csv.root", "recreate");
	
	std::map<const TString, TH1F *> histoMap;
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
				histoMap[s] -> SetDirectory(out);
			}
		}
	}
	
	const Long64_t num = t -> GetEntries();
	const Long64_t numPercent = num / 1000;
	for(Long64_t i = 1; i < num; ++i) {
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
				
				histoMap[getName(flavorIndex, ptIndex, etaIndex).c_str()] -> Fill(csv);
			}
		}
		if(i % numPercent == 0) std::cout << ((float)i / num * 100.0) << "%" << std::endl;
	}
	typedef std::map<TString, TH1F *>::iterator iterator;
	for(iterator it = histoMap.begin(); it != histoMap.end(); ++it) {
		it -> second -> Write();
	}
	out -> Close();
	f -> Close();
	
	return 0;
}