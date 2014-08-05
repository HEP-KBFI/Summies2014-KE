#include <boost/program_options.hpp>

#include <string> // std::string
#include <cstdlib> // EXIT_SUCCESS
#include <iostream> // std::cout, std::cerr, std::endl

#include <TFile.h>
#include <TTree.h>
#include <TString.h>

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	
	std::string inName, inTreeName, outName;
	Int_t nEntries;
	
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,I", po::value<std::string>(&inName), "input file name")
			("nEntries,n", po::value<Int_t>(&nEntries) -> default_value(-1), "number of entries to be copied")
			("out,o", po::value<std::string>(&outName), "output file name")
			("tree,t", po::value<std::string>(&inTreeName), "tree name")
		;
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		
		if(vm.count("help")) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS); // ugly
		}
		if(vm.count("input") == 0 || vm.count("out") == 0 || vm.count("tree") == 0) {
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
	
	TFile * in = TFile::Open(inName.c_str(), "read");
	if(in -> IsZombie() || ! in -> IsOpen()) {
		std::cerr << "error on opening " << inName << std::endl;
		std::exit(EXIT_FAILURE);
	}
	TTree * inTree = dynamic_cast<TTree *>(in -> Get(inTreeName.c_str()));
	
	inTree -> SetBranchStatus("*", 0);
	inTree -> SetBranchStatus("nhJets", 1);
	inTree -> SetBranchStatus("naJets", 1);
	inTree -> SetBranchStatus("hJet_pt", 1);
	inTree -> SetBranchStatus("hJet_eta", 1);
	inTree -> SetBranchStatus("hJet_csv", 1);
	inTree -> SetBranchStatus("hJet_flavour", 1);
	inTree -> SetBranchStatus("aJet_pt", 1);
	inTree -> SetBranchStatus("aJet_eta", 1);
	inTree -> SetBranchStatus("aJet_csv", 1);
	inTree -> SetBranchStatus("aJet_flavour", 1);
	inTree -> SetBranchStatus("nvlep", 1);
	inTree -> SetBranchStatus("nalep", 1);
	inTree -> SetBranchStatus("vLepton_pt", 1);
	inTree -> SetBranchStatus("aLepton_pt", 1);
	inTree -> SetBranchStatus("vLepton_eta", 1);
	inTree -> SetBranchStatus("aLepton_eta", 1);
	inTree -> SetBranchStatus("vLepton_pfCombRelIso", 1);
	inTree -> SetBranchStatus("aLepton_pfCombRelIso", 1);
	inTree -> SetBranchStatus("vLepton_type", 1);
	inTree -> SetBranchStatus("aLepton_type", 1);
	inTree -> SetBranchStatus("vLepton_id80", 1);
	inTree -> SetBranchStatus("aLepton_id80", 1);
	inTree -> SetBranchStatus("vLepton_id95", 1);
	inTree -> SetBranchStatus("aLepton_id95", 1);
	inTree -> SetBranchStatus("vLepton_charge", 1);
	inTree -> SetBranchStatus("aLepton_charge", 1);
	inTree -> SetBranchStatus("vLepton_idMVAtrig", 1);
	inTree -> SetBranchStatus("aLepton_idMVAtrig", 1);
	inTree -> SetBranchStatus("vLepton_idMVAnotrig", 1);
	inTree -> SetBranchStatus("aLepton_idMVAnotrig", 1);
	inTree -> SetBranchStatus("vLepton_idMVApresel", 1);
	inTree -> SetBranchStatus("aLepton_idMVApresel", 1);
	inTree -> SetBranchStatus("vLepton_particleIso", 1);
	inTree -> SetBranchStatus("aLepton_particleIso", 1);
	inTree -> SetBranchStatus("vLepton_dxy", 1);
	inTree -> SetBranchStatus("aLepton_dxy", 1);
	inTree -> SetBranchStatus("vLepton_innerHits", 1);
	inTree -> SetBranchStatus("aLepton_innerHits", 1);
	
	TFile * out = new TFile(outName.c_str(), "recreate");
	TTree * outTree = inTree -> CloneTree(nEntries);
	out -> Write();
	
	delete inTree;
	delete outTree;
	in -> Close();
	out -> Close();
	
	return EXIT_SUCCESS;
}