#include <boost/program_options.hpp>

#include <cstdlib> // EXIT_SUCCESS, std::exit
#include <iostream> // std::cout, std::cerr, std::endl
#include <vector> // std::vector<>
#include <map> // std::map<>

#include <TFile.h>
#include <TH1F.h>
#include <TKey.h>
#include <TROOT.h>
#include <TClass.h>

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	
	std::string inFilename, outFilename;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,i", po::value<std::string>(&inFilename), "input *.root file")
			("output,o", po::value<std::string>(&outFilename), "output file name")
		;
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		
		if(vm.count("input") == 0 || vm.count("output") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS); // ugly
		}
	}
	catch(std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		std::exit(EXIT_FAILURE); // ugly
	}
	catch(...) {
		std::cerr << "exception of unkown type" << std::endl;
	}
	
	TFile * in = TFile::Open(inFilename.c_str(), "read");
	if(in -> IsZombie() || ! in -> IsOpen()) {
		std::cerr << "Couldn't open " << inFilename << "." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	TIter next(in -> GetListOfKeys());
	TKey * key;
	std::vector<TH1F *> histoVector;
	while((key = dynamic_cast<TKey *>(next()))) {
		TClass * cl = gROOT -> GetClass(key -> GetClassName());
		if(! cl -> InheritsFrom("TH1F")) continue;
		TH1F * h = dynamic_cast<TH1F *> (key -> ReadObj());
		histoVector.push_back(h);
	}
	
	// http://root.cern.ch/root/html/tutorials/hist/twoscales.C.html
	TFile * out = TFile::Open(outFilename.c_str(), "recreate");
	std::map<TString, TH1F *> cumulHistos;
	for(auto h: histoVector) {
		Int_t nBins = h -> GetNbinsX();
		TString name = h -> GetName();
		cumulHistos[name] = new TH1F(name, name, nBins + 1, 0, 1);
		cumulHistos[name] -> SetDirectory(out);
		Float_t total = 0;
		for(Int_t i = 0; i < nBins; ++i) {
			total += h -> GetBinContent(i);
			cumulHistos[name] -> SetBinContent(i, total);
		}
		cumulHistos[name] -> Scale(1.0 / h -> Integral());
		cumulHistos[name] -> Write();
	}
	
	in -> Close();
	out -> Close();
	
	return EXIT_SUCCESS;
}