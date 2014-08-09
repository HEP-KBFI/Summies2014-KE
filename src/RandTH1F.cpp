#include <cstdlib> // EXIT_FAILURE, std::exit()
#include <iostream> // std::cout, std::endl
#include <vector> // std::vector<>
#include <random> // std::mt19937_64, std::uniform_real_distribution
#include <chrono> // std::chrono
#include <map> // std::map<>

#include <TFile.h>
#include <TH1F.h>
#include <TKey.h>
#include <TROOT.h>
#include <TClass.h>
#include <TMath.h>

#include "RandTH1F.hpp"

RandTH1F::RandTH1F(std::string cumulDistFilename)
	: cumulDistFilename(cumulDistFilename) { }
RandTH1F::~RandTH1F() {
	histoMap.clear();
	randomValues.clear();
	keys.clear();
	in.release();
}

void RandTH1F::openFile(void) {
	in = std::unique_ptr<TFile>(TFile::Open(cumulDistFilename.c_str(), "read"));
	if(in -> IsZombie() || ! in -> IsOpen()) {
		std::cerr << "Couldn't open file " << cumulDistFilename << std::endl;
		std::exit(EXIT_FAILURE);
	}
}
void RandTH1F::loadHistograms(void) {
	TIter next(in -> GetListOfKeys());
	TKey * key;
	while((key = dynamic_cast<TKey *>(next()))) {
		TClass * cl = gROOT -> GetClass(key -> GetClassName());
		if(! cl -> InheritsFrom("TH1F")) continue;
		std::shared_ptr<TH1F> temp(dynamic_cast<TH1F *> (key -> ReadObj()));
		TString id = temp -> GetName();
		histoMap[id] = temp;
		keys.push_back(id);
		delete cl;
	}
	//delete key;
}
void RandTH1F::generateRandom(TString histogramName, Int_t nEntries) {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937_64 gen(seed);
	std::uniform_real_distribution<Float_t> dis(0,1);
	for(int i = 0; i < nEntries; ++i) {
		Float_t r = randLinpolEdge(histoMap[histogramName], dis(gen), bruteSearch);
		randomValues[histogramName].push_back(r);
	}
}
bool RandTH1F::hasNext(TString histogramName) {
	return ! randomValues[histogramName].empty();
}
Float_t RandTH1F::getRandom(TString histogramName) {
	Float_t val = randomValues[histogramName].back();
	randomValues[histogramName].pop_back();
	return val;
}
std::vector<Float_t> & RandTH1F::getRandomVector(TString histogramName) {
	return randomValues[histogramName];
}
std::vector<TString> & RandTH1F::getKeys() {
	return keys;
}

Int_t RandTH1F::bruteSearch(std::shared_ptr<TH1F> h, Double_t r) {
	Int_t binMin = h -> GetMinimumBin(), binMax = h -> GetMaximumBin();
	Int_t bin = binMin;
	for( ; bin <= binMax; ++bin) {
		if(h -> GetBinContent(bin) > r) break;
	}
	return bin;
}
Float_t RandTH1F::randLinpolEdge(std::shared_ptr<TH1F> h, Float_t r,
								 Int_t (* search)(std::shared_ptr<TH1F> h, Double_t r)) {
	Int_t bin = search(h, r);
	Float_t x1, y1, x2, y2;
	x1 = h -> GetBinLowEdge(bin);
	y1 = h -> GetBinContent(bin - 1);
	x2 = h -> GetBinLowEdge(bin + 1);
	y2 = h -> GetBinContent(bin);
	Float_t x = (r - y1) * (x2 - x1) / (y2 - y1) + x1;
	return x;
}