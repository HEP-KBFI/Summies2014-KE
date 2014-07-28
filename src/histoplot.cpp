#include <cstdlib> // EXIT_SUCCESS
#include <algorithm> // std::max
#include <map> // std::map
#include <vector> // std::vector

#include <TCanvas.h>
#include <TFile.h>
#include <TH1F.h>
#include <TString.h>

std::string flavorStrings  [3] = 	{"c", "b", "l"};
std::string ptRangeStrings [6] = 	{"[20,30]", "[30,40]", "[40,60]", "[60,100]", "[100,160]", "[160,inf]"};
std::string etaRangeStrings[3] = 	{"[0,0.8]", "[0.8,1.6]", "[1.6,2.5]"};

std::string csvString = "csv";
EColor colorRanges[3] = {kBlue, kRed, kGreen};

int main(void) {
	
	Int_t dimX = 600;
	Int_t dimY = 400;
	std::string extension = ".png";
	std::string inName = "TTresult";
	std::string outName = "TTresult_allFlavors";
	TFile * in = TFile::Open(inName.append(".root").c_str(), "read");
	
	for(int j = 0; j < 6; ++j) {
		for(int k = 0; k < 3; ++k) {
			std::string key = ptRangeStrings[j];
			key += "_";
			key += etaRangeStrings[k];

			std::string canvasTitle = ptRangeStrings[j];
			canvasTitle += "_";
			canvasTitle += etaRangeStrings[k];
			TCanvas * c = new TCanvas(canvasTitle.c_str(), canvasTitle.c_str(), dimX, dimY);
			
			for(int i = 0; i < 3; ++i) {
				std::string histoName = csvString;
				histoName += "_" + flavorStrings[i];
				histoName += "_" + ptRangeStrings[j];
				histoName += "_" + etaRangeStrings[k];
				TH1F * h = dynamic_cast<TH1F *> (in -> Get(histoName.c_str()));
				h -> SetLineColor(colorRanges[i]);
				h -> Scale(1.0/(h -> Integral()));
				h -> SetMaximum((h -> GetMaximum())*2);
				h -> Draw((i == 0 ? "" : "same"));
				c -> Modified();
				c -> Update();
			}
			c -> SaveAs(canvasTitle.append(extension).c_str());
			delete c;
			
		}
	}
	
	
	return EXIT_SUCCESS;
}