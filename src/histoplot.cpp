#include <cstdlib> // EXIT_SUCCESS
#include <algorithm> // std::max
#include <map> // std::map
#include <vector> // std::vector
#include <sstream>
#include <iostream>

#include <TCanvas.h>
#include <TFile.h>
#include <TH1F.h>
#include <TString.h>
#include <TStyle.h>
#include <TLegend.h>

#include "common.hpp"

/**
 * @todo
 *  - error bars
 *  - optional: save into root file
 *  - CL arguments (boost ptree and program_options)
 */

int main(void) {
	
	Int_t dimX = 800;
	Int_t dimY = 500;
	std::string extension = ".png";
	std::string inName = "res/TTresult";
	std::string outName = "TTresult_allFlavors";
	EColor colorRanges[3] = {kBlue, kRed, kGreen};
	
	TFile * in = TFile::Open(inName.append(".root").c_str(), "read");
	
	for(int j = 0; j < 6; ++j) {
		for(int k = 0; k < 3; ++k) {
			std::string canvasTitle = getAbbrName(j, k);
			TCanvas * c = new TCanvas(canvasTitle.c_str(), canvasTitle.c_str(), dimX, dimY);
			gStyle -> SetOptStat(kFALSE);
			TLegend * legend = new TLegend(0.70, 0.70, 0.85, 0.85);
			Float_t maxY = -1;
			// the line
			//        maxY = maxY < h -> GetMaximum() ? h -> GetMaximum() : maxY;
			// in the second loop doesn't work, must loop over first to get the max value
			// of the Y axis
			for(int i = 0; i < 3; ++i) {
				TH1F * h = dynamic_cast<TH1F *> (in -> Get(getName(i, j, k).c_str()));
				h -> Scale(1.0/(h -> Integral()));
				maxY = h -> GetMaximum() > maxY ? h -> GetMaximum() : maxY;
				delete h;
			}
			for(int i = 0; i < 3; ++i) {
				TH1F * h = dynamic_cast<TH1F *> (in -> Get(getName(i, j, k).c_str()));
				std::string legendLabel = flavorNames[i] + " jet";
				std::stringstream histoTitle;
				histoTitle << getHistoTitle(j, k) << " @ " << h -> GetNbinsX() << " bins";
				h -> SetLineColor(colorRanges[i]);
				h -> SetLineWidth(2);
				h -> GetXaxis() -> SetTitle("CSV discriminator");
				h -> GetYaxis() -> SetTitle("Relative number of events per bin");
				h -> Scale(1.0/(h -> Integral()));
				h -> SetMaximum(1.1 * maxY);
				h -> Draw((i == 0 ? "" : "same"));
				h -> SetTitle(histoTitle.str().c_str());
				legend -> AddEntry(h, legendLabel.c_str());
				c -> Modified();
				c -> Update();
			}
			legend -> Draw();
			c -> SaveAs(canvasTitle.append(extension).c_str());
			c -> Close();
			delete legend;
		}
	}
	
	in -> Close();
	return EXIT_SUCCESS;
}