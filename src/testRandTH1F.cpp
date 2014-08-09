#include <iostream> // std::cout, std::endl;
#include <map> // std::map<>

#include <TFile.h>
#include <TH1F.h>
#include <TKey.h>
#include <TROOT.h>
#include <TClass.h>
#include <TMath.h>

#include "RandTH1F.hpp"

int main() {
	/*
	RandTH1F r("res/TT_csv_cumulative.root");
	r.openFile();
	r.loadHistograms();
	auto keys = r.getKeys();
	*/
	std::map<TString, Int_t> nEntries;
	TFile * in = TFile::Open("res/TT_csv_histograms.root", "read");
	if(in -> IsZombie() || ! in -> IsOpen()) {
		std::cerr << "Couldn't open file " << "res/TT_csv_histograms.root" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	TIter next2(in -> GetListOfKeys());
	TKey * key2;
	while((key2 = dynamic_cast<TKey *>(next2()))) {
		TClass * cl = gROOT -> GetClass(key2 -> GetClassName());
		if(! cl -> InheritsFrom("TH1F")) continue;
		TH1F * temp = dynamic_cast<TH1F *> (key2 -> ReadObj());
		nEntries[temp -> GetName()] = temp -> Integral();
		//delete cl;
	}
	/*
	for(auto name: keys) {
		r.generateRandom(name, nEntries[name]);
		std::cout << name << std::endl;
	}*/
	return 0;
}