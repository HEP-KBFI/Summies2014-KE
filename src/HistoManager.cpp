#include "HistoManager.h"
#include <iostream>

void HistoManager::initRanges() {
	ranges = input -> getRanges();
}
void HistoManager::createFile(TString option) {
	file = new TFile(input -> getHname() + ".root", option);
}
void HistoManager::cd() {
	file -> cd();
}
void HistoManager::process(std::shared_ptr<SingleFilePointer> & sfp) {
	// initialize some stuff
	TTree * tree = sfp -> getTree();
	Long64_t nEntries = tree -> GetEntries();
	for(auto & kv: ranges) {
		branches[kv.first] = 0;	
	}
	flavorVar = std::make_pair(input -> getFlavorVar(), 0);
	xval = std::make_pair(input -> getXval(), 0);
	// set up the references
	for(auto & kv: branches) {
		tree -> SetBranchAddress(kv.first, &(kv.second));
	}
	tree -> SetBranchAddress(flavorVar.first, &(flavorVar.second));
	tree -> SetBranchAddress(xval.first, &(xval.second));
	// set up the index map
	std::map<TString, int> rangeIndices;
	// loop over the events
	for(Long64_t i = 0; i < nEntries; ++i) {
		tree -> GetEntry(i);
		// ignore particles if not of interest or not in range
		if(! (input -> hasFlavor(flavorVar.second))) continue;
		bool doContinue = true;
		for(auto & kv: branches) {
			rangeIndices[kv.first] = getRangeIndex(kv.first, kv.second);
			doContinue = doContinue && (rangeIndices[kv.first] != -1);
		}
		if(! doContinue) continue;
		TString histoName = getHistoName(rangeIndices);
		TString histoTitle = getHistoTitle(rangeIndices);
		if(histograms.count(histoName) == 0) {
			histograms[histoName] = new TH1F(histoName, histoTitle,
											 input -> getBins(), input -> getMinX(), input -> getMaxX());
			histograms.at(histoName) -> SetDirectory(file);
			//histograms.at(histoName) -> Sumw2(); // enable bin errors; ugh.. only error bars remain
		}
		histograms[histoName] -> Fill(xval.second);
	}
}
void HistoManager::write() {
	for(auto & kv: histograms) {
		kv.second -> Write();
	}
}
void HistoManager::closeFile() {
	file -> Close();
}
TString HistoManager::getHistoName(std::map<TString, int> rangeIndices) {
	return getHistoString(rangeIndices, "_");;
}
TString HistoManager::getHistoTitle(std::map<TString, int> rangeIndices) {
	return getHistoString(rangeIndices, " ");
}
TString HistoManager::getHistoString(std::map<TString, int> rangeIndices, TString delim) {
	TString name = input -> getXname() + delim;
	name += input -> getFlavor(flavorVar.second);
	for(auto & kv: branches) {
		name += delim;
		int index = rangeIndices[kv.first]; // not -1
		auto it = ranges.at(kv.first).begin();
		std::advance(it, index);
		name += "[" + f2s(it -> first) + "," + f2s(it -> second) + "]";
	}
	return name;
}
TString HistoManager::f2s(Float_t f) {
	if(f == INF) return inf;
	std::string strf = std::to_string(f);
	return strf.substr(0, strf.find('.') + 2);
}
int HistoManager::getRangeIndex(TString name, Float_t val) const {
	auto range = ranges.at(name);
	int count = 0;
	for(auto & kv: range) {
		if(kv.first <= val && kv.second > val) return count;
		++count;
	}
	return -1;
}
