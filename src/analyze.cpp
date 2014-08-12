#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <boost/timer.hpp>

#include <cstdlib> //EXIT_SUCCESS, std::abs
#include <iostream> // std::cout
#include <map> // std::map<>
#include <cmath> // std::fabs
#include <vector> // std::vector<>
#include <algorithm> // std::find, std::sort, std::accumulate, std::prev_permutation
#include <fstream> // std::ofstream

#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TKey.h>
#include <TROOT.h>
#include <TClass.h>
#include <TMath.h>

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
	friend bool operator == (const Lepton & jetL, const Lepton & jetR);
	
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

bool operator == (const Lepton & leptonL, const Lepton & leptonR) {
	bool returnValue = true;
	returnValue = returnValue && TMath::AreEqualAbs(leptonL.type, leptonR.type, 0.1);
	returnValue = returnValue && TMath::AreEqualAbs(leptonL.eta, leptonR.eta, 1e-6);
	returnValue = returnValue && TMath::AreEqualAbs(leptonL.pt, leptonR.pt, 1e-6);
	returnValue = returnValue && TMath::AreEqualAbs(leptonL.relIso, leptonR.relIso, 1e-6);
	return returnValue;
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
	friend bool operator == (const Jet & jetL, const Jet & jetR);
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

bool operator == (const Jet & jetL, const Jet & jetR) {
	bool returnValue = true;
	returnValue = returnValue && TMath::AreEqualAbs(jetL.flavor, jetR.flavor, 0.1);
	returnValue = returnValue && TMath::AreEqualAbs(jetL.eta, jetR.eta, 1e-6);
	returnValue = returnValue && TMath::AreEqualAbs(jetL.pt, jetR.pt, 1e-6);
	returnValue = returnValue && TMath::AreEqualAbs(jetL.csv, jetR.csv, 1e-6);
	return returnValue;
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
	std::string inFilename, treeName, hinput, cinput, outFilename;
	bool	enableVerbose = false, writeToFile = false, plotIterations = false,
			sampleOnce = false, sampleMultiple = false, useAnalytic = false;
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
			("cumulatives,c", po::value<std::string>(&cinput), "input cumulatives which are used to find analytic probability")
			("plot-iterations,p", "plot the iterations for every event")
			("sample-once,s", "sample only once")
			("sample-multiple,m", "sample multiple times")
			("use-analytic,a", "use analytic expression to calculate the passing probability of an event")
			("file,f", "write the statistics to a file")
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
		if(	vm.count("input") == 0 || vm.count("tree") == 0 || vm.count("output") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if((vm.count("Nj") == 0 || vm.count("Ntag") == 0) && (vm.count("sample-multiple") == 0 || vm.count("plot-iterations")))
		if((vm.count("Nj") == 0 || vm.count("Ntag") == 0) && vm.count("sample-once") > 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("file") > 0) {
			writeToFile = true;
		}
		if(vm.count("plot-iterations") > 0) {
			plotIterations = true;
		}
		if(vm.count("sample-once") > 0) {
			sampleOnce = true;
		}
		if(vm.count("sample-multiple") > 0) {
			sampleMultiple = true;
		}
		if(vm.count("use-analytic") > 0) {
			useAnalytic = true;
		}
		if(vm.count("use-analytic") == 0 && vm.count("histograms") == 0) {
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
	if(Nj < Ntag) {
		std::cerr << "cannot tag more jets than the event contains" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if((plotIterations && sampleOnce) || (plotIterations && sampleMultiple) || (sampleOnce && sampleMultiple)) {
		std::cerr << "pick only one flag of the following: -p -s -m" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if((writeToFile && sampleOnce) || (writeToFile && sampleMultiple)) {
		std::cerr << "the events can be written to a file only if it's specified with -p" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if(useAnalytic && cinput.empty()) {
		std::cerr << "the cumulatives must be specified if one wants to calculate analytic probability" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if(useAnalytic && (plotIterations || sampleOnce || sampleMultiple)) {
		std::cerr << "pick only one flag of the following: -p -s -m -a" << std::endl;
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
	
	std::map<TString, TH1F *> histograms;
	TFile * histoFile;
	if(! useAnalytic) {
		if(enableVerbose) std::cout << "Opening file " << hinput << " ..." << std::endl;
		histoFile = TFile::Open(hinput.c_str(), "read");
		if(histoFile -> IsZombie() || ! histoFile -> IsOpen()) {
			std::cerr << "Cannot open " << hinput << "." << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(enableVerbose) std::cout << "Reading all histograms ..." << std::endl;
		TKey * keyHisto;
		TIter nextHisto(histoFile -> GetListOfKeys());
		while((keyHisto = dynamic_cast<TKey *>(nextHisto()))) {
			TClass * cl = gROOT -> GetClass(keyHisto -> GetClassName());
			if(! cl -> InheritsFrom("TH1F")) continue;
			TH1F * h = dynamic_cast<TH1F *> (keyHisto -> ReadObj());
			histograms[h -> GetName()] = h;
		}
	}
	
	/************* read cumulatives *************************/
	
	std::map<TString, TH1F *> cumulatives;
	std::map<TString, Float_t> probabilities;
	if(useAnalytic) {
		if(enableVerbose) {
			std::cout << "Reading cumulatives from " << cinput << " ..." << std::endl;
		}
		// read the histograms
		TFile * cumulativeFile = TFile::Open(cinput.c_str(), "read");
		if(cumulativeFile -> IsZombie() || ! cumulativeFile -> IsOpen()) {
			std::cerr << "Cannot open " << cinput << "." << std::endl;
			std::exit(EXIT_FAILURE);
		}
		TKey * keyCumul;
		TIter nextCumul(cumulativeFile -> GetListOfKeys());
		while((keyCumul = dynamic_cast<TKey *>(nextCumul()))) {
			TClass * cl = gROOT -> GetClass(keyCumul -> GetClassName());
			if(! cl -> InheritsFrom("TH1F")) continue;
			TH1F * h = dynamic_cast<TH1F *> (keyCumul -> ReadObj());
			cumulatives[h -> GetName()] = h;
		}
		
		auto randLinpolEdge = [] (TH1F * h, Float_t x, Int_t bin) -> Float_t {
			Float_t x1, y1, x2, y2;
			x1 = h -> GetBinLowEdge(bin);
			y1 = h -> GetBinContent(bin - 1);
			x2 = h -> GetBinLowEdge(bin + 1);
			y2 = h -> GetBinContent(bin);
			Float_t y = y1 + (y2 - y1) * (x - x1) / (x2 - x1);
			return y;
		};
		
		// calculate the probabilities for the working point
		for(auto & kv: cumulatives) {
			Int_t bin = kv.second -> FindBin(CSVM);
			probabilities[kv.first] = 1.0 - randLinpolEdge(kv.second, CSVM, bin);
		}
		
		if(enableVerbose) {
			std::cout << "Closing " << cinput << " ..." << std::endl;
		}
		cumulativeFile -> Close();
	}
	
	auto comb = [] (std::vector<float> & v, int N, int K) -> float {
		std::string bitmask(K, 1); // K leading 1's
		bitmask.resize(N, 0); // N-K trailing 0's
		float sum_prob = 0;
		do {
			float prob = 1;
			for (int i = 0; i < N; ++i) {
				if (bitmask[i]) prob *= v[i];
				else prob *= (1 - v[i]);
			}
			sum_prob += prob;
		} while (std::prev_permutation(bitmask.begin(), bitmask.end()));
		return sum_prob;
	};
	
	/************** output file *****************************/
	
	if(enableVerbose) std::cout << "Creating file " << outFilename << " ..." << std::endl;
	TFile * out = TFile::Open(outFilename.c_str(), "recreate");
	TTree * u; // tree for output file
	if(! plotIterations) {
		u = new TTree(treeName.c_str(), treeName.c_str());
		u -> SetDirectory(out);
	}
	
	/*********** jet branches ***********************************/
	
	if(enableVerbose) std::cout << "Setting up branch addresses ..." << std::endl;
	
	t -> SetBranchAddress("nhJets", &nhJets);
	t -> SetBranchAddress("hJet_pt", &hJet_pt);
	t -> SetBranchAddress("hJet_eta", &hJet_eta);
	t -> SetBranchAddress("hJet_flavour", &hJet_flavour);
	//t -> SetBranchAddress("hJet_phi", &hJet_phi);
	//t -> SetBranchAddress("hJet_e", &hJet_e);
	//t -> SetBranchAddress("hJet_genPt", &hJet_genPt);
	t -> SetBranchAddress("hJet_csv", &hJet_csv);
	
	t -> SetBranchAddress("naJets", &naJets);
	t -> SetBranchAddress("aJet_pt", &aJet_pt);
	t -> SetBranchAddress("aJet_eta", &aJet_eta);
	t -> SetBranchAddress("aJet_flavour", &aJet_flavour);
	//t -> SetBranchAddress("aJet_phi", &aJet_phi);
	//t -> SetBranchAddress("aJet_e", &aJet_e);
	//t -> SetBranchAddress("aJet_genPt", &aJet_genPt);
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
	
	/*************** NEW TREE STUFF ***************************/
	/*************** jet branches *****************************/
	//int maxNumberOfHJets = 2;
	//int maxNumberOfAJets = 20; // see the definition above
	
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
	
	if(! plotIterations) {
	
		u -> Branch("nhJets", &n_nhJets, "nhJets/I");
		u -> Branch("hJet_pt", &n_hJet_pt, "hJet_pt[nhJets]/F");
		u -> Branch("hJet_eta", &n_hJet_eta, "hJet_eta[nhJets]/F");
		u -> Branch("hJet_csv", &n_hJet_csv, "hJet_csv[nhJets]/F");
		u -> Branch("hJet_flavour", &n_hJet_flavour, "hJet_flavour[nhJets]/F");
		//u -> Branch("hJet_phi", &n_hJet_phi, "hJet_phi[nhJets]/F");
		//u -> Branch("hJet_e", &n_hJet_e, "hJet_e[nhJets]/F");
		//u -> Branch("hJet_genPt", &n_hJet_genPt, "hJet_genPt[nhJets]/F");
		
		u -> Branch("naJets", &n_naJets, "naJets/I");
		u -> Branch("aJet_pt", &n_aJet_pt, "aJet_pt[naJets]/F");
		u -> Branch("aJet_eta", &n_aJet_eta, "aJet_eta[naJets]/F");
		u -> Branch("aJet_csv", &n_aJet_csv, "aJet_csv[naJets]/F");
		u -> Branch("aJet_flavour", &n_aJet_flavour, "aJet_flavour[naJets]/F");
		//u -> Branch("aJet_phi", &n_aJet_phi, "aJet_phi[naJets]/F");
		//u -> Branch("aJet_e", &n_aJet_e, "aJet_e[naJets]/F");
		//u -> Branch("aJet_genPt", &n_aJet_genPt, "aJet_genPt[naJets]/F");
		
	}
	
	/*************** lepton branches **************************/
	//int maxVLeptons = 2; // see the definition above
	//int maxALeptons = 100; // there were up to 40 leptons in the first 100k events
	
	Int_t n_nvlep;
	Int_t n_nalep;
	
	Float_t n_vLepton_pt[maxVLeptons];
	Float_t n_vLepton_eta[maxVLeptons];
	Float_t n_vLepton_pfCombRelIso[maxVLeptons];
	Int_t n_vLepton_type[maxVLeptons];
	//Float_t n_vLepton_id80[maxVLeptons];
	//Float_t n_vLepton_id95[maxVLeptons];
	//Float_t n_vLepton_charge[maxVLeptons];
	Float_t n_vLepton_idMVAtrig[maxVLeptons];
	//Float_t n_vLepton_idMVAnotrig[maxVLeptons];
	//Float_t n_vLepton_idMVApresel[maxVLeptons];
	//Float_t n_vLepton_particleIso[maxVLeptons];
	//Float_t n_vLepton_dxy[maxVLeptons];
	//Float_t n_vLepton_innerHits[maxVLeptons];
	
	Float_t n_aLepton_pt[maxALeptons];
	Float_t n_aLepton_eta[maxALeptons];
	Float_t n_aLepton_pfCombRelIso[maxALeptons];
	Int_t n_aLepton_type[maxALeptons];
	//Float_t n_aLepton_id80[maxALeptons];
	//Float_t n_aLepton_id95[maxALeptons];
	//Float_t n_aLepton_charge[maxALeptons];
	Float_t n_aLepton_idMVAtrig[maxALeptons];
	//Float_t n_aLepton_idMVAnotrig[maxALeptons];
	//Float_t n_aLepton_idMVApresel[maxALeptons];
	//Float_t n_aLepton_particleIso[maxALeptons];
	//Float_t n_aLepton_dxy[maxALeptons];
	//Float_t n_aLepton_innerHits[maxALeptons];
	
	if(! plotIterations) {
		
		u -> Branch("nvlep", &n_nvlep, "nvlep/I");
		u -> Branch("nalep", &n_nalep, "nalep/I");
		
		u -> Branch("vLepton_pt", &n_vLepton_pt, "vLepton_pt[nvlep]/F");
		u -> Branch("vLepton_eta", &n_vLepton_eta, "vLepton_eta[nvlep]/F");
		u -> Branch("vLepton_pfCombRelIso", &n_vLepton_pfCombRelIso, "vLepton_pfCombRelIso[nvlep]/F");
		u -> Branch("vLepton_type", &n_vLepton_type, "vLepton_type[nvlep]/I");
		u -> Branch("vLepton_idMVAtrig", &n_vLepton_idMVAtrig, "vLepton_idMVAtrig[nvlep]/F");
		
		u -> Branch("aLepton_pt", &n_aLepton_pt, "aLepton_pt[nalep]/F");
		u -> Branch("aLepton_eta", &n_aLepton_eta, "aLepton_eta[nalep]/F");
		u -> Branch("aLepton_pfCombRelIso", &n_aLepton_pfCombRelIso, "aLepton_pfCombRelIso[nalep]/F");
		u -> Branch("aLepton_type", &n_aLepton_type, "aLepton_type[nalep]/I");
		u -> Branch("aLepton_idMVAtrig", &n_aLepton_idMVAtrig, "aLepton_idMVAtrig[nalep]/F");
		
	}
	
	/************** sample once/multiple times *****************/
	
	Float_t n_hJet_csvGen[maxNumberOfHJets];
	Float_t n_aJet_csvGen[maxNumberOfAJets];
	Int_t n_btags;
	
	Float_t n_Jet_csvN;
	Float_t n_Jet_prob;
	
	if(! plotIterations) {
		if(sampleOnce) {
			u -> Branch("hJet_csvGen", &n_hJet_csvGen, "hJet_csvGen[nhJets]/F");
			u -> Branch("aJet_csvGen", &n_aJet_csvGen, "aJet_csvGen[naJets]/F");
			u -> Branch("btags", &n_btags, "btags/I");
		}
		else if(sampleMultiple) {
			u -> Branch("Jet_csvN", &n_Jet_csvN, "Jet_csvN/F");
			u -> Branch("Jet_prob", &n_Jet_prob, "Jet_prob/F");
		}
		else if(useAnalytic) {
			u -> Branch("Jet_prob", &n_Jet_prob, "Jet_prob/F");
		}
	}
	
	/************* create plaintext output file ****************/
	
	std::string textfilename = std::to_string(beginEvent) + "_" + std::to_string(endEvent) + ".txt";
	std::ofstream textfile;
	if(writeToFile) {
		if(enableVerbose) {
			std::cout << "Creating file " << textfilename << " ..."  << std::endl;
		}
		textfile.open(textfilename);
	}
	
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
		if((sumOfJets != Nj) || (sumOfJets != j_coll.size())) continue;
		/****************** sample unitl it passes the working point *****************/
		
		if(plotIterations) {
			if(writeToFile) {
				textfile << "EVENT:\t" << i << std::endl;
				for(auto jet: validJets) {
					textfile << jet << std::endl;
				}
			}
			Int_t iterMax = 100;
			std::string name = std::to_string(i) + "_event";
			TH1F * h = new TH1F(name.c_str(), name.c_str(), iterMax - 1, 1, iterMax);
			h -> SetDirectory(out);
			h -> SetTitle(name.c_str());
			std::vector<int> counter(Nj, 0);
			for(Int_t j = 1; j <= nIter; ++j) { // number of entries in histogram
				Int_t tagCounter = 0;
				for(Int_t iterations = 1; iterations <= nIterMax; ++iterations) { // csv generation loop
					for(auto jet: validJets) { // loop over jets one time per csv generation
						Float_t pt = getPtIndex(jet.getPt());
						Float_t eta = getEtaIndex(std::fabs(jet.getEta()));
						Float_t flavor = getFlavorIndex(std::fabs(jet.getFlavor()));
						std::string key = getName(flavor, pt, eta);
						Float_t r = histograms[key.c_str()] -> GetRandom();
						if(r >= CSVM) {
							++tagCounter;
							if(writeToFile) {
								std::vector<Jet>::iterator iter = std::find(validJets.begin(), validJets.end(), jet);
								std::size_t index = std::distance(validJets.begin(), iter);
								++counter[index];
							}
						}
						if(tagCounter == Ntag) break; // if enough number of btags found, quit the jet loop
					}
					if(tagCounter == Ntag) { // if enough number of btags found, fill the histogram
						if(tagCounter == Ntag) h -> Fill(iterations);
						else h -> Fill(-1); // if not enough iterations were made to pass the wp
						break;
					}
				}
			}
			if(writeToFile) {
				for(auto val: counter) {
					textfile << val << "\t";
				}
				textfile << std::endl;
				textfile << "----------------------------------" << std::endl;
			}
			h -> Write();
		}
		else {
			
			/************** fill tree branches **********************/
			// jets
			n_nhJets = nhJets;
			n_naJets = naJets;
			for(int j = 0; j < nhJets; ++j) {
				n_hJet_pt[j] = hJet_pt[j];
				n_hJet_eta[j] = hJet_eta[j];
				n_hJet_csv[j] = hJet_csv[j];
				n_hJet_flavour[j] = hJet_flavour[j];
			}
			for(int j = 0; j < naJets; ++j) {
				n_aJet_pt[j] = aJet_pt[j];
				n_aJet_eta[j] = aJet_eta[j];
				n_aJet_csv[j] = aJet_csv[j];
				n_aJet_flavour[j] = aJet_flavour[j];
			}
			// leptons
			n_nvlep = nvlep;
			n_nalep = nalep;
			for(int j = 0; j < nvlep; ++j) {
				n_vLepton_pt[j] = vLepton_pt[j];
				n_vLepton_eta[j] = vLepton_eta[j];
				n_vLepton_pfCombRelIso[j] = vLepton_pfCombRelIso[j];
				n_vLepton_type[j] = vLepton_type[j];
				n_vLepton_idMVAtrig[j] = vLepton_idMVAtrig[j];
			}
			for(int j = 0; j < nalep; ++j) {
				n_aLepton_pt[j] = aLepton_pt[j];
				n_aLepton_eta[j] = aLepton_eta[j];
				n_aLepton_pfCombRelIso[j] = aLepton_pfCombRelIso[j];
				n_aLepton_type[j] = aLepton_type[j];
				n_aLepton_idMVAtrig[j] = aLepton_idMVAtrig[j];
			}
			
			if(sampleMultiple) {
				std::vector<Int_t> iter;
				for(Int_t j = 1; j <= nIter; ++j) { // number of entries in histogram
					Int_t tagCounter = 0;
					for(Int_t iterations = 1; iterations <= nIterMax; ++iterations) { // csv generation loop
						for(auto jet: validJets) { // loop over jets one time per csv generation
							Float_t pt = getPtIndex(jet.getPt());
							Float_t eta = getEtaIndex(std::fabs(jet.getEta()));
							Float_t flavor = getFlavorIndex(std::fabs(jet.getFlavor()));
							std::string key = getName(flavor, pt, eta);
							Float_t r = histograms[key.c_str()] -> GetRandom();
							if(r >= CSVM) {
								++tagCounter;
							}
							if(tagCounter == Ntag) break; // if enough number of btags found, quit the jet loop
						}
						if(tagCounter == Ntag) { // if enough number of btags found, fill the histogram
							if(tagCounter == Ntag) iter.push_back(iterations);
							else {
								// if not enough iterations were made to pass the wp
							}
							break;
						}
					}
				}
				Int_t sum = std::accumulate(iter.begin(), iter.end(), 0);
				Float_t mean = float(sum) / iter.size();
				
				/************* fill additional branches ****************/
				n_Jet_csvN = mean;
				n_Jet_prob = 1.0 / mean;
			}
			else if(sampleOnce) {
				Int_t btagCounter = 0;
				for(int j = 0; j < naJets; ++j) {
					Float_t pt = getPtIndex(aJet_pt[j]);
					Float_t eta = getEtaIndex(std::fabs(aJet_eta[j]));
					Float_t flavor = getFlavorIndex(std::fabs(aJet_flavour[j]));
					std::string key = getName(flavor, pt, eta);
					n_aJet_csvGen[j] = histograms[key.c_str()] -> GetRandom();
					if(n_aJet_csvGen[j] >= CSVM) ++btagCounter;
				}
				for(int j = 0; j < nhJets; ++j) {
					Float_t pt = getPtIndex(hJet_pt[j]);
					Float_t eta = getEtaIndex(std::fabs(hJet_eta[j]));
					Float_t flavor = getFlavorIndex(std::fabs(hJet_flavour[j]));
					std::string key = getName(flavor, pt, eta);
					n_hJet_csvGen[j] = histograms[key.c_str()] -> GetRandom();
					if(n_hJet_csvGen[j] >= CSVM) ++btagCounter;
				}
				n_btags = btagCounter;
			}
			else if(useAnalytic) {
				std::vector<float> individualProbabilities;
				for(auto jet: validJets) {
					Float_t pt = getPtIndex(jet.getPt());
					Float_t eta = getEtaIndex(std::fabs(jet.getEta()));
					Float_t flavor = getFlavorIndex(std::fabs(jet.getFlavor()));
					std::string key = getName(flavor, pt, eta);
					individualProbabilities.push_back(probabilities[key.c_str()]);
				}
				n_Jet_prob = comb(individualProbabilities, Nj, Ntag);
			}
			u -> Fill();
		}
	}
	
	if(! plotIterations) u -> Write();
	
	/*********** close everything *******************************/
	
	if(enableVerbose) {
		std::cout << "Closing " << inFilename << (useAnalytic ? " and " : ", ")<< outFilename;
		std::cout << (useAnalytic ? " ...\n" : (" and " + hinput + " ...\n"));
	}
	in -> Close();
	out -> Close();
	if(! useAnalytic) {
		histoFile -> Close();
	}
	if(writeToFile) {
		if(enableVerbose) {
			std::cout << "Closing " << textfilename << " ..." << std::endl;
		}
		textfile.close();
	}
	
	return EXIT_SUCCESS;
}