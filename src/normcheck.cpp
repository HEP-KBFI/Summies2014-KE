#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <cstdlib> // std::exit(), EXIT_SUCCESS, EXIT_FAILURE
#include <string> // std::string
#include <algorithm> // std::accumulate()
#include <vector> // std::vector<>
#include <fstream> // std::ofstream
#include <iostream> // std::cout
#include <streambuf> // std::streambuf

#include <TFile.h>
#include <TTree.h>

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	using boost::property_tree::ptree; // ptree, read_ini
	
	std::string configFile, output, varName, treeName;
	bool enableVerbose = false, writeToFile = false;
	Long64_t beginEvent, endEvent;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("config,c", po::value<std::string>(&configFile), "read config file")
			("begin,b", po::value<Long64_t>(&beginEvent) -> default_value(0), "the event number to start with")
			("end,e", po::value<Long64_t>(&endEvent) -> default_value(-1), "the event number to end with\ndefault (-1) means all events")
			("output,o", po::value<std::string>(&output), "output file name")
			("var-name,n", po::value<std::string>(&varName), "name of the probability variable")
			("tree,t", po::value<std::string>(&treeName), "tree name")
			("verbose,v", "verbose mode (enables progressbar)")
		;
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		
		if(vm.count("help")) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS); // ugly
		}
		if(vm.count("verbose") != 0) {
			enableVerbose = true;
		}
		if(vm.count("config") == 0 || vm.count("var-name") == 0 || vm.count("tree") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS);
		}
		if(vm.count("output") > 0) {
			writeToFile = true;
		}
	}
	catch(std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		std::exit(EXIT_FAILURE); // ugly
	}
	catch(...) {
		std::cerr << "exception of unkown type" << std::endl;
	}
	
	// sanity check
	if((endEvent >=0 && beginEvent > endEvent) || beginEvent < 0) {
		std::cerr << "incorrect values for begin and/or end" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	if(enableVerbose) std::cout << "Parsing configuration file " << configFile << " ... " << std::endl;
	ptree pt_ini;
	read_ini(configFile, pt_ini);
	auto trim = [] (std::string s) -> std::string {
		s = s.substr(0, s.find(";")); // remove the comment
		boost::algorithm::trim(s); // remove whitespaces around the string
		return s;
	};
	
	if(enableVerbose) std::cout << "Opening files ..." << std::endl;
	auto section = pt_ini.get_child("norm");
	std::vector<TFile *> files;
	for(auto & key: section) {
		files.push_back(TFile::Open(trim(key.second.data()).c_str(), "read"));
	}
	
	if(enableVerbose) std::cout << "Opening trees ..." << std::endl;
	std::vector<TTree *> trees;
	for(auto & file: files) {
		trees.push_back(dynamic_cast<TTree *>(file -> Get(treeName.c_str())));
	}
	
	std::vector<Float_t> probabilities(trees.size(), 0);
	for(unsigned int i = 0; i < trees.size(); ++i) {
		trees[i] -> SetBranchAddress(varName.c_str(), &probabilities[i]);
	}
	
	if(endEvent == -1) {
		Long64_t maxEvent = trees[0] -> GetEntries();
		for(auto & tree: trees) {
			if(maxEvent > tree -> GetEntries()) maxEvent = tree -> GetEntries();
		}
		endEvent = maxEvent;
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
	
	if(enableVerbose) std::cout << "Looping over " << (endEvent - beginEvent) << " events ..." << std::endl;
	for(Long64_t i = beginEvent; i < endEvent; ++i) {
		for(auto & tree: trees) {
			tree -> GetEntry(i);
		}
		Float_t sum = std::accumulate(probabilities.begin(), probabilities.end(), 0.0);
		out << "EVENT " << std::to_string(i) << ":\t" << std::fixed << std::to_string(sum) << std::endl;
	}
	
	if(enableVerbose) std::cout << "Closing the files ..." << std::endl;
	for(auto file: files) {
		file -> Close();
	}
	
	return EXIT_SUCCESS;
}