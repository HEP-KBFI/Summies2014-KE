#include <cstdlib> // EXIT_SUCCESS
#include <algorithm> // std::max
#include <map> // std::map
#include <vector> // std::vector
#include <iostream>

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
	
	Int_t dimX = 800;
	Int_t dimY = 500;
	std::string extension = ".png";
	std::string inName = "TTresult";
	std::string outName = "TTresult_allFlavors";
	TFile * in = TFile::Open(inName.append(".root").c_str(), "read");
	
	auto getName = [] (int i, int j, int k) -> std::string {
		std::string histoName = csvString;
		histoName += "_" + flavorStrings[i];
		histoName += "_" + ptRangeStrings[j];
		histoName += "_" + etaRangeStrings[k];
		return histoName;
	};
	
	for(int j = 0; j < 6; ++j) {
		for(int k = 0; k < 3; ++k) {
			std::string key = ptRangeStrings[j];
			key += "_";
			key += etaRangeStrings[k];

			std::string canvasTitle = ptRangeStrings[j];
			canvasTitle += "_";
			canvasTitle += etaRangeStrings[k];
			TCanvas * c = new TCanvas(canvasTitle.c_str(), canvasTitle.c_str(), dimX, dimY);
			
			Float_t maxY = -1;
			// the line
			//        maxY = maxY < h -> GetMaximum() ? h -> GetMaximum() : maxY;
			// in the second loop doesn't work, must loop over first to get the max value
			// of the Y axis
			for(int i = 0; i < 3; ++i) {
				std::string histoName = getName(i, j, k);
				TH1F * h = dynamic_cast<TH1F *> (in -> Get(histoName.c_str()));
				h -> Scale(1.0/(h -> Integral()));
				maxY = h -> GetMaximum() > maxY ? h -> GetMaximum() : maxY;
				delete h;
			}
			for(int i = 0; i < 3; ++i) {
				std::string histoName = getName(i, j, k);
				TH1F * h = dynamic_cast<TH1F *> (in -> Get(histoName.c_str()));
				h -> SetLineColor(colorRanges[i]);
				h -> Scale(1.0/(h -> Integral()));
				h -> SetMaximum(1.1 * maxY);
				h -> Draw((i == 0 ? "" : "same"));
				c -> Modified();
				c -> Update();
			}
			c -> SaveAs(canvasTitle.append(extension).c_str());
			c -> Close();
		}
	}
	
	in -> Close();
	return EXIT_SUCCESS;
}