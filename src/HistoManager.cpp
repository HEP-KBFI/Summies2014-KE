#include "HistoManager.h"

void HistoManager::initRanges() {
	variableRanges = input -> getVariableRanges();
}
void HistoManager::createFile(TString option) {
	file = std::unique_ptr<TFile>(new TFile(input -> getHistogramName() + ".root", option));
}
void HistoManager::cd() {
	file -> cd();
}
void HistoManager::process(std::shared_ptr<SingleFilePointer> & sfp) {
	// initialize some stuff
	TTree * tree = sfp -> getTree();
	for(const auto & kv: variableRanges) {
		branchReferences[kv.first] = 0;	
	}
	flavorVar = std::make_pair(input -> getFlavorVariableName(), 0);
	xVar      = std::make_pair(input -> getXVariableName(), 0);
	
	// set up the references
	for(auto & kv: branchReferences) {
		tree -> SetBranchAddress(kv.first, &(kv.second));
	}
	tree -> SetBranchAddress(flavorVar.first, &(flavorVar.second));
	tree -> SetBranchAddress(xVar.first, &(xVar.second));
	
	std::map<TString, int> rangeIndices; // set up the index map
	
	// loop over the events
	Long64_t nEntries = tree -> GetEntries();
	for(Long64_t i = 0; i < nEntries; ++i) {
		tree -> GetEntry(i);
		
		// ignore particles if not of interest or not in range
		if(! (input -> hasFlavor(flavorVar.second))) continue;
		bool doContinue = true;
		for(auto & kv: branchReferences) {
			rangeIndices[kv.first] = getRangeIndex(kv.first, kv.second);
			doContinue = doContinue && (rangeIndices[kv.first] != -1);
		}
		if(! doContinue) continue;
		
		// fill the histogram
		TString histogramName = getHistoName(rangeIndices);
		TString histogramTitle = getHistoTitle(rangeIndices);
		if(histograms.count(histogramName) == 0) {
			histograms[histogramName] = std::unique_ptr<TH1F>(new TH1F(
					histogramName, histogramTitle, input -> getBins(),
					input -> getMinX(), input -> getMaxX()
				));
			histograms.at(histogramName) -> SetDirectory(file.get());
		}
		histograms[histogramName] -> Fill(xVar.second);
	}
}
void HistoManager::write() {
	for(auto & kv: histograms) {
		kv.second -> Write();
	}
}
void HistoManager::close() {
	input.reset(); // not necessary, actually
	variableRanges.clear();
	flavorVar.first = "";
	flavorVar.second = 0;
	xVar.first = "";
	xVar.second = 0;
	histograms.clear();
	branchReferences.clear();
	file -> Close();
	file.reset();
}
TString HistoManager::getHistoName(std::map<TString, int> rangeIndices) {
	return getHistoString(rangeIndices, "_");;
}
TString HistoManager::getHistoTitle(std::map<TString, int> rangeIndices) {
	return getHistoString(rangeIndices, " ");
}
TString HistoManager::getHistoString(std::map<TString, int> rangeIndices, TString delim) {
	TString name = input -> getXTitleName() + delim;
	name += input -> getFlavor(flavorVar.second);
	
	for(const auto & kv: branchReferences) {
		name += delim;
		int index = rangeIndices[kv.first]; // not -1
		auto it = variableRanges.at(kv.first).begin();
		std::advance(it, index);
		//if(kv.first == "f_lept1_eta") std::cout << f2s(it -> second) << std::endl;
		name += (input -> getVariableAlias(kv.first)) + "=";
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
	auto range = variableRanges.at(name);
	int count = 0;
	for(const auto & kv: range) {
		if(kv.first <= val && kv.second > val) return count;
		++count;
	}
	return -1;
}
