#include <cstdlib> // EXIT_SUCCESS
#include <algorithm> // std::max
#include <map> // std::map
#include <vector> // std::vector
#include <iostream>

#include <TCanvas.h>
#include <TFile.h>
#include <TH1F.h>
#include <TString.h>

#include "common.h"

/**
 * @todo
 *  - title
 *  - legend
 *  - axis labels
 *  - thick lines
 *  - error bars
 *  - optional: sav into root file
 *  - CL arguments (boost ptree and program_options)
 */

int main(void) {
	
	Int_t dimX = 800;
	Int_t dimY = 500;
	std::string extension = ".png";
	std::string inName = "res/TTresult";
	std::string outName = "TTresult_allFlavors";
	std::string csvString = "csv";
	EColor colorRanges[3] = {kBlue, kRed, kGreen};
	
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