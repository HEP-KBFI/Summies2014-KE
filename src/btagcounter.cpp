#include <boost/program_options.hpp>

#include <cstdlib> // EXIT_SUCCESS, std::exit
#include <iostream> // std::cout, std::cerr, std::endl
#include <fstream> // std::ofstream
#include <streambuf> // std::streambuf

#include <TFile.h>
#include <TTree.h>

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	
	std::string input, treeName, output;
	Long64_t beginEvent, endEvent;
	bool writeToFile = false, useAnalytical = false, useMultiple = false;
	Int_t nBtags;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,i", po::value<std::string>(&input), "input *.root file")
			("begin,b", po::value<Long64_t>(&beginEvent) -> default_value(0), "the event number to start with")
			("end,e", po::value<Long64_t>(&endEvent) -> default_value(-1), "the event number to end with\ndefault (-1) means all events")
			("tree,t", po::value<std::string>(&treeName), "name of the tree (assumed to be common)")
			("file,f", po::value<std::string>(&output), "output filename")
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
		if(vm.count("input") == 0 || vm.count("tree") == 0 || vm.count("nBtags") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("file") > 0) {
			writeToFile = true;
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
	
	Float_t btag_aProb;
	Float_t btag_mProb;
	Int_t btag_count;
	
	if(useAnalytical) {
		t -> SetBranchAddress("btag_aProb", &btag_aProb);
	}
	if(useMultiple) {
		t -> SetBranchAddress("btag_mProb", &btag_mProb);
	}
	t -> SetBranchAddress("btag_count", &btag_count);
	
	endEvent = (endEvent == -1) ? t -> GetEntries() : endEvent;
	
	Float_t mProb = 0.0;
	Float_t aProb = 0.0;
	Int_t bcount = 0;
	// loop over the events
	for(Int_t i = beginEvent; i < endEvent; ++i) {
		t -> GetEntry(i);
		bcount += (nBtags == btag_count) ? 1 : 0;
		if(useAnalytical) {
			aProb += btag_aProb;
		}
		if(useMultiple) {
			mProb += btag_mProb;
		}
	}
	Int_t nEvents = endEvent - beginEvent;
	
	inFile -> Close();
	
	std::streambuf * buf;
	std::ofstream of;
	if(writeToFile) {
		of.open(output);
		buf = of.rdbuf();
	}
	else {
		buf = std::cout.rdbuf();
	}
	std::ostream out(buf);
	out << "number of events that passed the cut:\t" << nEvents << std::endl;
	out << "sum of " << nBtags << " b-tagged jets:\t\t\t" << bcount << std::endl;
	if(useAnalytical) {
		out << "sum of analytic probabilities:\t\t" << std::fixed << aProb << std::endl;
	} if(useMultiple) {
		out << "sum of multisample weights:\t\t" << std::fixed << mProb << std::endl;
	}
	
	return EXIT_SUCCESS;
}