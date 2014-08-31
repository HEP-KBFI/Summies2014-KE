#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include <cstdlib> // EXIT_SUCCESS, std::exit
#include <iostream> // std::cout, std::cerr, std::endl
#include <fstream> // std::ofstream
#include <sstream> // std::stringstream
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
	bool setLog = false, writeToFile = false, doPlots = false, doKolmogorov = false, doChi2 = false, useNormalized = false;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,i", po::value<std::string>(&input), "input *.root file")
			("dimx,x", po::value<Int_t>(&dimX) -> default_value(900), "the x dimension of the histogram")
			("dimy,y", po::value<Int_t>(&dimY) -> default_value(600), "the y dimension of the histogram")
			("dir,d", po::value<std::string>(&dir), "the output directory")
			("extension,e", po::value<std::string>(&extension), "extension of the output files\nif not set, no plots will be created")
			("output,o", po::value<std::string>(&output), "output file\nif not specified, printed to stdout")
			("kolmogorov,k", "do Kolmogorov test")
			("chi2,c", "do chi 2 test")
			("enable-log,l", "sets y-axis to logarithmic scale")
			("use-normalized,n", "use normalized histograms to do the statistical tests")
		;
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		
		if(vm.count("help")) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS); // ugly
		}
		if(vm.count("input") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS);
		}
		if(vm.count("enable-log") > 0) {
			setLog = true;
		}
		if(vm.count("extension") > 0) {
			doPlots = true;
		}
		if(vm.count("output") > 0) {
			writeToFile = true;
		}
		if(vm.count("kolmogorov") > 0) {
			doKolmogorov = true;
		}
		if(vm.count("chi2") > 0) {
			doChi2 = true;
		}
		if(vm.count("use-normalized") > 0) {
			useNormalized = true;
		}
	}
	catch(std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		std::exit(EXIT_FAILURE); // ugly
	}
	catch(...) {
		std::cerr << "exception of unkown type" << std::endl;
	}
	if(! (doPlots || doKolmogorov || doChi2)) {
		std::cerr << "at least one of the following flags are required: -e -k -c" << std::endl;
		std::exit(EXIT_FAILURE);
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
	histoTitles["pt"] = "p_{t} of all jets";
	histoTitles["eta"] = "#eta of all jets";
	histoTitles["csv"] = "CSV of all jets";
	histoTitles["leadPt"] = "p_{t} of leading jet";
	histoTitles["subleadPt"] = "p_{t} of subleading jet";
	
	std::map<std::string, std::string> histoLabels;
	histoLabels["H"] = "hard cut";
	histoLabels["A"] = "analytical weight";
	histoLabels["M"] = "multisample weight";
	histoLabels["R"] = "real CSV btag cut";
	
	std::map<std::string, std::string> histoXaxis;
	histoXaxis["pt"] = "Jet p_{t} (GeV)";
	histoXaxis["eta"] = "Jet #eta";
	histoXaxis["csv"] = "Jet CSV";
	histoXaxis["leadPt"] = "p_{t} of lead jet (GeV)";
	histoXaxis["subleadPt"] = "p_{t} of sublead jet (GeV)";
	
	if(doPlots) {
		for(auto & kv: vars) {
			auto s = kv.first;
			TCanvas * c = new TCanvas(histoTitles[s].c_str(), histoTitles[s].c_str(), dimX, dimY);
			gStyle -> SetOptStat(kFALSE);
			TLegend * legend = new TLegend(0.70, 0.90, 0.95, 1.0);
			//legend -> SetNColumns(vars[s]);
			
			Float_t maxY = -1;
			for(auto & h: histoMap) {
				std::string title = h.first;
				if(boost::iequals(title.substr(0, title.find(" ")), s)) {
					Float_t scaleFactor = useNormalized ? float(1e5) / h.second -> Integral() : 1;
					maxY = maxY < scaleFactor * (h.second -> GetMaximum()) ? scaleFactor * (h.second -> GetMaximum()) : maxY;
				}
			}
			
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
					
					if(useNormalized) {
						h.second -> Scale(float(1e5) / h.second -> Integral());
						h.second -> GetYaxis() -> SetTitle("Normalized (to 10^{5}) number of events per bin");
					}
					else {
						h.second -> GetYaxis() -> SetTitle("Number of events per bin");
					}
					h.second -> GetYaxis() -> SetTitleOffset(1.3);
					h.second -> SetMinimum(0);
					h.second -> SetMaximum(1.1 * maxY);
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
	}
	
	if(doKolmogorov || doChi2) {
		std::stringstream ss;
		for(auto & h: histoMap) {
			std::string title = h.first;
			std::string varString = title.substr(0, title.find(" "));
			std::string suffix = title.substr(title.find(" ") + 1);
			if(boost::iequals(suffix, "R")) { // compare R with A, M or H
				std::map<std::string, TH1F *> testHistos;
				for(auto & k: histoMap) {
					std::string titleInner = k.first;
					std::string varStringInner = titleInner.substr(0, titleInner.find(" "));
					std::string suffixInner = titleInner.substr(titleInner.find(" ") + 1);
					if(! boost::iequals(varString, varStringInner)) continue;
					if(boost::iequals(suffixInner, "A") || boost::iequals(suffixInner, "M")
						|| boost::iequals(suffixInner, "H")
					) {
						testHistos[suffixInner] = k.second;
					}
				}
				ss << varString << std::endl;
				for(auto & kv: testHistos) {
					ss << kv.first << std::endl;
					if(useNormalized) {
						kv.second -> Scale(float(1e5) / kv.second -> Integral());
					}
					if(doKolmogorov) {
						auto kTest = h.second -> KolmogorovTest(kv.second);
						ss << "Kolmogorov:\t" << std::fixed << kTest << std::endl;
					}
					if(doChi2) {
						auto kTest = h.second -> Chi2Test(kv.second, "UW");
						ss << "Chi2:\t\t" << std::fixed << kTest << std::endl;
					}
					ss << "-----------------------------" << std::endl;
				}
			}
		}
		std::streambuf * buf;
		std::ofstream of;
		if(writeToFile) {
			of.open(output);
			buf = of.rdbuf();
		}
		else {
			buf = std::cout.rdbuf();
		}
		std::ostream out(buf);
		out << ss.str();
		//out << binss.str();
	}
		
	inFile -> Close();
	
	return EXIT_SUCCESS;
}