#include <cstdlib> // EXIT_SUCCESS
#include <string> // std::string
#include <map> // std::map<>
#include <sstream> // std::stringstream
#include <utility> // std::pair

#include <TFile.h>
#include <TTree.h>
#include <TString.h>
#include <TH1F.h>

std::string flavorStrings  [3] = 	{"c", "b", "l"};
std::string ptRangeStrings [6] = 	{"[20,30]", "[30,40]", "[40,60]", "[60,100]", "[100,160]", "[160,inf]"};
std::string etaRangeStrings[3] = 	{"[0,0.8]", "[0.8,1.6]", "[1.6,2.5]"};
std::string csvString = "csv";

int main(void){
	// set up vars
	std::string dir = "res/results";
	std::string prefix = "TTout_";
	std::string result = "TTresult";
	int min = 1;
	int max = 100;
	Int_t bins = 50;
	Float_t minCSV = 0.0, maxCSV = 1.0;
	
	// open files
	TFile * out = TFile::Open(result.append(".root").c_str(), "recreate");	
	std::map<std::string, TH1F *> histograms;
	for(int i = 0; i < 3; ++i) {
		for(int j = 0; j < 6; ++j) {
			for(int k = 0; k < 3; ++k) {
				std::string name = csvString;
				name += "_" + flavorStrings[i];
				name += "_" + ptRangeStrings[j];
				name += "_" + etaRangeStrings[k];
				histograms[name] = new TH1F(name.c_str(), name.c_str(), bins, minCSV, maxCSV);
				histograms.at(name) -> SetDirectory(out);
			}
		}
	}
	// add histograms
	for(int i = min; i <= max; ++i) {
		std::stringstream path;
		path << dir << "/" << prefix << i << ".root";
		TFile * in = TFile::Open(path.str().c_str(), "read");
		for(const auto & kv: histograms) {
			TH1F * tempHistogram;
			tempHistogram = dynamic_cast<TH1F *>(in -> Get(kv.first.c_str()));
			kv.second -> Add(tempHistogram);
		}
		in -> Close();
	}
	
	// write results
	out -> Write();
	out -> Close();
	
	return EXIT_SUCCESS;
}