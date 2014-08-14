#include <boost/program_options.hpp>

#include <cstdlib> // EXIT_SUCCESS, std::exit
#include <iostream> // std::cout, std::cerr, std::endl
#include <fstream> // std::ofstream
#include <streambuf> // std::streambuf

#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	
	std::string input, treeName, output;
	Long64_t beginEvent, endEvent;
	Int_t nBtags;
	Float_t CSVM;
	bool useAnalytical = false, useMultiple = false;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,i", po::value<std::string>(&input), "input *.root file")
			("begin,b", po::value<Long64_t>(&beginEvent) -> default_value(0), "the event number to start with")
			("end,e", po::value<Long64_t>(&endEvent) -> default_value(-1), "the event number to end with\ndefault (-1) means all events")
			("tree,t", po::value<std::string>(&treeName), "name of the tree (assumed to be common)")
			("output,o", po::value<std::string>(&output), "output filename")
			("working-point,w", po::value<Float_t>(&CSVM) -> default_value(0.679000000), "CSV working point")
			("nBtags,n", po::value<Int_t>(&nBtags), "number of b-tags")
			("use-analytical,a", "use analytical probabilities")
			("use-multiple,m", "use weights obtained by multiple sampling method")
		;
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		
		if(vm.count("help")) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS); // ugly
		}
		if(vm.count("input") == 0 || vm.count("tree") == 0 || vm.count("nBtags") == 0
			|| vm.count("output") == 0 || vm.count("working-point") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("use-analytical")) {
			useAnalytical = true;
		}
		if(vm.count("use-multiple")) {
			useMultiple = true;
		}
	}
	catch(std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		std::exit(EXIT_FAILURE); // ugly
	}
	catch(...) {
		std::cerr << "exception of unkown type" << std::endl;
	}
	
	if(!(useAnalytical || useMultiple)) {
		std::cerr << "you have to specify at least one of the following flags: -a -m" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	/************* open iteration file *************/
	TFile * inFile = TFile::Open(input.c_str(), "read");
	if(inFile -> IsZombie() || ! inFile -> IsOpen()) {
		std::cerr << "Couldn't open file " << input << " ..." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	TTree * t = dynamic_cast<TTree *> (inFile -> Get(treeName.c_str()));
	
	/*********** create two histograms **************/
	TFile * outFile = TFile::Open(output.c_str(), "recreate");
	if(outFile -> IsZombie() || ! outFile -> IsOpen()) {
		std::cerr << "Couldn't create file " << output << " ..." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	std::string hardCutTitle = "pt_hardCut", weightedTitle = "pt_weighted";
	Int_t nbins = 250;
	TH1F * hardCut = new TH1F(hardCutTitle.c_str(), hardCutTitle.c_str(), nbins, 0, nbins);
	hardCut -> SetDirectory(outFile);
	TH1F * weightedA = new TH1F((weightedTitle + "_A").c_str(), (weightedTitle + "_A").c_str(), nbins, 0, nbins);
	TH1F * weightedM = new TH1F((weightedTitle + "_M").c_str(), (weightedTitle + "_M").c_str(), nbins, 0, nbins);
	if(useAnalytical) {
		weightedA -> SetDirectory(outFile);
	}
	if(useMultiple) {
		weightedM -> SetDirectory(outFile);
	}
	
	Float_t btag_aProb;
	Float_t btag_mProb;
	Int_t btag_count;
	
	int maxNumberOfHJets = 2;
	int maxNumberOfAJets = 20;
	
	Int_t nhJets;
	Int_t naJets;
	
	Float_t hJet_pt[maxNumberOfHJets];
	Float_t aJet_pt[maxNumberOfAJets];
	
	Float_t hJet_csvGen[maxNumberOfHJets];
	Float_t aJet_csvGen[maxNumberOfAJets];
	
	if(useAnalytical) {
		t -> SetBranchAddress("btag_aProb", &btag_aProb);
	}
	if(useMultiple) {
		t -> SetBranchAddress("btag_mProb", &btag_mProb);
	}
	t -> SetBranchAddress("btag_count", &btag_count);
	t -> SetBranchAddress("nhJets", &nhJets);
	t -> SetBranchAddress("naJets", &naJets);
	t -> SetBranchAddress("hJet_pt", &hJet_pt);
	t -> SetBranchAddress("aJet_pt", &aJet_pt);
	t -> SetBranchAddress("hJet_csvGen", &hJet_csvGen);
	t -> SetBranchAddress("aJet_csvGen", &aJet_csvGen);
	
	endEvent = (endEvent == -1) ? t -> GetEntries() : endEvent;
	
	// loop over the events
	for(Int_t i = beginEvent; i < endEvent; ++i) {
		t -> GetEntry(i);
		if(btag_count != nBtags) continue; // skip the event if the number of btags doesn't coincide
		for(int j = 0; j < nhJets; ++j) {
			if(hJet_csvGen[j] >= CSVM) {
				hardCut -> Fill(hJet_pt[j]);
			}
			if(useAnalytical) {
				weightedA -> Fill(hJet_pt[j], btag_aProb);
			}
			if(useMultiple) {
				weightedM -> Fill(hJet_pt[j], btag_mProb);
			}
		}
		for(int j = 0; j < naJets; ++j) {
			if(aJet_csvGen[j] >= CSVM) {
				hardCut -> Fill(aJet_pt[j]);
			}
			if(useAnalytical) {
				weightedA -> Fill(aJet_pt[j], btag_aProb);
			}
			if(useMultiple) {
				weightedM -> Fill(aJet_pt[j], btag_mProb);
			}
		}
	}
	
	hardCut -> Write();
	if(useAnalytical) {
		weightedA -> Write();
	}
	if(useMultiple) {
		weightedM -> Write();
	}
	inFile -> Close();
	outFile -> Close();
	
	return EXIT_SUCCESS;
}