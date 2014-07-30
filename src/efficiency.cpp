#include <cstdlib> // EXIT_SUCCESS
#include <string> // std::string
#include <vector> // std::vector<>
#include <fstream> // std::ofstream
#include <iostream> // std::cout
#include <streambuf> // std::streambuf
#include <map> //std::map
#include <sstream> // std::stringstream

#include <TH1F.h>
#include <TFile.h>
#include <TString.h>
#include <TCanvas.h>
#include <TGraphErrors.h>
#include <TMultiGraph.h>
#include <TLegend.h>

#include "common.hpp"

int main(void) {
	std::string inFilename = "res/TTresults.root";
	std::string outDir = "plots";
	std::string ext = "png";
	TFile * in = TFile::Open(inFilename.c_str());
	Int_t dimx = 900, dimy = 600;
	
	std::vector<Double_t> threshold;
	Double_t thresholdStart = 0.02;
	Double_t thresholdStep = 0.02;
	while(thresholdStart < 1.0) {
		threshold.push_back(thresholdStart);
		thresholdStart += thresholdStep;
	}
	std::vector<Double_t> thresholdErrors(threshold.size(), 0.0);
	
	std::string csvString = "CSV cut,c jet,c jet error,b jet,b jet error,light jet, light jet error";
	//bool printToFile = true;
	
	for(int j = 0; j < 6; ++j) {
		for(int k = 0; k < 3; ++k) {
			std::map<std::string, std::vector<Double_t> > vals;
			Int_t nbins;
			for(auto th: threshold){
				for(int i = 0; i < 3; ++i) {
					TH1F * h = dynamic_cast<TH1F *> (in -> Get(getName(i, j, k).c_str()));
					h -> Scale(1.0 / h -> Integral());
					Int_t binIndex = h -> FindBin(th);
					//Double_t intBelow = h -> Integral(1, binIndex - 1);
					Double_t error;
					nbins = h -> GetNbinsX();
					Double_t integralAbove = h -> IntegralAndError(binIndex, nbins, error);
					std::string key = flavorStrings[i];
					std::string key_error = key + "_error";
					vals[key].push_back(integralAbove);
					vals[key_error].push_back(error);
				}
			}
			TCanvas * c = new TCanvas(getAbbrName(j, k).c_str(), getAbbrName(j, k).c_str(), dimx, dimy);
			TLegend * legend = new TLegend(0.78, 0.76, 0.90, 0.90);
			c -> SetGrid();
			TMultiGraph * mg = new TMultiGraph();
			for(int i = 0; i < 3; ++i) {
				std::string key = flavorStrings[i];
				std::string key_error = key + "_error";				
				TGraphErrors * gr = new TGraphErrors(threshold.size(), &threshold[0], &(vals[key])[0],
													 &thresholdErrors[0], &(vals[key_error])[0]);
				legend -> AddEntry(gr, std::string(flavorNames[i] + " jet").c_str(), "p");
				gr -> SetMarkerColor(colorRanges[i]);
				gr -> SetMarkerStyle(20);
				gr -> SetMarkerSize(0.65);
				mg -> Add(gr);
			}
			std::stringstream mgTitle;
			mgTitle << getHistoTitle(j, k) << " @ " << nbins << " bins";
			mg -> Draw("ap");
			mg -> GetXaxis() -> SetLimits(0.0, 1.0);
			mg -> GetXaxis() -> SetTitle("CSV discriminator");
			mg -> GetYaxis() -> SetTitle("Efficiency");
			mg -> GetYaxis() -> SetTitleOffset(0.8);
			mg -> SetMinimum(0.0);
			mg -> SetMaximum(1.02);
			mg -> GetHistogram() -> SetTitle(mgTitle.str().c_str());
			c -> Update();
			legend -> Draw();
			std::string saveLocation = outDir + "/effs_" + getAbbrName(j, k) + "." + ext;
			c -> SaveAs(saveLocation.c_str());
			c -> Close();
			delete legend;
			delete mg;
			/*
			std::streambuf * buf;
			std::ofstream of;
			if(printToFile) {
				of.open(outDir + "/" + getAbbrName(j, k) + ".csv");
				buf = of.rdbuf();
			}
			else {
				buf = std::cout.rdbuf();
			}
			std::ostream out(buf);
			out << csvString << std::endl;
			out << std::fixed;
			
			for(auto th: threshold){
				out << th << ",";
				for(int i = 0; i < 3; ++i) {
					TH1F * h = dynamic_cast<TH1F *> (in -> Get(getName(i, j, k).c_str()));
					h -> Scale(1.0 / h -> Integral());
					Int_t binIndex = h -> FindBin(th);
					//Double_t intBelow = h -> Integral(1, binIndex - 1);
					
					Double_t error;
					Double_t integralAbove = h -> IntegralAndError(binIndex, h -> GetNbinsX(), error);
					out << integralAbove << "," << error;
					if(i == 2) 	out << std::endl;
					else		out << ",";
				}
			}
			*/
		}
	}
	return EXIT_SUCCESS;
}