#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include <cstdlib> // EXIT_SUCCESS, std::exit
#include <iostream> // std::cout, std::cerr, std::endl
#include <fstream> // std::ofstream
#include <streambuf> // std::streambuf
#include <map> // std::map<>
#include <string> // std::string
#include <vector> // std::vector<>

#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TKey.h>
#include <TROOT.h>
#include <TClass.h>
#include <TStyle.h>
#include <TLegend.h>
#include <TCanvas.h>

#include "common.hpp"

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	
	std::string input, output, dir, extension;
	Int_t dimX, dimY;
	bool setLog = false;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,i", po::value<std::string>(&input), "input *.root file")
			("dimx,x", po::value<Int_t>(&dimX) -> default_value(900), "the x dimension of the histogram")
			("dimy,y", po::value<Int_t>(&dimY) -> default_value(600), "the y dimension of the histogram")
			("dir,d", po::value<std::string>(&dir), "the output directory")
			("extension,e", po::value<std::string>(&extension), "extension of the output files")
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
			std::exit(EXIT_SUCCESS);
		}
		if(vm.count("enable-log") > 0) {
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
	
	TFile * inFile = TFile::Open(input.c_str(), "read");
	if(inFile -> IsZombie() || ! inFile -> IsOpen()) {
		std::cerr << "Couldn't open " << input << "." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	TKey * key;
	TIter next(inFile -> GetListOfKeys());
	std::map<std::string, TH1F *> histoMap;
	while((key = dynamic_cast<TKey *>(next()))) {
		TClass * cl = gROOT -> GetClass(key -> GetClassName());
		if(! cl -> InheritsFrom("TH1F")) continue;
		TH1F * h = dynamic_cast<TH1F *> (key -> ReadObj());
		histoMap[std::string(h -> GetTitle())] = h;
	}
	
	std::map<std::string, int> vars;
	for(auto & s: histoMap) {
		std::string stemp = s.first.substr(0, s.first.find(" "));
		if(vars.count(stemp) == 0) {
			vars[stemp] = 1;
		}
		else {
			++vars[stemp];
		}
	}
	
	std::map<std::string, std::string> histoTitles;
	histoTitles["pt"] = "Jet p_{t}";
	histoTitles["eta"] = "Jet #eta";
	histoTitles["csv"] = "Jet CSV";
	
	std::map<std::string, std::string> histoLabels;
	histoLabels["H"] = "hard cut";
	histoLabels["A"] = "analytical weight";
	histoLabels["M"] = "multisample weight";
	
	std::map<std::string, std::string> histoXaxis;
	histoXaxis["pt"] = "Jet p_{t} (GeV)";
	histoXaxis["eta"] = "Jet #eta";
	histoXaxis["csv"] = "Jet CSV";
	
	for(auto & kv: vars) {
		auto s = kv.first;
		TCanvas * c = new TCanvas(histoTitles[s].c_str(), histoTitles[s].c_str(), dimX, dimY);
		gStyle -> SetOptStat(kFALSE);
		TLegend * legend = new TLegend(0.60, 0.90, 0.95, 0.95);
		legend -> SetNColumns(vars[s]);
		int counter = 0;
		for(auto & h: histoMap) {
			std::string title = h.first;
			if(boost::iequals(title.substr(0, title.find(" ")), s)) {
				std::string suffix = title.substr(title.find(s) + s.size() + 1);
				h.second -> SetLineColor(colorRanges[counter]);
				h.second -> SetLineWidth(2);
				h.second -> SetLineWidth(2);
				
				h.second -> GetXaxis() -> SetTitle(histoXaxis[s].c_str());
				h.second -> GetXaxis() -> SetTitleOffset(1.2);
				h.second -> GetYaxis() -> SetTitle("Number of events per bin");
				h.second -> GetYaxis() -> SetTitleOffset(1.2);
				h.second -> SetMinimum(0);
				h.second -> Draw((counter == 0 ? "hist e" : "same hist e")); // same e for the error bars
				h.second -> SetTitle(histoTitles[s].c_str());
				std::string entriesString = " (" + std::to_string(Int_t(h.second -> GetEntries())) + ")";
				legend -> AddEntry(h.second, (histoLabels[suffix] + entriesString).c_str());
				c -> SetGrid();
				if(setLog) c -> SetLogy(1);
				c -> SetRightMargin(0.05);
				c -> Modified();
				c -> Update();
				++counter;
			}
		}
		legend -> Draw();
		std::string canvasTitle = s;
		if(setLog) canvasTitle = "log_" + canvasTitle;
		if(! dir.empty()) canvasTitle = dir + "/" + canvasTitle;
		c -> SaveAs(canvasTitle.append("." + extension).c_str());
		c -> Close();
		delete legend;
	}
	
	inFile -> Close();
	
	return EXIT_SUCCESS;
}