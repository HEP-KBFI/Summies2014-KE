#include <boost/program_options.hpp>

#include <cstdlib> // EXIT_SUCCESS
#include <string> // std::string
#include <iostream> // std::cout
#include <sstream> // std::stringstream
#include <streambuf> // std::streambuf
#include <fstream> // std::ofstream

#include <TFile.h>
#include <TH1F.h>
#include <TString.h>

#include "common.hpp"

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	
	std::string input_1, input_2, outFile;
	bool doKolmogorov = false, doChi2 = false;
	
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input1,i", po::value<std::string>(&input_1), "the first input *.root file")
			("input2,j", po::value<std::string>(&input_2), "the second input *.root file")
			("out,o", po::value<std::string>(&outFile), "output file; if not set, print to stdout")
			("use-kolmogorov,K", "Kolmogorov test")
			("use-chi2,C", "Chi2 test")
		;
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		
		if(vm.count("help")) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS); // ugly
		}
		if(vm.count("input1") == 0 || vm.count("input2") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("use-kolmogorov")) {
			doKolmogorov = true;
		}
		if(vm.count("use-chi2")) {
			doChi2 = true;
		}
		if(vm.count("use-kolmogorov") == 0 && vm.count("use-chi2")) {
			std::cout << "You must specify at least one test." << std::endl;
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}
	catch(std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		std::exit(EXIT_FAILURE); // ugly
	}
	catch(...) {
		std::cerr << "exception of unkown type" << std::endl;
	}
	
	TFile * df = TFile::Open(input_1.c_str(), "read");
	TFile * sf = TFile::Open(input_2.c_str(), "read");
	if(df -> IsZombie() || ! df -> IsOpen()) {
		std::cerr << "error on opening " << input_1 << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if(sf -> IsZombie() || ! sf -> IsOpen()) {
		std::cerr << "error on opening " << input_2 << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	std::stringstream kolmo_ss, chi2_ss;
	
	for(int i = 0; i < 3; ++i) {
		for(int j = 0; j < 6; ++j) {
			for(int k = 0; k < 3; ++k) {
				std::string hname = getName(i, j, k);
				TH1F * dh = dynamic_cast<TH1F *> (df -> Get(hname.c_str()));
				TH1F * sh = dynamic_cast<TH1F *> (sf -> Get(hname.c_str()));
				dh -> Scale(1.0 / dh -> Integral());
				sh -> Scale(1.0 / sh -> Integral());
				if(doKolmogorov) {
					Double_t kolmoVal = dh -> KolmogorovTest(sh);
					kolmo_ss << hname << "," << std::fixed << kolmoVal << std::endl;
				}
				if(doChi2) {
					Double_t chi2Val = dh -> Chi2Test(sh, "WW");
					chi2_ss << hname << "," << std::fixed << chi2Val << std::endl;
				}
				delete dh;
				delete sh;
			}
		}
	}
	df -> Close();
	sf -> Close();
	
	std::streambuf * buf;
	std::ofstream of;
	if(! outFile.empty()) {
		of.open(outFile);
		buf = of.rdbuf();
	}
	else {
		buf = std::cout.rdbuf();
	}
	std::ostream out(buf);
	
	if(doKolmogorov) {
		out << "Kolmogorov test:" << std::endl;
		out << kolmo_ss.str();
	}
	if(doChi2) {
		out << "Chi2 test:" << std::endl;
		out << chi2_ss.str();
	}
	
	return EXIT_SUCCESS;
}