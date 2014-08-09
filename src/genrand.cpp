#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <boost/timer.hpp>

#include <cstdlib> // EXIT_SUCCESS, std::exit()
#include <iostream> // std::cout, std::endl
#include <vector> // std::vector<>
#include <random> // std::mt19937_64
#include <chrono> // std::chrono
#include <map> // std::map<>

#include <TFile.h>
#include <TH1F.h>
#include <TKey.h>
#include <TROOT.h>
#include <TClass.h>
#include <TMath.h>

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	
	std::string cumulFilename; // cumulative distributions
	std::string histoFilename; // CSV pdfs
	std::string outFilename; // output filename
	bool enableVerbose = true;
	
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("cumulative,i", po::value<std::string>(&cumulFilename), "cumulative distribution")
			("histogram,j", po::value<std::string>(&histoFilename), "histograms")
			("output,o", po::value<std::string>(&outFilename), "output")
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
		if(vm.count("histogram") == 0 || vm.count("cumulative") == 0 || vm.count("output") == 0) {
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
	
	/************************ open files ******************************/
	
	TFile * in = TFile::Open(cumulFilename.c_str(), "read");
	if(in -> IsZombie() || ! in -> IsOpen()) {
		std::cerr << "Couldn't open file " << cumulFilename << std::endl;
		std::exit(EXIT_FAILURE);
	}
	TFile * comp = TFile::Open(histoFilename.c_str(), "read");
	if(comp -> IsZombie() || ! comp -> IsOpen()) {
		std::cerr << "Couldn't open file " << histoFilename << std::endl;
		std::exit(EXIT_FAILURE);
	}
	TFile * out = TFile::Open(outFilename.c_str(), "recreate");
	
	/*********************** obtain histograms ***********************/
	TIter nextCumul(in -> GetListOfKeys());
	TKey * keyCumul;
	std::vector<TH1F *> histoCumul;
	while((keyCumul = dynamic_cast<TKey *>(nextCumul()))) {
		TClass * cl = gROOT -> GetClass(keyCumul -> GetClassName());
		if(! cl -> InheritsFrom("TH1F")) continue;
		TH1F * h = dynamic_cast<TH1F *> (keyCumul -> ReadObj());
		histoCumul.push_back(h);
	}
	TIter nextHisto(comp -> GetListOfKeys());
	TKey * keyHisto;
	std::vector<TH1F *> histoHisto;
	std::map<TString, Int_t> integrals;
	while((keyHisto = dynamic_cast<TKey *>(nextHisto()))) {
		TClass * cl = gROOT -> GetClass(keyHisto -> GetClassName());
		if(! cl -> InheritsFrom("TH1F")) continue;
		TH1F * h = dynamic_cast<TH1F *> (keyHisto -> ReadObj());
		integrals[h -> GetName()] = h -> Integral();
		histoHisto.push_back(h);
	}
	std::map<TString, TH1F *> outsamples;
	for(auto & h: histoHisto) {
		TString name = h -> GetName();
		outsamples[name] = new TH1F(name, name, h -> GetNbinsX(), 0, 1);
		outsamples[name] -> SetDirectory(out);
	}
	
	/************************ define lambdas ***********************/
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
	
	/******************* sample *************************/
	
	boost::progress_display * show_progress;
	if(enableVerbose) {
		std::cout << "Looping over " << histoCumul.size() << " histograms ... " << std::endl;
		show_progress = new boost::progress_display(histoCumul.size());
	}
	
	for(auto & h: histoCumul) {
		unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::mt19937_64 gen(seed);
		std::uniform_real_distribution<Float_t> dis(0,1);
		TString name = h -> GetName();
		Int_t maxIter = integrals[name]; // assuming they're not normalized to one
		
		for(int i = 0; i < maxIter; ++i) {
			Float_t r = randLinpolEdge(h, dis(gen), bruteSearch);
			outsamples[name] -> Fill(r);
		}
		if(enableVerbose) ++(*show_progress);
	}
	
	/****************** write them **********************/
	
	for(auto & kv: outsamples) {
		kv.second -> Write();
	}
	
	/*************** close everything ****************/
	
	in -> Close();
	comp -> Close();
	out -> Close();
	
	return EXIT_SUCCESS;
}