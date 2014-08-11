#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <boost/timer.hpp>

#include <cmath> // std::pow(), std::cosh()
#include <map> // std::map<>
#include <string> // std::string
#include <iostream> // std::cout, std::cerr, std::endl
#include <cstdlib> // std::atoi(), std::atof(), EXIT_SUCCESS
#include <memory> // std::unique_ptr<>
#include <random> // std::mt19937_64
#include <chrono> // std::chrono

#include <TString.h>
#include <TTree.h>
#include <TFile.h>
#include <TH1F.h>
#include <TKey.h>
#include <TROOT.h>
#include <TClass.h>

#include "common.hpp"

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	
	// command line option parsing
	std::string output, input, hinput, cinput, tree, newtree;
	Long64_t beginEvent, endEvent;
	Float_t workingPoint;
	Int_t maxSamples;
	bool enableVerbose = false, sampleALot = false;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,i", po::value<std::string>(&input), "input *.root file")
			("tree,t", po::value<std::string>(&tree), "tree name of the input *.root file")
			("newtree,n", po::value<std::string>(&newtree), "new tree name of the output file")
			("cumulatives,c", po::value<std::string>(&cinput), "cumulatives for random number generation")
			("histograms,k", po::value<std::string>(&hinput), "histograms for random number generation")
			("begin,b", po::value<Long64_t>(&beginEvent) -> default_value(0), "the event number to start with")
			("end,e", po::value<Long64_t>(&endEvent) -> default_value(-1), "the event number to end with\ndefault (-1) means all events")
			("output,o", po::value<std::string>(&output), "output file name")
			("working-point,w", po::value<Float_t>(&workingPoint) -> default_value(0.679), "working point of the CSV value (default CSVM)")
			("max-samples,s", po::value<Int_t>(&maxSamples), "maximum number of samples")
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
		if(vm.count("output") == 0 || vm.count("tree") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("multiple-sampling")) {
			if(vm.count("max-samples") == 0) {
				std::cout << desc << std::endl;
				std::exit(EXIT_FAILURE);
			}
			sampleALot = true;
		}
		if(vm.count("newtree") == 0) {
			newtree = tree;
		}
		if((vm.count("cumulatives") > 0 && vm.count("histograms") > 0) || 
			(vm.count("cumulatives") == 0 && vm.count("histograms") == 0)) {
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
	
	/******************************************************************************************************/
	
	// open the file and tree
	if(enableVerbose) std::cout << "Reading " << input << " ... " << std::endl;
	std::unique_ptr<TFile> in(TFile::Open(input.c_str(), "read"));
	if(in -> IsZombie() || ! in -> IsOpen()) {
		std::cerr << "error on opening " << input << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if(enableVerbose) std::cout << "Accessing TTree " << tree << " ... " << std::endl;
	TTree * t; // std::unique_ptr can't handle TTree .. 
	t = dynamic_cast<TTree *>(in -> Get(tree.c_str()));
	
	// which method to use to generate random vars
	bool useCumul = ! cinput.empty();
	
	// open cumulatives
	std::map<TString, TH1F *> histoCumul;
	TFile * fcumul;
	TKey * keyCumul;
	
	std::map<TString, TH1F *> histograms;
	TFile * fhisto;
	TKey * keyHisto;
	if(useCumul) {
		if(enableVerbose) std::cout << "Reading " << cinput << " ... " << std::endl;
		fcumul = TFile::Open(cinput.c_str(), "read");
		if(fcumul -> IsZombie() || ! fcumul -> IsOpen()) {
			std::cerr << "Couldn't open file " << cinput << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(enableVerbose) std::cout << "Reading all cumulatives ... " << std::endl;
		TIter nextCumul(fcumul -> GetListOfKeys());
		
		while((keyCumul = dynamic_cast<TKey *>(nextCumul()))) {
			TClass * cl = gROOT -> GetClass(keyCumul -> GetClassName());
			if(! cl -> InheritsFrom("TH1F")) continue;
			TH1F * h = dynamic_cast<TH1F *> (keyCumul -> ReadObj());
			histoCumul[h -> GetName()] = h;
		}
	}
	else {
		if(enableVerbose) std::cout << "Reading " << hinput << " ... " << std::endl;
		fhisto = TFile::Open(hinput.c_str(), "read");
		if(fhisto -> IsZombie() || ! fhisto -> IsOpen()) {
			std::cerr << "Couldn't open file " << hinput << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(enableVerbose) std::cout << "Reading all histograms ... " << std::endl;
		TIter nextHisto(fhisto -> GetListOfKeys());
		
		while((keyHisto = dynamic_cast<TKey *>(nextHisto()))) {
			TClass * cl = gROOT -> GetClass(keyHisto -> GetClassName());
			if(! cl -> InheritsFrom("TH1F")) continue;
			TH1F * h = dynamic_cast<TH1F *> (keyHisto -> ReadObj());
			histograms[h -> GetName()] = h;
		}
	}
	// create the output file
	if(enableVerbose) std::cout << "Creating " << output << " ... " << std::endl;
	std::unique_ptr<TFile> out(new TFile(output.c_str(), "recreate"));
	TTree * u = new TTree(newtree.c_str(), "Tree with generated CSV values according to the histograms."); // output tree
	u -> SetDirectory(out.get());
	
	/******************************************************************************************************/
	// define some functions for the random number generation
	
	auto bruteSearch = [] (TH1F * h, Double_t r) {
		Int_t binMin = h -> GetMinimumBin(), binMax = h -> GetMaximumBin();
		Int_t bin = binMin;
		for( ; bin <= binMax; ++bin) {
			if(h -> GetBinContent(bin) > r) break;
		}
		return bin;
	};
	
	auto randLinpolEdge = [] (TH1F * h, Float_t r, Int_t (*search)(TH1F *h, Double_t r)) -> Float_t {
		Int_t bin = search(h, r);
		Float_t x1, y1, x2, y2;
		x1 = h -> GetBinLowEdge(bin);
		y1 = h -> GetBinContent(bin - 1);
		x2 = h -> GetBinLowEdge(bin + 1);
		y2 = h -> GetBinContent(bin);
		Float_t x = (r - y1) * (x2 - x1) / (y2 - y1) + x1;
		return x;
	};
	
	/******************************************************************************************************/
	
	// set up the variables
	// variables to be used are commented out for obv performance reasons
	if(enableVerbose) std::cout << "Setting up branch addresses ... " << std::endl;
	const int maxNumberOfHJets = 2;
	const int maxNumberOfAJets = 20;
	
	/******************************************************************************************************/
	
	// set up PRNG
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937_64 gen(seed);
	std::uniform_real_distribution<Float_t> dis(0,1);
	
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
			Float_t absFlavor = TMath::Abs(hJet_flavour[j]); // antiparticles included
			
			int flavorIndex = getFlavorIndex(absFlavor);
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
					for(; iterations <= maxSamples; ++iterations) {
						if(useCumul) {
							randomCSV = randLinpolEdge(histoCumul[key], dis(gen), bruteSearch);
						}
						else {
							randomCSV = histograms[key] -> GetRandom();
						}
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
					if(useCumul) {
						n_hJet_csvGen[j] = randLinpolEdge(histoCumul[key], dis(gen), bruteSearch);
					}
					else {
						n_hJet_csvGen[j] = histograms[key] -> GetRandom();
					}
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
			Float_t absFlavor = TMath::Abs(aJet_flavour[j]); // antiparticles included
			
			int flavorIndex = getFlavorIndex(absFlavor);
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
					for(iterations = 1; iterations <= maxSamples; ++iterations) {
						if(useCumul) {
							randomCSV = randLinpolEdge(histoCumul[key], dis(gen), bruteSearch);
						}
						else {
							randomCSV = histograms[key] -> GetRandom();
						}
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
					if(useCumul) {
						n_aJet_csvGen[j] = randLinpolEdge(histoCumul[key], dis(gen), bruteSearch);
					}
					else {
						n_aJet_csvGen[j] = histograms[key] -> GetRandom();
					}
				}
			}
		}
		
		u -> Fill();
		
		if(enableVerbose) ++(*show_progress);
	}
	
	if(enableVerbose) std::cout << "Writing to " << output << " ... " << std::endl;
	u -> Write();
	
	// close the files
	if(enableVerbose) {
		std::cout << "Closing " << input << ", " << output << ", ";
		std::cout << " and " << (useCumul ? cinput : hinput) << " ... " << std::endl;
	}
	in -> Close();
	out -> Close();
	if(useCumul) fcumul -> Close();
	else fhisto -> Close();
	
	return EXIT_SUCCESS;
}
