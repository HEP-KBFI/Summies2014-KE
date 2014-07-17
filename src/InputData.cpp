#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <exception> // std::exception
#include <cstdlib> // std::exit

#include "InputData.h"

// section nomenclature
#define SIGNAL		std::string("signal")
#define BACKGROUND 	std::string("background")
#define DATA		std::string("data")
#define VARIABLES	std::string("variables")
#define RANGES		std::string("ranges")
#define FLAVORS		std::string("flavors")
#define HISTOGRAM	std::string("histogram")
// variable nomenclature
#define DIR			std::string("dir")
#define TREE		std::string("tree")
#define VAR		std::string("var")
#define ID			std::string("id")
#define XVAL		std::string("xval")
#define XNAME		std::string("xname")
#define XRANGE		std::string("xrange")
#define WEIGHTVAR	std::string("weightvar")
#define BINS		std::string("bins")

InputData::InputData(int argc, char ** argv) {
	parse(argc, argv);
}
InputData::~InputData() {
	sbdFiles.clear();
	dir = "";
	tree = "";
}
const std::string InputData::getDir() const {
	return dir;
}
const TString InputData::getTreeName() const {
	return tree;
}
const std::vector<std::string> & InputData::getFileNames(std::string key) const {
	return sbdFiles.at(key);
}
int InputData::getLength(std::string key) const {
	return sbdFiles.at(key).size();
}
void InputData::parse(int argc, char ** argv) {
	namespace po = boost::program_options;
	using boost::property_tree::ptree; // ptree, read_ini
	
	// command line option parsing
	std::string fileName;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,I", po::value<std::string>(&fileName) -> default_value("config.ini"), "read config file")
		;
		po::positional_options_description p;
		p.add("input", -1);
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
		po::notify(vm);
		
		if(vm.count("help")) {
			std::cout << desc << std::endl;
			std::exit(0); // ugly
		}
		
		if(vm.count("input")) {
			std::cout << "Parsing configuration file " << fileName << " ... " << std::endl;
		}
		else {
			// dummy
		}
	}
	catch(std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		std::exit(0); // ugly
	}
	catch(...) {
		std::cerr << "exception of unkown type" << std::endl;
	}
	
	// parse config file
	ptree pt;
	read_ini(fileName, pt);
	
	// a nasty way to read input params
	dir = trim(pt.get<std::string>("misc.dir")).append("/");
	tree = trim(pt.get<std::string>("misc.tree"));
	
	for(auto & section: pt) {
		std::cout << "[" << section.first << "]" << std::endl;
		for(auto & key: section.second) {
			std::string temp = key.second.get_value<std::string>();
			sbdFiles[section.first].push_back(trim(temp));
			std::cout << key.first << " = " << trim(temp) << std::endl;
		}
	}
}
std::string InputData::trim(std::string s) {
	s = s.substr(0, s.find(";")); // lose the comment
	boost::algorithm::trim(s); // lose whitespaces around the string
	return s;
}