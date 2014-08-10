#include <boost/program_options.hpp>

#include <cstdlib> // EXIT_SUCCESS
#include <map> // std::map
#include <vector> // std::vector<>
#include <sstream> // std::stringstream
#include <iostream> // std::cout, std::cerr, std::endl
#include <algorithm> // std::sort

#include <TCanvas.h>
#include <TFile.h>
#include <TH1F.h>
#include <TString.h>
#include <TStyle.h>
#include <TLegend.h>

#include "common.hpp"

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	
	// command line option parsing
	Int_t dimX, dimY;
	std::string inName, extension, dir;
	bool setLog = false;
	
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,i", po::value<std::string>(&inName), "input *.root file")
			("dimx,x", po::value<Int_t>(&dimX) -> default_value(900), "the x dimension of the histogram")
			("dimy,y", po::value<Int_t>(&dimY) -> default_value(600), "the y dimension of the histogram")			
			("extension,e", po::value<std::string>(&extension), "the extension of the output file")
			("dir,d", po::value<std::string>(&dir), "the output directory")
			("enable-log,l", "sets y-axis to logarithmic scale")
		;
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		
		if(vm.count("help")) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS); // ugly
		}
		if(vm.count("input") == 0 || vm.count("extension") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("enable-log")) {
			setLog = true;
		}
	}
	catch(std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		std::exit(EXIT_FAILURE); // ugly
	}
	catch(...) {
		std::cerr << "exception of unkown type" << std::endl;
	}
	
	if(dimX < 100 || dimY < 100) {
		std::cerr << "the dimensions cannot be smaller than 100, try again" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	TFile * in = TFile::Open(inName.c_str(), "read");
	if(in -> IsZombie() || ! in -> IsOpen()) {
		std::cerr << "error on opening the root file" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	for(int j = 0; j < 6; ++j) {
		for(int k = 0; k < 3; ++k) {
			std::string canvasTitle = getAbbrName(j, k);
			TCanvas * c = new TCanvas(canvasTitle.c_str(), canvasTitle.c_str(), dimX, dimY);
			gStyle -> SetOptStat(kFALSE);
			TLegend * legend = new TLegend(0.17, 0.85, 0.43, 0.90);
			legend -> SetNColumns(3);
			for(int i = 0; i < 3; ++i) {
				TH1F * h = dynamic_cast<TH1F *> (in -> Get(getName(i, j, k).c_str()));
				std::string legendLabel = flavorNames[i] + " jet";
				std::stringstream histoTitle;
				histoTitle << "CSV CDF   " << getHistoTitle(j, k) << " @ ";
				histoTitle << h -> GetNbinsX() << " bins";
				std::string xLabel = "CSV discriminator";
				h -> SetLineColor(colorRanges[i]);
				h -> SetLineWidth(2);
				h -> GetXaxis() -> SetTitle(xLabel.c_str());
				h -> GetYaxis() -> SetTitle("Cumulative distribution function");
				h -> GetYaxis() -> SetTitleOffset(1.2);
				h -> SetMaximum(1.05);
				h -> SetMinimum(0.0);
				h -> Draw((i == 0 ? "hist" : "same hist")); // same e for the error bars
				h -> SetTitle(histoTitle.str().c_str());
				legend -> AddEntry(h, legendLabel.c_str());
				c -> SetGrid(1);
				if(setLog) c -> SetLogy(1);
				c -> SetRightMargin(0.05);
				c -> Modified();
				c -> Update();
			}
			legend -> Draw();
			if(setLog) canvasTitle = "log_" + canvasTitle;
			canvasTitle = "cumul_" + canvasTitle;
			if(! dir.empty()) canvasTitle = dir + "/" + canvasTitle;
			c -> SaveAs(canvasTitle.append("." + extension).c_str()); // char * = TString
			c -> Close();
			delete legend;
		}
	}
	
	in -> Close();
	return EXIT_SUCCESS;
}