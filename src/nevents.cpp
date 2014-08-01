#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <iostream> // std::cout, std::endl
#include <string> // std::string
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS

#include <TFile.h>
#include <TTree.h>

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	using boost::property_tree::ptree; // ptree, read_ini
	
	// command line option parsing
	std::string cmd_input, cmd_tree, configFile, field;
	bool hasConfig = false;
	std::string fieldHelp = "the field of the config file to be read";
	fieldHelp.append("\npossible values:\n    H (for histogram)\n    S (for sample)\n");
	fieldHelp.append("the flag must be given if config file is specified");
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,i", po::value<std::string>(&cmd_input), "input *.root file")
			("tree,t", po::value<std::string>(&cmd_tree), "tree name of the input file")
			("config,c", po::value<std::string>(&configFile), "config file\nif not set, read input flags")
			("field,f", po::value<std::string>(&field),	fieldHelp.c_str())
		;
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		
		if(vm.count("help")) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS); // ugly
		}
		if(vm.count("config") == 0 && (vm.count("input") == 0 || vm.count("tree") == 0)) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("config") > 0 && vm.count("field") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if(vm.count("config")) {
			hasConfig = true;
		}
	}
	catch(std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		std::exit(EXIT_FAILURE); // ugly
	}
	catch(...) {
		std::cerr << "exception of unkown type" << std::endl;
	}
	
	std::string cfg_input, cfg_tree;
	if(hasConfig) {
		// parse config file
		// if the config file doesn't exists, the program throws an error and exits
		ptree pt_ini;
		read_ini(configFile, pt_ini);
		auto trim = [] (std::string s) -> std::string {
			s = s.substr(0, s.find(";")); // remove the comment
			boost::algorithm::trim(s); // remove whitespaces around the string
			return s;
		};
		std::string key;
		if(boost::iequals(field, "h")) 	key = "histogram";
		else 							key = "sample";
		cfg_input = trim(pt_ini.get<std::string>(key + ".in"));
		cfg_tree = trim(pt_ini.get<std::string>(key + ".tree"));
	}
	
	std::string input = hasConfig ? cfg_input : cmd_input;
	std::string tree = hasConfig ? cfg_tree : cmd_tree;
	
	TFile * f = TFile::Open(input.c_str(), "read");
	if(f -> IsZombie() || ! f -> IsOpen()) {
		std::cerr << "error on opening " << input << std::endl;
		std::exit(EXIT_FAILURE);
	}
	TTree * t = dynamic_cast<TTree *> (f -> Get(tree.c_str()));
	std::cout << (t -> GetEntries());
	return EXIT_SUCCESS;
}