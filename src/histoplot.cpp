#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

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
	using boost::property_tree::ptree; // ptree, read_ini
	
	// command line option parsing
	Int_t dimX, dimY;
	Float_t cmd_workingPoint;
	std::string inName, extension, dir, config, otherInput;
	bool setLog = false, useSampled = false, useMultisampled = false, plotIterations=false, plotAllInOne=true, customNorm = false;
	
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,i", po::value<std::string>(&inName), "input *.root file")
			("other-input,j", po::value<std::string>(&otherInput), "other input file which helps to normalize distributions")
			("dimx,x", po::value<Int_t>(&dimX) -> default_value(900), "the x dimension of the histogram")
			("dimy,y", po::value<Int_t>(&dimY) -> default_value(600), "the y dimension of the histogram")
			("extension,e", po::value<std::string>(&extension), "the extension of the output file")
			("dir,d", po::value<std::string>(&dir), "the output directory")
			("config,c", po::value<std::string>(&config), "config file (needed when -m is given)")
			("working-point,w", po::value<Float_t>(&cmd_workingPoint) -> default_value(-1), "working point (needed when -m is given)")
			("enable-log,l", "sets y-axis to logarithmic scale")
			("use-sampled,s", "adds 'sampled' to the x-axis label")
			("use-multisampled,m", "adds 'multiple times sampled' to the x-axis label")
			("plot-iterations,p", "plots the number of iterations to pass the working point")
			("plot-single,n", "plot the number of iterations to separate canvases")
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
		if(vm.count("use-sampled")) {
			useSampled = true;
		}
		if(vm.count("use-multisampled")) {
			useMultisampled = true;
			if(vm.count("config") == 0 && vm.count("working-point") == 0) {
				std::cout << desc << std::endl;
				std::exit(EXIT_FAILURE);
			}
		}
		if((vm.count("use-sampled") > 0) && (vm.count("use-multisampled") > 0)) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("plot-iterations")) {
			plotIterations = true;
		}
		if(vm.count("plot-iterations") > 0 && (vm.count("use-sampled") > 0 || vm.count("use-multisampled") > 0)) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("plot-single")) {
			plotAllInOne = false;
		}
		if(vm.count("other-input") > 0) {
			customNorm = true;
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
	
	Float_t workingPoint, cfg_workingPoint = -1;
	if(! config.empty()) {
		ptree pt_ini;
		read_ini(config, pt_ini);
		auto trim = [] (std::string s) -> std::string {
			s = s.substr(0, s.find(";")); // remove the comment
			boost::algorithm::trim(s); // remove whitespaces around the string
			return s;
		};
		
		cfg_workingPoint = std::atof(trim(pt_ini.get<std::string>("sample.wp")).c_str());
	}
	
	workingPoint = (cmd_workingPoint == -1) ? cfg_workingPoint : cmd_workingPoint;
	
	TFile * in = TFile::Open(inName.c_str(), "read");
	if(in -> IsZombie() || ! in -> IsOpen()) {
		std::cerr << "error on opening the root file" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	std::string key = "csv_";
	if(useSampled || useMultisampled) key = "csvGen_";
	else if(plotIterations) key = "csvN_";
	
	if(plotIterations) {
		if(plotAllInOne) {
			for(int j = 0; j < 6; ++j) {
				for(int k = 0; k < 3; ++k) {
					std::string canvasTitle = getAbbrName(j, k);
					TCanvas * c = new TCanvas(canvasTitle.c_str(), canvasTitle.c_str(), dimX, dimY);
					gStyle -> SetOptStat(kFALSE);
					TLegend * legend = new TLegend(0.37, 0.85, 0.63, 0.90);
					legend -> SetNColumns(3);
					Float_t maxY = -1;
					Int_t maxBins = -1;
					// the line
					//        maxY = maxY < h -> GetMaximum() ? h -> GetMaximum() : maxY;
					// in the second loop doesn't work, must loop over first to get the max value
					// of the Y axis
					//std::map<int, Int_t> w;
					for(int i = 0; i < 3; ++i) {
						TH1F * h = dynamic_cast<TH1F *> (in -> Get(getName(i, j, k, key).c_str()));
						//h -> Scale(1.0/(h -> Integral()));
						maxY = h -> GetMaximum() > maxY ? h -> GetMaximum() : maxY;
						maxBins = h -> GetNbinsX() > maxBins ? h -> GetNbinsX() : maxBins;
						delete h;
						//w[XendpointMultisample[i]] = i;
					}
					
					//typedef std::map<int, Int_t>::reverse_iterator iter;
					//int firstIndex = w.rbegin() -> second;
					//for(iter it = w.rbegin(); it != w.rend(); ++it) {
					for(int i = 0; i < 3; ++i) {
						//int i = it -> second;
						TH1F * h = dynamic_cast<TH1F *> (in -> Get(getName(i, j, k, key).c_str()));
						std::string legendLabel = flavorNames[i] + " jet";
						std::stringstream histoTitle;
						histoTitle << getHistoTitle(j, k) << " @ " << maxBins << " bins";
						h -> SetLineColor(colorRanges[i]);
						h -> SetLineWidth(2);
						h -> GetXaxis() -> SetTitle("Number of iterations");
						h -> GetYaxis() -> SetTitle("Normalized number of events per bin");
						h -> GetYaxis() -> SetTitleOffset(1.2);
						h -> SetMaximum(1.1 * maxY);
						h -> SetMinimum(1);
						h -> Draw((i == 0 ? "hist e" : "same hist e")); // same e for the error bars
						h -> SetTitle(histoTitle.str().c_str());
						legend -> AddEntry(h, legendLabel.c_str());
						if(setLog) c -> SetLogy(1);
						c -> SetRightMargin(0.05);
						c -> Modified();
						c -> Update();
					}
					legend -> Draw();
					
					if(setLog) canvasTitle = "log_" + canvasTitle;
					canvasTitle = "iter_" + canvasTitle;
					canvasTitle = "hist_" + canvasTitle;
					if(! dir.empty()) canvasTitle = dir + "/" + canvasTitle;
					c -> SaveAs(canvasTitle.append("." + extension).c_str()); // char * = TString
					c -> Close();
					delete legend;
				}
			}
		} else {
			for(int i = 0; i < 3; ++i) {
				for(int j = 0; j < 6; ++j) {
					for(int k = 0; k < 3; ++k) {
						std::string canvasTitle = getName(i, j, k, key);
						TCanvas * c = new TCanvas(canvasTitle.c_str(), canvasTitle.c_str(), dimX, dimY);
						gStyle -> SetOptStat(kFALSE);
						
						TH1F * h = dynamic_cast<TH1F *> (in -> Get(getName(i, j, k, key).c_str()));
						//h -> Scale(1.0 / h -> Integral()); // ???
						Float_t maxY = h -> GetMaximum();
						
						std::stringstream histoTitle;
						histoTitle << getHistoTitle(i, j, k) << " @ " << h -> GetNbinsX() << " bins";
						h -> SetLineColor(colorRanges[i]);
						h -> SetLineWidth(2);
						h -> GetXaxis() -> SetTitle("Number of iterations");
						h -> GetYaxis() -> SetTitle("Number of events per bin");
						h -> GetYaxis() -> SetTitleOffset(1.5);
						h -> SetMaximum(1.1 * maxY);
						h -> Draw("hist e"); // same e for the error bars
						h -> SetTitle(histoTitle.str().c_str());
						if(setLog) c -> SetLogy(1);
						c -> SetRightMargin(0.05);
						c -> Modified();
						c -> Update();
						
						if(setLog) canvasTitle = "log_" + canvasTitle;
						canvasTitle = "iter_" + canvasTitle;
						canvasTitle = "hist_" + canvasTitle;
						if(! dir.empty()) canvasTitle = dir + "/" + canvasTitle;
						c -> SaveAs(canvasTitle.append("." + extension).c_str()); // char * = TString
						c -> Close();
					}
				}
			}
		}
	}
	else {
		for(int j = 0; j < 6; ++j) {
			for(int k = 0; k < 3; ++k) {
				std::string canvasTitle = getAbbrName(j, k);
				TCanvas * c = new TCanvas(canvasTitle.c_str(), canvasTitle.c_str(), dimX, dimY);
				gStyle -> SetOptStat(kFALSE);
				TLegend * legend = new TLegend(0.37, 0.85, 0.63, 0.90);
				legend -> SetNColumns(3);
				Float_t maxY = -1;
				// the line
				//        maxY = maxY < h -> GetMaximum() ? h -> GetMaximum() : maxY;
				// in the second loop doesn't work, must loop over first to get the max value
				// of the Y axis
				for(int i = 0; i < 3; ++i) {
					TH1F * h = dynamic_cast<TH1F *> (in -> Get(getName(i, j, k, key).c_str()));
					h -> Scale(1.0/(h -> Integral()));
					maxY = h -> GetMaximum() > maxY ? h -> GetMaximum() : maxY;
					delete h;
				}
				for(int i = 0; i < 3; ++i) {
					TH1F * h = dynamic_cast<TH1F *> (in -> Get(getName(i, j, k, key).c_str()));
					std::string legendLabel = flavorNames[i] + " jet";
					std::stringstream histoTitle;
					histoTitle << "CSV   " << getHistoTitle(j, k) << " @ ";
					std::string xLabel = "CSV discriminator";
					if(useSampled)				xLabel = "Sampled " + xLabel;
					else if(useMultisampled)	xLabel = "Multiple times sampled " + xLabel;
					h -> SetLineColor(colorRanges[i]);
					h -> SetLineWidth(2);
					if(useMultisampled) {
						Int_t wpBin = h -> FindBin(workingPoint);
						Int_t nBins = h -> GetNbinsX();
						//h -> GetXaxis() -> SetRange(wpBin - 1, nBins);
						histoTitle << (nBins - wpBin + 1) << " bins";
						xLabel += " (wp " + std::to_string(workingPoint).substr(0, 5) + ")";
					}
					else {
						histoTitle << h -> GetNbinsX() << " bins";
					}
					h -> GetXaxis() -> SetTitle(xLabel.c_str());
					h -> GetYaxis() -> SetTitle("Normalized number of events per bin");
					h -> GetYaxis() -> SetTitleOffset(1.2);
					if(customNorm) {
						TFile * customFile = TFile::Open(otherInput.c_str(), "read");
						TH1F * customHisto = dynamic_cast<TH1F *> (customFile -> Get(getName(i, j, k).c_str()));
						Int_t wpBin = customHisto -> FindBin(workingPoint);
						Int_t nBins = customHisto -> GetNbinsX();
						Float_t notSoPreciseIntegral = customHisto -> Integral(wpBin, nBins);
						Float_t normalizationFactor = notSoPreciseIntegral / customHisto -> Integral();
						h -> Scale(normalizationFactor / h -> Integral());
					}
					else {
						h -> Scale(1.0/(h -> Integral()));
					}
					h -> SetMinimum(1e-3);
					h -> SetMaximum(1.1 * maxY);
					h -> Draw((i == 0 ? "hist e" : "same hist e")); // same e for the error bars
					h -> SetTitle(histoTitle.str().c_str());
					legend -> AddEntry(h, legendLabel.c_str());
					if(setLog) c -> SetLogy(1);
					c -> SetRightMargin(0.05);
					c -> Modified();
					c -> Update();
				}
				legend -> Draw();
				if(setLog) canvasTitle = "log_" + canvasTitle;
				if(useSampled)				canvasTitle = "sampled_" + canvasTitle;
				else if(useMultisampled)	canvasTitle = "multisampled_" + canvasTitle;
				canvasTitle = "hist_" + canvasTitle;
				if(! dir.empty()) canvasTitle = dir + "/" + canvasTitle;
				c -> SaveAs(canvasTitle.append("." + extension).c_str()); // char * = TString
				c -> Close();
				delete legend;
			}
		}
	}
	
	in -> Close();
	return EXIT_SUCCESS;
}