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
	bool useAnalytic = false, useMultiple = false, useRealCSV = false;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,i", po::value<std::string>(&input), "input *.root file")
			("begin,b", po::value<Long64_t>(&beginEvent) -> default_value(0), "the event number to start with")
			("end,e", po::value<Long64_t>(&endEvent) -> default_value(-1), "the event number to end with\ndefault (-1) means all events")
			("tree,t", po::value<std::string>(&treeName), "name of the tree (assumed to be common)")
			("output,o", po::value<std::string>(&output), "output filename")
			("nBtags,n", po::value<Int_t>(&nBtags), "number of b-tags")
			("use-analytical,a", "use analytical probabilities")
			("use-multiple,m", "use weights obtained by multiple sampling method")
			("use-real-csv,r", "use real CSV")
		;
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		
		if(vm.count("help")) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS); // ugly
		}
		if(vm.count("input") == 0 || vm.count("tree") == 0 || vm.count("nBtags") == 0 || vm.count("output") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("use-analytical") > 0) {
			useAnalytic = true;
		}
		if(vm.count("use-multiple") > 0) {
			useMultiple = true;
		}
		if(vm.count("use-real-csv") > 0) {
			useRealCSV = true;
		}
	}
	catch(std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		std::exit(EXIT_FAILURE); // ugly
	}
	catch(...) {
		std::cerr << "exception of unkown type" << std::endl;
	}
	
	if(!(useAnalytic || useMultiple)) {
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
	
	std::string hardCutTitle = "hardcut", weightedTitle = "weighted";
	std::string ptString = "pt", etaString = "eta", csvString = "csv";
	Int_t nbins = 50;
	
	TH1F * pt_hardCut = new TH1F(ptString.c_str(), (ptString + " H").c_str(), nbins, 0.0, 250);
	TH1F * eta_hardCut = new TH1F(etaString.c_str(), (etaString + " H").c_str(), nbins, -3.0, 3.0);
	TH1F * csv_hardCut = new TH1F(csvString.c_str(), (csvString + " H").c_str(), nbins, 0.0, 1.0);
	
	TH1F * pt_weightedA = new TH1F(ptString.c_str(), (ptString + " A").c_str(), nbins, 0.0, 250);
	TH1F * eta_weightedA = new TH1F(etaString.c_str(), (etaString + " A").c_str(), nbins, -3.0, 3.0);
	TH1F * csv_weightedA = new TH1F(csvString.c_str(), (csvString + " A").c_str(), nbins, 0.0, 1.0);
	
	TH1F * pt_weightedM = new TH1F(ptString.c_str(), (ptString + " M").c_str(), nbins, 0.0, 250);
	TH1F * eta_weightedM = new TH1F(etaString.c_str(), (etaString + " M").c_str(), nbins, -3.0, 3.0);
	TH1F * csv_weightedM = new TH1F(csvString.c_str(), (csvString + " M").c_str(), nbins, 0.0, 1.0);
	
	TH1F * pt_hardCutR = new TH1F(ptString.c_str(), (ptString + " R").c_str(), nbins, 0.0, 250);
	TH1F * eta_hardCutR = new TH1F(etaString.c_str(), (etaString + " R").c_str(), nbins, -3.0, 3.0);
	TH1F * csv_hardCutR = new TH1F(csvString.c_str(), (csvString + " R").c_str(), nbins, 0.0, 1.0);
	
	pt_hardCut -> SetDirectory(outFile);
	eta_hardCut -> SetDirectory(outFile);
	csv_hardCut -> SetDirectory(outFile);
	if(useAnalytic) {
		pt_weightedA -> SetDirectory(outFile);
		eta_weightedA -> SetDirectory(outFile);
		csv_weightedA -> SetDirectory(outFile);
	}
	if(useMultiple) {
		pt_weightedM -> SetDirectory(outFile);
		eta_weightedM -> SetDirectory(outFile);
		csv_weightedM -> SetDirectory(outFile);
	}
	if(useRealCSV) {
		pt_hardCutR -> SetDirectory(outFile);
		eta_hardCutR -> SetDirectory(outFile);
		csv_hardCutR -> SetDirectory(outFile);
	}
	
	Float_t btag_aProb;
	Float_t btag_mProb;
	Int_t btag_count;
	Int_t btag_real_count;
	
	int maxNumberOfHJets = 2;
	int maxNumberOfAJets = 20;
	
	Int_t nhJets;
	Int_t naJets;
	
	Float_t hJet_pt[maxNumberOfHJets];
	Float_t aJet_pt[maxNumberOfAJets];
	
	Float_t hJet_eta[maxNumberOfHJets];
	Float_t aJet_eta[maxNumberOfAJets];
	
	Float_t hJet_csv[maxNumberOfHJets];
	Float_t aJet_csv[maxNumberOfAJets];
	
	Float_t hJet_csvGen[maxNumberOfHJets];
	Float_t aJet_csvGen[maxNumberOfAJets];
	
	if(useAnalytic) {
		t -> SetBranchAddress("btag_aProb", &btag_aProb);
	}
	if(useMultiple) {
		t -> SetBranchAddress("btag_mProb", &btag_mProb);
	}
	if(useRealCSV) {
		t -> SetBranchAddress("btag_real_count", &btag_real_count);
	}
	t -> SetBranchAddress("btag_count", &btag_count);
	t -> SetBranchAddress("nhJets", &nhJets);
	t -> SetBranchAddress("naJets", &naJets);
	t -> SetBranchAddress("hJet_pt", &hJet_pt);
	t -> SetBranchAddress("aJet_pt", &aJet_pt);
	t -> SetBranchAddress("hJet_eta", &hJet_eta);
	t -> SetBranchAddress("aJet_eta", &aJet_eta);
	t -> SetBranchAddress("hJet_csv", &hJet_csv);
	t -> SetBranchAddress("aJet_csv", &aJet_csv);
	t -> SetBranchAddress("hJet_csvGen", &hJet_csvGen);
	t -> SetBranchAddress("aJet_csvGen", &aJet_csvGen);
	
	endEvent = (endEvent == -1) ? t -> GetEntries() : endEvent;
	
	// loop over the events
	for(Int_t i = beginEvent; i < endEvent; ++i) {
		t -> GetEntry(i);
		
		for(int j = 0; j < nhJets; ++j) {
			if(btag_count == nBtags) {
				pt_hardCut -> Fill(hJet_pt[j]);
				eta_hardCut -> Fill(hJet_eta[j]);
				csv_hardCut -> Fill(hJet_csv[j]);
			}
			if(useAnalytic) {
				pt_weightedA -> Fill(hJet_pt[j], btag_aProb);
				eta_weightedA -> Fill(hJet_eta[j], btag_aProb);
				csv_weightedA -> Fill(hJet_csv[j], btag_aProb);
			}
			if(useMultiple) {
				pt_weightedM -> Fill(hJet_pt[j], btag_mProb);
				eta_weightedM -> Fill(hJet_eta[j], btag_mProb);
				csv_weightedM -> Fill(hJet_csv[j], btag_mProb);
			}
			if(btag_real_count == nBtags) {
				pt_hardCutR -> Fill(hJet_pt[j]);
				eta_hardCutR -> Fill(hJet_eta[j]);
				csv_hardCutR -> Fill(hJet_csv[j]);
			}
		}
		for(int j = 0; j < naJets; ++j) {
			if(btag_count == nBtags) {
				pt_hardCut -> Fill(aJet_pt[j]);
				eta_hardCut -> Fill(aJet_eta[j]);
				csv_hardCut -> Fill(aJet_csv[j]);
			}
			if(useAnalytic) {
				pt_weightedA -> Fill(aJet_pt[j], btag_aProb);
				eta_weightedA -> Fill(aJet_eta[j], btag_aProb);
				csv_weightedA -> Fill(aJet_csv[j], btag_aProb);
			}
			if(useMultiple) {
				pt_weightedM -> Fill(aJet_pt[j], btag_mProb);
				eta_weightedM -> Fill(aJet_eta[j], btag_mProb);
				csv_weightedM -> Fill(aJet_csv[j], btag_mProb);
			}
			if(btag_real_count == nBtags) {
				pt_hardCutR -> Fill(aJet_pt[j]);
				eta_hardCutR -> Fill(aJet_eta[j]);
				csv_hardCutR -> Fill(aJet_csv[j]);
			}
		}
	}
	
	/************ write the histograms *****************/
	pt_hardCut -> Write();
	if(useAnalytic) pt_weightedA -> Write();
	if(useMultiple) pt_weightedM -> Write();
	if(useRealCSV)	pt_hardCutR -> Write();
	eta_hardCut -> Write();
	if(useAnalytic) eta_weightedA -> Write();
	if(useMultiple) eta_weightedM -> Write();
	if(useRealCSV)	eta_hardCutR -> Write();
	csv_hardCut -> Write();
	if(useAnalytic) csv_weightedA -> Write();
	if(useMultiple) csv_weightedM -> Write();
	if(useRealCSV)	csv_hardCutR -> Write();
	
	inFile -> Close();
	outFile -> Close();
	
	return EXIT_SUCCESS;
}