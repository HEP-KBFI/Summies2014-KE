#include <algorithm> // std::prev_permutation, std::transform
#include <iostream> // std::cout, std::endl
#include <string> // std::string
#include <chrono> // std::chrono
#include <random> // std::mt19937_64, std::uniform_real_distribution<>
#include <vector> // std::vector
#include <cstdlib> // EXIT_SUCCESS
#include <numeric> // std::accumulate
#include <map> // std::map

#include <TFile.h>
#include <TH1F.h>
#include <TKey.h>
#include <TROOT.h>
#include <TClass.h>
 
int main(void) {
	int Nj = 4, Ntag = 2;
	std::string cinput = "res/TTcsv_cumulatives.root";
	Float_t CSVM = 0.679;
	
	std::map<TString, TH1F *> cumulatives;
	std::map<Float_t, TString> probabilities;
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
		probabilities[1.0 - randLinpolEdge(kv.second, CSVM, bin)] = kv.first;
	}
	
	cumulativeFile -> Close();
	
	for(auto & kv: probabilities) {
		std::cout << std::fixed << kv.first << "\t" << kv.second << std::endl;
	}
	
	return EXIT_SUCCESS;
}