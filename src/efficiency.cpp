#include <cstdlib> // EXIT_SUCCESS
#include <string> // std::string
#include <vector> // std::vector<>
#include <fstream> // std::ofstream
#include <iostream> // std::cout
#include <streambuf> // std::streambuf

#include <TH1F.h>
#include <TFile.h>
#include <TString.h>

#include "common.hpp"

int main(void) {
	std::string inFilename = "res/TTresults.root";
	std::string outDir = "effs";
	TFile * in = TFile::Open(inFilename.c_str());
	
	std::vector<Double_t> threshold;
	Double_t thresholdStart = 0.1;
	Double_t thresholdStep = 0.02;
	while(thresholdStart < 1.0) {
		threshold.push_back(thresholdStart);
		thresholdStart += thresholdStep;
	}
	
	std::string csvString = "CSV cut,c jet,b jet,light jet";
	bool printToFile = true;
	
	for(int j = 0; j < 6; ++j) {
		for(int k = 0; k < 3; ++k) {
			for(auto th: threshold){
				for(int i = 0; i < 3; ++i) {
					TH1F * h = dynamic_cast<TH1F *> (in -> Get(getName(i, j, k).c_str()));
					h -> Scale(1.0 / h -> Integral());
					Int_t binIndex = h -> FindBin(th);
					//Double_t intBelow = h -> Integral(1, binIndex - 1);
					
					Double_t integralAbove = h -> Integral(binIndex, h -> GetNbinsX());
					
				}
			}
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
					
					Double_t integralAbove = h -> Integral(binIndex, h -> GetNbinsX());
					out << integralAbove;
					if(i == 2) 	out << std::endl;
					else		out << ",";
				}
			}
			*/
		}
	}
	return EXIT_SUCCESS;
}