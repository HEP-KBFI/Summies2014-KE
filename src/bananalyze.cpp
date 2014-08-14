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
	bool	enableVerbose = false;
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
		if(vm.count("Nj") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("Nj") == 0 || vm.count("Ntag") == 0) {
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
	
	/************* read cumulatives *************************/
	
	std::map<TString, TH1F *> cumulatives;
	std::map<TString, Float_t> probabilities;
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
	TTree * u = new TTree(treeName.c_str(), treeName.c_str());
	u -> SetDirectory(out);
	
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
	
	/*
	
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
	*/
	
	/************** sample once/multiple times *****************/
	/*
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
	*/
	
	/*********** loop over events *******************************/
	
	if(endEvent < 0) endEvent = t -> GetEntries();
	
	boost::progress_display * show_progress;
	if(enableVerbose) {
		Long64_t dif = endEvent - beginEvent;
		std::cout << "Looping over " << dif << " events ... " << std::endl;
		show_progress = new boost::progress_display(endEvent - beginEvent);
	}
	
	std::string bKey = "b", cKey = "c", lKey = "l";
	std::vector<std::string> flavorKeys = {bKey, cKey, lKey};
	auto findFlavor = [cKey, bKey, lKey] (Float_t flavorCode) -> std::string {
		if		(TMath::AreEqualAbs(flavorCode, 4, FL_EPS)) return cKey;
		else if (TMath::AreEqualAbs(flavorCode, 5, FL_EPS)) return bKey;
		else if (TMath::Abs(flavorCode) < 4 || TMath::AreEqualAbs(flavorCode, 21, FL_EPS)) return lKey;
		return "";
	};
	
	Float_t sumProb = 0.0, sumAProb = 0.0, sumCProb = 0.0;
	Int_t passedCuts = 0;
	for(Long64_t i = beginEvent; i < endEvent; ++i) {
		if(enableVerbose) ++(*show_progress);
		
		t -> GetEntry(i);
		//std::cout << i << std::endl;
		
		JetCollection j_coll;
		j_coll.add(nhJets, hJet_pt, hJet_eta, hJet_flavour, hJet_csv);
		j_coll.add(naJets, aJet_pt, aJet_eta, aJet_flavour, aJet_csv);
		
		j_coll.sortPt(); // sort by jet pt (descending)
		
		//for(auto & jet: j_coll) std::cout << jet << std::endl;
		//std::cout << "--------------------------------------" << std::endl;
		
		Jet jet = j_coll.getJet(0);
		if(jet.getPt() < 20 || std::fabs(jet.getEta()) >= 2.5) continue; // skip the event
		++passedCuts;
		
		Float_t pt = getPtIndex(jet.getPt());
		Float_t eta = getEtaIndex(std::fabs(jet.getEta()));
		Float_t flavor = getFlavorIndex(std::fabs(jet.getFlavor()));
		std::string key = getName(flavor, pt, eta);
		int Npass = 0;
		for(int iterations = 1; iterations <= nIter; ++iterations) {
			Float_t r = histograms[key.c_str()] -> GetRandom();
			if(r >= CSVM) ++Npass;
		}
		sumProb += float(Npass) / nIter;
		sumAProb += probabilities[key.c_str()];
		
		Float_t r = histograms[key.c_str()] -> GetRandom();
		if(r >= CSVM) ++sumCProb;
	}
	
	std::cout << "multiple sampling:\t" << std::fixed << sumProb / passedCuts << std::endl;
	std::cout << "analytic probability:\t" << std::fixed << sumAProb / passedCuts << std::endl;
	std::cout << "single sampling:\t" << std::fixed << sumCProb / passedCuts << std::endl;
	
	/*********** close everything *******************************/
	
	if(enableVerbose) {
		std::cout << "Closing " << inFilename << ", " << outFilename;
		std::cout << hinput << std::endl;
	}
	in -> Close();
	out -> Close();
	histoFile -> Close();
	
	return EXIT_SUCCESS;
}