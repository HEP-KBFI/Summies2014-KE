#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <exception> // std::exception
#include <cstdlib> // std::exit, std::atof

#include "Common.h"
#include "InputData.h"

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
	auto trim = [] (std::string s) -> std::string {
		s = s.substr(0, s.find(";")); // lose the comment
		boost::algorithm::trim(s); // lose whitespaces around the string
		return s;
	};
	
	// misc
	dir = trim(pt.get<std::string>(MISC + "." + DIR));
	tree = trim(pt.get<std::string>(MISC + "." + TREE));
	
	// input files
	auto inputFiles = [&pt, &trim, this] (std::string type) -> void {
		auto c = pt.get_child(type);
		for(auto & key: c) {
			sbdFiles[type].push_back(trim(key.second.get_value<std::string>()));
		}
	};
	inputFiles(DATA);
	inputFiles(BACKGROUND);
	inputFiles(SIGNAL);
	
	// flavor var
	std::string flavorVar = trim(pt.get<std::string>(FLAVORS + "." + VAR));
	
	// ranges of the variables
	{
		std::map<std::string, TString> varMap;
		auto c = pt.get_child(VARIABLES);
		for(auto & key: c) {
			if(boost::iequals(key.first, flavorVar)) continue;
			varMap[key.first] = key.second.get_value<std::string>().c_str();
		}
		for(auto & key: varMap) {
			auto d = pt.get_child(key.first + "_" + RANGES);
			for(auto & varKey: d) {
				std::string floatPair = trim(varKey.second.get_value<std::string>());
				int i = floatPair.find(",");
				std::string begRange = floatPair.substr(0,i);
				std::string endRange = floatPair.substr(i + 1);
				float fBegRange = std::atof(begRange.c_str());
				float fEndRange;
				// default INF could produce undefined behavior
				if(boost::iequals(endRange, inf)) {
					fEndRange = INF;
				}
				else {
					fEndRange = std::atof(endRange.c_str());
				}
				varRanges[key.second].push_back(std::make_pair(fBegRange, fEndRange));
			}
		}
	}
	
	// flavors again
	flavors.first = trim(pt.get<std::string>(VARIABLES + "." + flavorVar));
	auto splitString = [] (std::string s, std::string delim) -> std::vector<Float_t> {
		size_t pos = 0;
		std::string token;
		std::vector<Float_t> v;
		while((pos = s.find(delim)) != std::string::npos) {
			token = s.substr(0, pos);
			v.push_back(std::atof(token.c_str()));
			s.erase(0, pos + delim.length());
		}
		v.push_back(std::atof(s.c_str()));
		return v;
	};
	flavors.second = splitString(trim(pt.get<std::string>(FLAVORS + "." + ID)), ",");
	
	// histogram variables
	xval = trim(pt.get<std::string>(HISTOGRAM + "." + XVAL)).c_str();
	weightvar = std::atof(trim(pt.get<std::string>(HISTOGRAM + "." + WEIGHTVAR)).c_str());
	xname = trim(pt.get<std::string>(HISTOGRAM + "." + XNAME)).c_str();
	bins = std::atof(trim(pt.get<std::string>(HISTOGRAM + "." + BINS)).c_str());
	std::vector<Float_t> xr = splitString(trim(pt.get<std::string>(HISTOGRAM + "." + XRANGE)), ",");
	xrange.first = xr.at(0);
	xrange.second = xr.at(1);
	
}