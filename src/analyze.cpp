#include <boost/algorithm/string.hpp>
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
#include "Jet.hpp"
#include "JetCollection.hpp"

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	
	/*********** input ******************************************/
	std::string inFilename, treeName, hinput, cinput, outFilename;
	bool enableVerbose = false, sampleOnce = false, sampleMultiple = false, useAnalytic = false, requireExact = false;
	Long64_t beginEvent, endEvent;
	Int_t requiredJets, requiredBtags;
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
			("Nj,j", po::value<Int_t>(&requiredJets), "required number of jets per event")
			("Ntag,n", po::value<Int_t>(&requiredBtags), "required number of btags per event")
			("working-point,w", po::value<Float_t>(&CSVM) -> default_value(0.679000000), "CSV working point")
			("Niter-max,x", po::value<Int_t>(&nIterMax), "maximum number of iterations needed to generate the CSV value")
			("histograms,k", po::value<std::string>(&hinput), "input histograms which are used to generate random CSV")
			("cumulatives,c", po::value<std::string>(&cinput), "input cumulatives which are used to find analytic probability")
			("sample-once,s", "sample only once (needs -k flag)")
			("sample-multiple,m", "sample multiple times (needs -k flag)")
			("use-analytic,a", "find the analytic probability (needs -c flag)")
			("exact,X", "require exact number of jets")
			("verbose,v", "verbose mode (enables progressbar)")
		;
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		
		if(vm.count("help") > 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS); // ugly
		}
		if(vm.count("verbose") > 0) {
			enableVerbose = true;
		}
		if(	vm.count("input") == 0 || vm.count("tree") == 0 || vm.count("output") == 0 ||
			vm.count("Nj") == 0 || vm.count("Ntag") == 0
		) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("sample-once") > 0) {
			if(vm.count("histograms") == 0) {
				std::cout << desc << std::endl;
				std::exit(EXIT_FAILURE);
			}
			sampleOnce = true;
		}
		if(vm.count("sample-multiple") > 0) {
			if(vm.count("histograms") == 0) {
				std::cout << desc << std::endl;
				std::exit(EXIT_FAILURE);
			}
			sampleMultiple = true;
		}
		if(vm.count("use-analytic") > 0) {
			if(vm.count("cumulatives") == 0) {
				std::cout << desc << std::endl;
				std::exit(EXIT_FAILURE);
			}
			useAnalytic = true;
		}
		if(vm.count("exact") > 0) {
			requireExact = true;
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
	if(requiredBtags > requiredJets) {
		std::cerr << "required number of btags cannnot exceed the number of required jets" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if(requiredJets < 0 || requiredBtags < 0) {
		std::cerr << "required number of jets/btags cannot be negative" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if(!(sampleOnce || sampleMultiple || useAnalytic)) {
		std::cerr << "you have to specify at least one of the following flags: -s -m -a" << std::endl;
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
	TKey * keyHisto;
	if(sampleOnce || sampleMultiple) {
		if(enableVerbose) std::cout << "Opening file " << hinput << " ..." << std::endl;
		histoFile = TFile::Open(hinput.c_str(), "read");
		if(histoFile -> IsZombie() || ! histoFile -> IsOpen()) {
			std::cerr << "Cannot open " << hinput << "." << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(enableVerbose) std::cout << "Reading all histograms ..." << std::endl;
		TIter nextHisto(histoFile -> GetListOfKeys());
		while((keyHisto = dynamic_cast<TKey *>(nextHisto()))) {
			TClass * cl = gROOT -> GetClass(keyHisto -> GetClassName());
			if(! cl -> InheritsFrom("TH1F")) continue;
			TH1F * h = dynamic_cast<TH1F *> (keyHisto -> ReadObj());
			histograms[h -> GetName()] = h;
		}
	}
	
	/************* read cumulatives *************************/
	
	std::map<TString, Float_t> probabilities;
	if(useAnalytic) {
		std::map<TString, TH1F *> cumulatives;
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
	
	/************** NEW BRANCHES ****************************/
	
	Float_t n_btag_mProb;
	Float_t n_btag_aProb;
	Int_t n_btag_count;
	Float_t n_hJet_csvGen[maxNumberOfHJets];
	Float_t n_aJet_csvGen[maxNumberOfAJets];
	
	if(sampleOnce) {
		u -> Branch("btag_count", &n_btag_count, "btag_count/I");
		u -> Branch("hJet_csvGen", &n_hJet_csvGen, "hJet_csvGen[nhJets]/F");
		u -> Branch("aJet_csvGen", &n_aJet_csvGen, "aJet_csvGen[naJets]/F");
	}
	if(sampleMultiple) {
		u -> Branch("btag_mProb", &n_btag_mProb, "btag_mProb/F");
	}
	if(useAnalytic) {
		u -> Branch("btag_aProb", &n_btag_aProb, "btag_aProb/F");
	}
	
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
	
	Float_t aProb = 0.0, mProb = 0.0;
	Int_t bCounter = 0;
	
	for(Long64_t i = beginEvent; i < endEvent; ++i) {
		if(enableVerbose) ++(*show_progress);
		
		t -> GetEntry(i);
		
		JetCollection j_coll;
		j_coll.add(nhJets, hJet_pt, hJet_eta, hJet_flavour, hJet_csv, "h");
		j_coll.add(naJets, aJet_pt, aJet_eta, aJet_flavour, aJet_csv, "a");
		
		j_coll.sortPt(); // sort by jet pt (descending)
		
		/****************** find the correct jets **********************/
		JetCollection passedJets;
		for(auto & jet: j_coll) {
			if(jet.getPt() < 20 || std::fabs(jet.getEta()) >= 2.5) continue;
			Float_t pt = getPtIndex(jet.getPt());
			Float_t eta = getEtaIndex(std::fabs(jet.getEta()));
			Float_t flavor = getFlavorIndex(std::fabs(jet.getFlavor()));
			std::string key = getName(flavor, pt, eta);
			jet.setName(key);
			passedJets.add(jet);
			if(! requireExact) {
				if(passedJets.size() == requiredJets) break; // only first 'requiredJets' jets
			}
		}
		if(passedJets.size() != requiredJets) continue; // skip the event
		
		/************* copy tree branches ******************************/
		
		n_nhJets = nhJets;
		n_naJets = naJets;
		
		for(int j = 0; j < n_nhJets; ++j) {
			n_hJet_pt[j] = hJet_pt[j];
			n_hJet_eta[j] = hJet_eta[j];
			n_hJet_csv[j] = hJet_csv[j];
			n_hJet_flavour[j] = hJet_flavour[j];
		}
		
		for(int j = 0; j < n_naJets; ++j) {
			n_aJet_pt[j] = aJet_pt[j];
			n_aJet_eta[j] = aJet_eta[j];
			n_aJet_csv[j] = aJet_csv[j];
			n_aJet_flavour[j] = aJet_flavour[j];
		}
		
		/**************** sample multiple times ******************************/
		int Npass = 0;
		if(sampleMultiple) {
			for(int iterations = 1; iterations <= nIterMax; ++iterations) {
				int btagCounter = 0;
				for(auto & jet: passedJets) {
					Float_t r = histograms[jet.getName().c_str()] -> GetRandom();
					if(r >= CSVM) ++btagCounter;
				}
				if(btagCounter == requiredBtags) {
					++Npass;
				}
			}
		}
		
		/************ analytic combination of probabilities **********************/
		std::vector<Float_t> individualProbabilities;
		if(useAnalytic) {
			for(auto & jet: passedJets) {
				individualProbabilities.push_back(probabilities[jet.getName().c_str()]);
			}
		}
		
		/****************** sample once *********************************/
		int btagCounter = 0;
		if(sampleOnce) {
			std::map<std::string, std::vector<int> > indices;
			for(auto & jet: passedJets) {
				int index = jet.getIndex();
				std::string type = jet.getType();
				Float_t r = histograms[jet.getName().c_str()] -> GetRandom();
				if(boost::iequals(type, "a")) n_aJet_csvGen[index] = r;
				else if(boost::iequals(type, "h")) n_hJet_csvGen[index] = r;
				if(r >= CSVM) ++btagCounter;
				indices[type].push_back(index);
			}
			for(int j = 0; j < n_naJets; ++j) {
				if(std::find(indices["a"].begin(), indices["a"].end(), j) != indices["a"].end()) continue;
				n_aJet_csvGen[j] = -1.0;
			}
			for(int j = 0; j < n_nhJets; ++j) {
				if(std::find(indices["h"].begin(), indices["h"].end(), j) != indices["h"].end()) continue;
				n_hJet_csvGen[j] = -1.0;
			}
		}
		
		/*********** assign & fill the tree *******************/
		if(sampleMultiple) {
			n_btag_mProb = float(Npass) / nIterMax;
			mProb += n_btag_mProb;
		}
		if(useAnalytic) {
			n_btag_aProb = comb(individualProbabilities, requiredJets, requiredBtags);
			aProb += n_btag_aProb;
		}
		if(sampleOnce) {
			n_btag_count = btagCounter;
			if(btagCounter == requiredBtags) ++bCounter;
		}
		u -> Fill();
	}
	
	u -> Write();
	
	/************* print out the results ************************/
	if(enableVerbose) {
		if(sampleMultiple) {
			std::cout << "Multiple sampling:\t" << mProb << std::endl;
		}
		if(useAnalytic) {
			std::cout << "Analytic probability:\t" << aProb << std::endl;
		}
		if(sampleOnce) {
			std::cout << "Sampled once:\t\t" << bCounter << std::endl;
		}
	}
	
	/*********** close everything *******************************/
	
	if(enableVerbose) {
		std::cout << "Closing " << inFilename;
		if(sampleOnce || sampleMultiple) {
			std::cout << ", " << hinput;
		}
		std::cout << " and " << outFilename << " ..." << std::endl;
	}
	in -> Close();
	out -> Close();
	if(sampleOnce || sampleMultiple) {
		histoFile -> Close();
	}
	
	return EXIT_SUCCESS;
}