#include <boost/program_options.hpp>

#include <cstdlib> // EXIT_SUCCESS, std::exit
#include <iostream> // std::cout, std::cerr, std::endl
#include <fstream> // std::ofstream
#include <streambuf> // std::streambuf

#include <TFile.h>
#include <TTree.h>

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	
	std::string iterFile, btagFile, treeName, fileName;
	Long64_t beginEvent, endEvent;
	bool writeToFile = false;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("iteration-file,i", po::value<std::string>(&iterFile), "input *.root file")
			("btag-file,j", po::value<std::string>(&btagFile), "input *.root file")
			("begin,b", po::value<Long64_t>(&beginEvent) -> default_value(0), "the event number to start with")
			("end,e", po::value<Long64_t>(&endEvent) -> default_value(-1), "the event number to end with\ndefault (-1) means all events")
			("tree,t", po::value<std::string>(&treeName), "name of the tree (assumed to be common)")
			("file,f", po::value<std::string>(&fileName), "output filename")
		;
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		
		if(vm.count("help")) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS); // ugly
		}
		if(vm.count("iteration-file") == 0 || vm.count("btag-file") == 0 || vm.count("tree") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("file") > 0) {
			writeToFile = true;
		}
	}
	catch(std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		std::exit(EXIT_FAILURE); // ugly
	}
	catch(...) {
		std::cerr << "exception of unkown type" << std::endl;
	}
	
	/************* open iteration file *************/
	TFile * iter = TFile::Open(iterFile.c_str(), "read");
	if(iter -> IsZombie() || ! iter -> IsOpen()) {
		std::cerr << "Couldn't open file " << iterFile << " ..." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	TTree * t = dynamic_cast<TTree *> (iter -> Get(treeName.c_str()));
	/************* open btag file ******************/
	TFile * btag = TFile::Open(btagFile.c_str(), "read");
	if(btag -> IsZombie() || ! btag -> IsOpen()) {
		std::cerr << "Couldn't open file " << btagFile << " ..." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	TTree * u = dynamic_cast<TTree *> (btag -> Get(treeName.c_str()));
	
	Int_t btags;
	Float_t Jet_csvN;
	
	t -> SetBranchAddress("Jet_csvN", &Jet_csvN);
	u -> SetBranchAddress("btags", &btags);
	endEvent = (endEvent == -1) ? t -> GetEntries() : endEvent;
	
	if(t -> GetEntries() != u -> GetEntries()) {
		std::cerr << "The files must contain the same number of events." << std::endl;
		iter -> Close();
		btag -> Close();
	}
	
	Int_t btagCounter = 0;
	Float_t iterCounter = 0;
	// loop over the events
	for(Int_t i = beginEvent; i < endEvent; ++i) {
		t -> GetEntry(i);
		u -> GetEntry(i);
		btagCounter += btags;
		iterCounter += 1.0 / Jet_csvN;
	}
	
	iter -> Close();
	btag -> Close();
	
	std::streambuf * buf;
	std::ofstream of;
	if(writeToFile) {
		of.open(fileName);
		buf = of.rdbuf();
	}
	else {
		buf = std::cout.rdbuf();
	}
	std::ostream out(buf);
	out << "inverse sum of iterations: " << std::fixed << iterCounter << std::endl;
	out << "number of btags: " << std::fixed << btagCounter << std::endl;
	
	return EXIT_SUCCESS;
}