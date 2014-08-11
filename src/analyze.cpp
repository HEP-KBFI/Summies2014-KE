#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <boost/timer.hpp>

#include <cstdlib> //EXIT_SUCCESS, std::abs
#include <iostream> // std::cout
#include <map> // std::map<>
#include <cmath> // std::fabs
#include <vector> // std::vector<>
#include <algorithm> // std::find, std::sort

#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TKey.h>
#include <TROOT.h>
#include <TClass.h>

#include "common.hpp"

class Lepton {
public:
	Lepton(Float_t pt, Float_t eta, Float_t relIso, Int_t type)
		: pt(pt), eta(eta), relIso(relIso), type(type) { }
	Float_t getPt() const { return pt; }
	Float_t getEta() const { return eta; }
	Float_t getRelIso() const { return relIso; }
	Int_t getType() const { return type; }
	friend std::ostream & operator << (std::ostream &, const Lepton &);
private:
	Float_t pt;
	Float_t eta;
	Float_t relIso;
	Int_t type;
};

std::ostream & operator << (std::ostream & stream, const Lepton & lepton) {
	stream << "lepton pt: " << lepton.getPt() << std::endl;
	stream << "lepton eta: " << lepton.getEta() << std::endl;
	stream << "lepton relIso: " << lepton.getRelIso() << std::endl;
	stream << "lepton type: " << lepton.getType() << std::endl;
	return stream;
}

class Jet {
public:
	Jet(Float_t pt, Float_t eta, Float_t flavor, Float_t csv)
		: pt(pt), eta(eta), flavor(flavor), csv(csv) { }
	Float_t getPt() const { return pt; }
	Float_t getEta() const { return eta; }
	Float_t getFlavor() const { return flavor; }
	Float_t getCSV() const { return csv; }
	friend std::ostream & operator << (std::ostream &, const Jet &);
private:
	Float_t pt;
	Float_t eta;
	Float_t flavor;
	Float_t csv;
};

std::ostream & operator << (std::ostream & stream, const Jet & jet) {
	stream << "jet pt: " << jet.getPt() << std::endl;
	stream << "jet eta: " << jet.getEta() << std::endl;
	stream << "jet flavor: " << jet.getFlavor() << std::endl;
	stream << "jet CSV: " << jet.getCSV() << std::endl;
	return stream;
}

class LeptonCollection {
public:
	LeptonCollection() { }
	void add(Int_t nLeptons, Float_t * pt, Float_t * eta,  Float_t * relIso, Int_t * type) {
		for(Int_t i = 0; i < nLeptons; ++i) {
			leptons.push_back(Lepton(pt[i], eta[i], relIso[i], type[i]));
		}
	}
	void add(Lepton l) {
		leptons.push_back(l);
	}
	std::vector<Lepton>::iterator begin() { return leptons.begin(); }
	std::vector<Lepton>::iterator end() { return leptons.end(); }
	std::vector<Lepton>::const_iterator begin() const { return leptons.begin(); }
	std::vector<Lepton>::const_iterator end() const { return leptons.end(); }
	void sortPt() {
		std::sort(leptons.begin(), leptons.end(),
			[] (Lepton L1, Lepton L2) -> bool {
				return L1.getPt() > L2.getPt();
			}
		);
	}
	Lepton & getLepton(int i) {
		return leptons[i];
	}
	std::size_t size() const {
		return leptons.size();
	}
private:
	std::vector<Lepton> leptons;
};

class JetCollection {
public:
	JetCollection() { }
	void add (Int_t nJets, Float_t * pt, Float_t * eta, Float_t * flavor, Float_t * csv) {
		for(Int_t i = 0; i < nJets; ++i) {
			jets.push_back(Jet(pt[i], eta[i], flavor[i], csv[i]));
		}
	}
	void add(Jet j) {
		jets.push_back(j);
	}
	std::vector<Jet>::iterator begin() { return jets.begin(); }
	std::vector<Jet>::iterator end() { return jets.end(); }
	std::vector<Jet>::const_iterator begin() const { return jets.begin(); }
	std::vector<Jet>::const_iterator end() const { return jets.end(); }
	void sortPt() {
		std::sort(jets.begin(), jets.end(),
			[] (Jet J1, Jet J2) -> bool {
				return J1.getPt() > J2.getPt();
			}
		);
	}
	Jet & getJet(int i) {
		return jets[i];
	}
	std::size_t size() const {
		return jets.size();
	}
private:
	std::vector<Jet> jets;
};

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	
	/*********** input ******************************************/
	std::string inFilename, treeName, hinput, outFilename;
	bool enableVerbose = false;
	Long64_t beginEvent, endEvent;
	Int_t Nj, Ntag;
	Float_t CSVM;
	Int_t nIter, nIterMax;
	
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,i", po::value<std::string>(&inFilename), "input *.root file")
			("output,o", po::value<std::string>(&outFilename), "output *.root file")
			("tree,t", po::value<std::string>(&treeName), "name of the tree")
			("begin,b", po::value<Long64_t>(&beginEvent) -> default_value(0), "the event number to start with")
			("end,e", po::value<Long64_t>(&endEvent) -> default_value(-1), "the event number to end with\ndefault (-1) means all events")
			("Nj,j", po::value<Int_t>(&Nj), "required number of jets per event")
			("Ntag,n", po::value<Int_t>(&Ntag), "required number of btags per event")
			("working-point,w", po::value<Float_t>(&CSVM) -> default_value(0.679), "CSV working point")
			("Niter,r", po::value<Int_t>(&nIter), "number of CSV needed to pass the working point")
			("Niter-max,x", po::value<Int_t>(&nIterMax), "maximum number of iterations needed to generate the CSV value")
			("histograms,k", po::value<std::string>(&hinput), "input histograms which are used to generate random CSV")
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
		if(	vm.count("input") == 0 || vm.count("tree") == 0 ||
			vm.count("Nj") == 0 || vm.count("Ntag") == 0 || vm.count("output") == 0
		) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS);
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
	if(Nj < Ntag) {
		std::cerr << "cannot tag more jets than the event contains" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	/*********** jets *******************************************/
	
	int maxNumberOfHJets = 2;
	int maxNumberOfAJets = 20;
	
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
	
	//Float_t hJet_csvGen[maxNumberOfHJets]; // SAMPLING
	//Float_t aJet_csvGen[maxNumberOfAJets]; // SAMPLING
	
	//Long64_t hJet_csvN[maxNumberOfHJets]; // SAMPLING
	//Long64_t aJet_csvN[maxNumberOfAJets]; // SAMPLING
	
	/*********** sample vars ************************************/
	
	//Float_t hJet_csvGen[maxNumberOfHJets];
	//Float_t aJet_csvGen[maxNumberOfAJets];
	
	//Long64_t hJet_csvN[maxNumberOfHJets];
	//Long64_t aJet_csvN[maxNumberOfAJets];
	
	/*********** leptons ****************************************/
	
	int maxVLeptons = 2;
	int maxALeptons = 100; // there were up to40 leptons in the first 100k events
	
	Int_t nvlep;
	Int_t nalep;
	
	Float_t vLepton_pt[maxVLeptons];
	Float_t vLepton_eta[maxVLeptons];
	Float_t vLepton_pfCombRelIso[maxVLeptons];
	Int_t vLepton_type[maxVLeptons];
	//Float_t vLepton_id80[maxVLeptons];
	//Float_t vLepton_id95[maxVLeptons];
	//Float_t vLepton_charge[maxVLeptons];
	Float_t vLepton_idMVAtrig[maxVLeptons];
	//Float_t vLepton_idMVAnotrig[maxVLeptons];
	//Float_t vLepton_idMVApresel[maxVLeptons];
	//Float_t vLepton_particleIso[maxVLeptons];
	//Float_t vLepton_dxy[maxVLeptons];
	//Float_t vLepton_innerHits[maxVLeptons];
	
	Float_t aLepton_pt[maxALeptons];
	Float_t aLepton_eta[maxALeptons];
	Float_t aLepton_pfCombRelIso[maxALeptons];
	Int_t aLepton_type[maxALeptons];
	//Float_t aLepton_id80[maxALeptons];
	//Float_t aLepton_id95[maxALeptons];
	//Float_t aLepton_charge[maxALeptons];
	Float_t aLepton_idMVAtrig[maxALeptons];
	//Float_t aLepton_idMVAnotrig[maxALeptons];
	//Float_t aLepton_idMVApresel[maxALeptons];
	//Float_t aLepton_particleIso[maxALeptons];
	//Float_t aLepton_dxy[maxALeptons];
	//Float_t aLepton_innerHits[maxALeptons];
	
	/*********** open files *************************************/
	if(enableVerbose) std::cout << "Opening file " << inFilename << " ..." << std::endl;
	TFile * in = TFile::Open(inFilename.c_str(), "read");
	if(in -> IsZombie() || ! in -> IsOpen()) {
		std::cerr << "Cannot open " << inFilename << "." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if(enableVerbose) std::cout << "Accessing tree " << treeName << " ..." << std::endl;
	TTree * t = dynamic_cast<TTree *> (in -> Get(treeName.c_str()));
	
	/*********** read histograms ******************************/
	
	if(enableVerbose) std::cout << "Opening file " << hinput << " ..." << std::endl;
	TFile * histoFile = TFile::Open(hinput.c_str(), "read");
	if(histoFile -> IsZombie() || ! histoFile -> IsOpen()) {
		std::cerr << "Cannot open " << hinput << "." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if(enableVerbose) std::cout << "Reading all histograms ..." << std::endl;
	std::map<TString, TH1F *> histograms;
	TKey * keyHisto;
	TIter nextHisto(histoFile -> GetListOfKeys());
	while((keyHisto = dynamic_cast<TKey *>(nextHisto()))) {
		TClass * cl = gROOT -> GetClass(keyHisto -> GetClassName());
		if(! cl -> InheritsFrom("TH1F")) continue;
		TH1F * h = dynamic_cast<TH1F *> (keyHisto -> ReadObj());
		histograms[h -> GetName()] = h;
	}
	
	/************** output file *****************************/
	
	if(enableVerbose) std::cout << "Creating file " << outFilename << " ..." << std::endl;
	TFile * out = TFile::Open(outFilename.c_str(), "recreate");
	
	/*********** jet branches ***********************************/
	
	if(enableVerbose) std::cout << "Setting up branch addresses ..." << std::endl;
	
	t -> SetBranchAddress("nhJets", &nhJets);
	t -> SetBranchAddress("hJet_pt", &hJet_pt);
	t -> SetBranchAddress("hJet_eta", &hJet_eta);
	t -> SetBranchAddress("hJet_flavour", &hJet_flavour);
	//t -> SetBranchAddress("hJet_phi", &hJet_phi);
	//t -> SetBranchAddress("hJet_e", &hJet_e);
	//t -> SetBranchAddress("hJet_genPt", &hJet_genPt);
	//t -> SetBranchAddress("hJet_csvGen", &hJet_csvGen); // SAMPLING
	//t -> SetBranchAddress("hJet_csvN", &hJet_csvN); // SAMPLING
	t -> SetBranchAddress("hJet_csv", &hJet_csv);
	
	t -> SetBranchAddress("naJets", &naJets);
	t -> SetBranchAddress("aJet_pt", &aJet_pt);
	t -> SetBranchAddress("aJet_eta", &aJet_eta);
	t -> SetBranchAddress("aJet_flavour", &aJet_flavour);
	//t -> SetBranchAddress("aJet_phi", &aJet_phi);
	//t -> SetBranchAddress("aJet_e", &aJet_e);
	//t -> SetBranchAddress("aJet_genPt", &aJet_genPt);
	//t -> SetBranchAddress("aJet_csvGen", &aJet_csvGen); // SAMPLING
	//t -> SetBranchAddress("aJet_csvN", &aJet_csvN); // SAMPLING
	t -> SetBranchAddress("aJet_csv", &aJet_csv);
	
	/*********** lepton branches ********************************/
	
	t -> SetBranchAddress("nvlep", &nvlep);
	t -> SetBranchAddress("nalep", &nalep);
	
	t -> SetBranchAddress("vLepton_pt", &vLepton_pt);
	t -> SetBranchAddress("aLepton_pt", &aLepton_pt);
	t -> SetBranchAddress("vLepton_eta", &vLepton_eta);
	t -> SetBranchAddress("aLepton_eta", &aLepton_eta);
	t -> SetBranchAddress("vLepton_pfCombRelIso", &vLepton_pfCombRelIso);
	t -> SetBranchAddress("aLepton_pfCombRelIso", &aLepton_pfCombRelIso);
	t -> SetBranchAddress("vLepton_type", &vLepton_type);
	t -> SetBranchAddress("aLepton_type", &aLepton_type);
	t -> SetBranchAddress("vLepton_idMVAtrig", &vLepton_idMVAtrig);
	t -> SetBranchAddress("aLepton_idMVAtrig", &aLepton_idMVAtrig);
	
	/*********** loop over events *******************************/
	
	if(endEvent < 0) endEvent = t -> GetEntries();
	
	boost::progress_display * show_progress;
	if(enableVerbose) {
		Long64_t dif = endEvent - beginEvent;
		std::cout << "Looping over " << dif << " events ... " << std::endl;
		show_progress = new boost::progress_display(endEvent - beginEvent);
	}
	
	std::string tight = "tight", loose = "loose";
	std::string bKey = "b", cKey = "c", lKey = "l";
	std::vector<std::string> flavorKeys = {bKey, cKey, lKey};
	auto findMuonType = [tight,loose] (std::map<std::string, int> & leptons, Float_t pt, Float_t eta, Float_t relIso) -> void {
		if		(pt > 26.0 && std::fabs(eta) < 2.1 && relIso < 0.12) leptons[tight]++;
		else if	(pt > 20.0 && std::fabs(eta) < 2.4 && relIso < 0.20) leptons[loose]++;
	};
	auto findElectronType = [tight,loose] (std::map<std::string, int> & leptons, Float_t pt, Float_t eta, Float_t relIso) -> void {
		if		(pt > 30.0 && std::fabs(eta) < 2.5 && relIso < 0.10) leptons[tight]++;
		else if	(pt > 20.0 && std::fabs(eta) < 2.5 && relIso < 0.15) leptons[loose]++;
	};
	auto findFlavor = [cKey, bKey, lKey] (Float_t flavorCode) -> std::string {
		if		(TMath::AreEqualAbs(flavorCode, 4, FL_EPS)) return cKey;
		else if (TMath::AreEqualAbs(flavorCode, 5, FL_EPS)) return bKey;
		else if (TMath::Abs(flavorCode) < 4 || TMath::AreEqualAbs(flavorCode, 21, FL_EPS)) return lKey;
		return "";
	};
	
	for(Long64_t i = beginEvent; i < endEvent; ++i) {
		if(enableVerbose) ++(*show_progress);
		
		t -> GetEntry(i);
		
		LeptonCollection l_coll;
		l_coll.add(nvlep, vLepton_pt, vLepton_eta, vLepton_pfCombRelIso, vLepton_type);
		l_coll.add(nalep, aLepton_pt, aLepton_eta, aLepton_pfCombRelIso, aLepton_type);
		
		JetCollection j_coll;
		j_coll.add(nhJets, hJet_pt, hJet_eta, hJet_flavour, hJet_csv);
		j_coll.add(naJets, aJet_pt, aJet_eta, aJet_flavour, aJet_csv);
		
		l_coll.sortPt(); // sort by lepton pt (descending)
		j_coll.sortPt(); // sort by jet pt (descending)
		
		/********************** lepton cut ************************/
		bool proceed = true;
		
		std::map<std::string, int> leptons;
		leptons[tight] = 0;
		leptons[loose] = 0;
		
		for(auto & lepton: l_coll) {
			Float_t pt = lepton.getPt();
			Float_t eta = lepton.getEta();
			Float_t relIso = lepton.getRelIso();
			Int_t type = lepton.getType();
			if		(std::abs(type) == 11) { // if electron
				findElectronType(leptons, pt, eta, relIso);
			}
			else if	(std::abs(type) == 13) { // if muon
				findMuonType(leptons, pt, eta, relIso);
			}
			if(leptons[tight] > 1) {
				proceed = false;
				break;
			}
			if(leptons[loose] > 0) {
				proceed = false;
				break;
			}
		}
		if(! proceed || leptons[tight] != 1 || leptons[loose] > 0) continue;
		
		/********************** cut them jets ****************************/
		if(j_coll.size() < Nj) continue;
		
		std::vector<Jet> validJets;
		for(auto & jet: j_coll) {
			if(jet.getPt() > 30.0 && std::fabs(jet.getEta()) < 2.5) {
				validJets.push_back(jet);
			}
		}
		int sumOfJets = validJets.size();
		if(sumOfJets != Nj) continue;
		/****************** sample unitl it passes the working point *****************/
		for(int jetIndex = 0; jetIndex < Ntag; ++jetIndex) {
			std::string name = std::to_string(i) + "_event" + std::to_string(jetIndex) + "_jet";
			Float_t pt = getPtIndex(validJets[jetIndex].getPt());
			Float_t eta = getEtaIndex(std::fabs(validJets[jetIndex].getEta()));
			Float_t flavor = getFlavorIndex(std::fabs(validJets[jetIndex].getFlavor()));
			std::string key = getName(flavor, pt, eta);
			Int_t iterMax = 400;
			
			TH1F * h = new TH1F(name.c_str(), name.c_str(), iterMax - 1, 1, iterMax);
			h -> SetDirectory(out);
			h -> SetTitle(getName(flavor, pt, eta, "iterations_").c_str());
			for(int j = 0; j < nIter; ++j) {
				Float_t CSVgen = -1;
				int iterations = 0;
				for(; iterations < nIterMax; ++iterations) {
					Float_t r = histograms[key.c_str()] -> GetRandom();
					if(r >= CSVM) {
						CSVgen = r;
						break;
					}
				}
				if(CSVgen < CSVM) h -> Fill(-1);
				else h -> Fill(iterations);
			}
			h -> Write();
		}
	}
	
	/*********** close everything *******************************/
	
	if(enableVerbose) {
		std::cout << "Closing " << inFilename << ", " << outFilename;
		std::cout << " and " << hinput << " ..." << std::endl;
	}
	in -> Close();
	out -> Close();
	histoFile -> Close();
	
	return EXIT_SUCCESS;
}