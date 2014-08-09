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

int main(void) {
	
	std::string inFilename = "out.root"; // cumulative distributions
	std::string secondFile = "res/TT_csv_histograms.root"; // CSV pdfs
	TFile * in = TFile::Open(inFilename.c_str(), "read");
	if(in -> IsZombie() || ! in -> IsOpen()) {
		std::cerr << "Couldn't open file " << inFilename << std::endl;
		std::exit(EXIT_FAILURE);
	}
	TFile * comp = TFile::Open(secondFile.c_str(), "read");
	if(comp -> IsZombie() || ! comp -> IsOpen()) {
		std::cerr << "Couldn't open file " << secondFile << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	// loop over cumul distr
	TIter next(in -> GetListOfKeys());
	TKey * key;
	std::vector<TH1F *> histoVector;
	while((key = dynamic_cast<TKey *>(next()))) {
		TClass * cl = gROOT -> GetClass(key -> GetClassName());
		if(! cl -> InheritsFrom("TH1F")) continue;
		TH1F * h = dynamic_cast<TH1F *> (key -> ReadObj());
		histoVector.push_back(h);
	}
	TIter next2(comp -> GetListOfKeys());
	TKey * key2;
	std::vector<TH1F *> histoVector2;
	std::map<TString, Int_t> integrals;
	while((key2 = dynamic_cast<TKey *>(next2()))) {
		TClass * cl = gROOT -> GetClass(key2 -> GetClassName());
		if(! cl -> InheritsFrom("TH1F")) continue;
		TH1F * h = dynamic_cast<TH1F *> (key2 -> ReadObj());
		integrals[h -> GetName()] = h -> Integral();
		histoVector2.push_back(h);
	}
	
	// define functions to get csv value from cumul distr
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
	
	auto randLinpol = [] (TH1F * h, Float_t r, Int_t (*search)(TH1F *h, Double_t r)) -> Float_t {
		Int_t bin = search(h, r);
		Float_t x1, y1, x2, y2;
		if(r <= h -> GetBinContent(1)) {
			x1 = h -> GetBinCenter(1) - h -> GetBinWidth(1);
			y1 = 0.0;
			x2 = h -> GetBinCenter(1);
			y2 = h -> GetBinContent(1);
		}
		else if(r >= h -> GetBinContent(h -> GetNbinsX() - 1)) {
			x1 = h -> GetBinCenter(h -> GetNbinsX() - 1);
			y1 = h -> GetBinContent(h -> GetNbinsX() - 1);
			x2 = h -> GetBinCenter(h -> GetNbinsX());
			y2 = h -> GetBinContent(h -> GetNbinsX());
		}
		else {
			if(r <= h -> GetBinContent(bin)) {
				x1 = h -> GetBinCenter(bin - 1);
				y1 = h -> GetBinContent(bin - 1);
				x2 = h -> GetBinCenter(bin);
				y2 = h -> GetBinContent(bin);
			}
			else {
				x1 = h -> GetBinCenter(bin);
				y1 = h -> GetBinContent(bin);
				x2 = h -> GetBinCenter(bin + 1);
				y2 = h -> GetBinContent(bin + 1);
			}
		}
		
		Float_t x = (r - y1) * (x2 - x1) / (y2 - y1) + x1;
		return x;
	};
	
	
	
	std::string outFilename = "scumul.root";
	TFile * out = TFile::Open(outFilename.c_str(), "recreate");
	std::map<TString, TH1F *> outsamples;
	for(auto & h: histoVector2) {
		TString name = h -> GetName();
		outsamples[name] = new TH1F(name, name, h -> GetNbinsX(), 0, 1);
		outsamples[name] -> SetDirectory(out);
	}
	
	for(auto & h: histoVector) {
		unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::mt19937_64 gen(seed);
		std::uniform_real_distribution<Float_t> dis(0,1);
		TString name = h -> GetName();
		std::cout << name << std::endl;
		Int_t maxIter = integrals[name]; // assuming they're not normalized to one
		
		for(int i = 0; i < maxIter; ++i) {
			Float_t r = randLinpolEdge(h, dis(gen), bruteSearch);
			outsamples[name] -> Fill(r);
		}
	}
	
	for(auto & kv: outsamples) {
		kv.second -> Write();
	}
	
	for(auto & h: histoVector2) {
		TString name = h -> GetName();
		h -> Scale(float(1e5) / h -> Integral());
		outsamples[name] -> Scale(float(1e5) / outsamples[name] -> Integral());
		std::cout << name << ": ";
		std::cout << h -> KolmogorovTest(outsamples[name]) << std::endl;
	}
	
	in -> Close();
	comp -> Close();
	out -> Close();
	
	return EXIT_SUCCESS;
}