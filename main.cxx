#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <exception> // std::exception
#include <map> // std::map
#include <string> // std::string
#include <vector> // std::vector
#include <iostream> // std::cerr, std::endl
#include <utility> // std::make_pair
#include <iterator> // std::advance
#include <cstdlib> // std::exit

#include <TString.h>
#include <TTree.h>
#include <TFile.h>

/**
 * @note
 *   - boost linked statically, root dynamically
 *   - assumption -- the same name for a TTree for all input files
 * 
 * @todo
 *   - looping over events, one file at the time (b/c big files)!
 *   - calculations with the variables per event
 *   - differentiating between data, signal and background
 *   - look up the makeclass thingy
 *   - output a root file
 */

class InputData {

typedef std::map<std::string, std::vector<std::string> > StringMap;

public:
	InputData(StringMap sbdFiles, std::string dir, TString tree)
		: sbdFiles(sbdFiles), dir(dir), tree(tree) { }
	const std::string getDir() const {
		return dir;
	}
	const TString getTreeName() const {
		return tree;
	}
	const std::vector<std::string> & getFileNames(std::string key) const {
		return sbdFiles.at(key);
	}
	int getLength(std::string key) const {
		return sbdFiles.at(key).size();
	}
private:
	const StringMap 	sbdFiles;
	const std::string 	dir;
	const TString 		tree;
};

class FilePointer {
public:
	FilePointer(InputData * input, std::string key) : input(input), key(key) { }
protected:
	const InputData * const input;
	const std::string 		key;
};

class MultipleFilePointer : public FilePointer {
public:
	MultipleFilePointer(InputData * input, std::string key) : FilePointer(input, key) { }
	void openAllFiles() {
		const auto stringList = input -> getFileNames(key);
		for(const std::string label: stringList) {
			std::string path = input -> getDir();
			path.append(label + ".root");
			files[label] = std::unique_ptr<TFile> (TFile::Open(path.c_str(), "read"));
			if(files[label] -> IsZombie()) {
				std::string msg = "Error on opening the file " + label + ". Abort.\n";
				throw msg;
			}
		}
		return;
	}
	void openAllTrees() {
		for(auto & kv: files) {
			if (kv.second -> IsOpen()) {
				std::unique_ptr<TTree> temp(dynamic_cast<TTree *> (kv.second -> Get(input -> getTreeName())));
				trees.emplace(kv.first, std::move(temp));
			}
			else {
				std::string msg = "The file " + std::string(kv.first) + " is not opened. Abort.\n";
				throw msg;
			}
		}
		return;
	}
	void clear() {
		trees.clear();
		files.clear();
		return;
	}
	const TTree * getTree(int n) const {
		auto it = trees.begin();
		std::advance(it, n);
		return (*it).second.get();
	}
	const TFile * getFile(int n) const {
		auto it = files.begin();
		std::advance(it, n);
		return (*it).second.get();
	}
	int getLength() const {
		return trees.size();
	}
private:
	std::map<TString, std::unique_ptr<TFile> > files;
	std::map<TString, std::unique_ptr<TTree> > trees;
};

class SingleFilePointer : public FilePointer {
public:
	SingleFilePointer(InputData * input, std::string key) : FilePointer(input, key) { }
	void openFile() {
		const auto stringList = input -> getFileNames(key);
		auto it = stringList.begin();
		std::advance(it, counter);
		fileName = (*it);
		std::string path = input -> getDir();
		path.append(fileName + ".root");
		file = std::unique_ptr<TFile> (TFile::Open(path.c_str(), "read"));
		if(file -> IsZombie()) {
			std::string msg = "Error on opening the file " + fileName + ". Abort.\n";
			throw msg;
		}
	}
	void openTree() {
		if (file -> IsOpen()) {
			std::unique_ptr<TObject> temp(file -> Get(input -> getTreeName()));
			tree = std::unique_ptr<TTree> (dynamic_cast<TTree *> (temp.get()));
			if(tree) temp.release();
		}
		else {
			std::string msg = "The file " + fileName + " is not opened. Abort.\n";
			throw msg;
		}
	}
	void close() {
		tree.reset();
		file -> Close();
		file.reset();
	}
	bool hasNext() const {
		return input -> getLength(key) - counter > 1;
	}
	void reset() {
		this -> close();
		counter = 0;
	}
	void next() {
		++counter;
	}
	const std::string getFileName() const {
		return fileName;
	}
	const TTree * getTree() const {
		return tree.get();
	}
	const TFile * getFile() const {
		return file.get();
	}
	int getLength() const {
		return input -> getLength(key);
	}
private:
	int 					counter = 0;
	std::unique_ptr<TFile> 	file;
	std::unique_ptr<TTree> 	tree;
	std::string fileName;
};

std::string trim(std::string);
InputData parse(int, char **);

int main(int argc, char ** argv) {
	InputData input = parse(argc, argv);
	
	MultipleFilePointer dataPointers(&input, "data");
	SingleFilePointer   sigPointers(&input, "signal");
	//MultipleFilePointer bkgPointers(&input, "background");
	
	
	dataPointers.openAllFiles();
	dataPointers.openAllTrees();
	dataPointers.getTree(0) -> Print();
	dataPointers.clear();
	
	dataPointers.clear();
	while(sigPointers.hasNext()) {
		sigPointers.openFile();
		sigPointers.openTree();
		sigPointers.getTree() -> Print();
		sigPointers.close();
		sigPointers.next();
	}
	
	return EXIT_SUCCESS;
}

std::string trim(std::string s) {
	s = s.substr(0, s.find(";")); // lose the comment
	boost::algorithm::trim(s); // lose whitespaces around the string
	return s;
}

InputData parse(int argc, char ** argv) {
	namespace po = boost::program_options;
	using boost::property_tree::ptree; // ptree, read_ini
	
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
	std::string dir = trim(pt.get<std::string>("misc.dir")).append("/");
	TString tree = trim(pt.get<std::string>("misc.tree"));
	
	std::map<std::string, std::vector<std::string> > sbdFiles;
	
	for(auto & section: pt) {
		for(auto & key: section.second) {
			std::string temp = key.second.get_value<std::string>();
			sbdFiles[section.first].push_back(trim(temp));
		}
	}
	return InputData(sbdFiles, dir, tree);
}