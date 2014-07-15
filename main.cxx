#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <exception> // std::exception
#include <map> // std::map
#include <string> // std::string
#include <vector> // std::vector
#include <iostream> // std::cerr, std::endl
#include <iterator> // std::advance
#include <cstdlib> // std::exit
#include <utility> // std::pair
#include <limits> // std::numeric_limits<>
//#include <sstream> // std::stringstream
//#include <iomanip> // std::setprecision

#include <TString.h>
#include <TTree.h>
#include <TFile.h>
#include <TH1F.h>
#include <TCanvas.h>
#include <TMath.h>

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
 *   - look into dlopen (probable dyn lib linking path mismatch)
 */

class InputData {

typedef std::map<std::string, std::vector<std::string> > StringMap;

public:
	InputData(StringMap sbdFiles, std::string dir, TString tree)
		: sbdFiles(sbdFiles), dir(dir), tree(tree) { }
	~InputData() {
		sbdFiles.clear();
		dir = "";
		tree = "";
	}
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
	StringMap 	sbdFiles;
	std::string dir;
	TString 	tree;
};

class FilePointer {
public:
	FilePointer(const std::shared_ptr<InputData> & input, std::string key) : input(input), key(key) { }
	~FilePointer() {
		//input.reset(); // not necessary, actually
		key = "";
	}
protected:
	std::shared_ptr<InputData>	input;
	std::string key;
};

class MultipleFilePointer : public FilePointer {
public:
	MultipleFilePointer(const std::shared_ptr<InputData> & input, std::string key) : FilePointer(input, key) { }
	~MultipleFilePointer() {
		this -> clear();
	}
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
	void openAllTrees() { // loads all TTree's of particular type (sig/bkg/data) into memory
		for(auto & kv: files) {
			if (kv.second -> IsOpen()) {
				std::unique_ptr<TTree> temp(dynamic_cast<TTree *> (kv.second -> Get(input -> getTreeName())));
				if(temp) trees.emplace(kv.first, std::move(temp));
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
	TTree * getTree(int n) const {
		auto it = trees.begin();
		std::advance(it, n);
		return (*it).second.get();
	}
	TFile * getFile(int n) const {
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
	SingleFilePointer(const std::shared_ptr<InputData> & input, std::string key) : FilePointer(input, key) { }
	~SingleFilePointer() {
		this -> reset();
		fileName = "";
	}
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
	void openTree() { // loads TTree into memory
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
		if(tree != NULL) {
			tree.reset();
		}
		if(file != NULL) {
			file -> Close();
			file.reset();
		}
	}
	bool hasNext() const {
		return input -> getLength(key) - counter > 0;
	}
	void reset() {
		this -> close();
		counter = 0;
	}
	const std::string getFileName() const {
		return fileName;
	}
	TTree * getTree() const {
		return tree.get();
	}
	TFile * getFile() const {
		return file.get();
	}
	int getLength() const {
		return input -> getLength(key);
	}
	void next() {
		++counter;
	}
	// pre-increment
	SingleFilePointer& operator++() {
		++counter;
		return (*this);
	}
	// post-increment
	void operator++(int) {
		++(*this);
	}
private:
	int 					counter = 0;
	std::unique_ptr<TFile> 	file;
	std::unique_ptr<TTree> 	tree;
	std::string 			fileName;
};

std::string trim(std::string);
InputData * parse(int, char **);

int main(int argc, char ** argv) {
	std::shared_ptr<InputData> input(parse(argc, argv));
	
	SingleFilePointer sigPointers(input, "signal");
	sigPointers.openFile();
	sigPointers.openTree();
	
	// NB! THE FOLLOWING CODE SERVES AS A PROTOTYPE FOR THE ACTUAL SOLUTION TO THE PROBLEM
	std::vector<std::pair<Float_t, Float_t> > pt_ranges;
	std::vector<std::pair<Float_t, Float_t> > eta_ranges;
	std::vector<Float_t> flavor_ranges;
	
	// to config file -> InputData (shouldn't be hardcoded)
	pt_ranges.push_back(std::make_pair(20.0, 30.0));
	pt_ranges.push_back(std::make_pair(30.0, 40.0));
	pt_ranges.push_back(std::make_pair(40.0, 60.0));
	pt_ranges.push_back(std::make_pair(60.0, 100.0));
	pt_ranges.push_back(std::make_pair(100.0, 160.0));
	// how else should I treat inf?
	pt_ranges.push_back(std::make_pair(160.0, std::numeric_limits<float>::max()));
	
	eta_ranges.push_back(std::make_pair(0, 0.8));
	eta_ranges.push_back(std::make_pair(0.8, 1.6));
	eta_ranges.push_back(std::make_pair(1.6, 2.5));
	
	// add jet tags based on the initial quark id's
	flavor_ranges.push_back(11.0); // electron
	flavor_ranges.push_back(13.0); // muon
	
	TFile * f = new TFile("histos.root", "recreate");
	f -> cd();
	TTree * tree = sigPointers.getTree(); // using single file atm
	for(int i = 1; i <= 1; ++i) { // loop over "jets"
		Float_t lept_pt; // 	kinematic variable
		Float_t lept_eta; // 	kinematic variable
		Float_t lept_pdgid; // 	particle id
		Float_t lept_sip; // 	"CSV"
		tree -> SetBranchAddress(std::string("f_lept"+std::to_string(i)+"_pt").c_str(), &lept_pt);
		tree -> SetBranchAddress(std::string("f_lept"+std::to_string(i)+"_eta").c_str(), &lept_eta);
		tree -> SetBranchAddress(std::string("f_lept"+std::to_string(i)+"_pdgid").c_str(), &lept_pdgid);
		tree -> SetBranchAddress(std::string("f_lept"+std::to_string(i)+"_sip").c_str(), &lept_sip);
		Long64_t nEntries = tree -> GetEntries();
		// loop over ranges
		for(const auto & pt: pt_ranges) { // over pt-s
			Float_t min_pt = pt.first;
			Float_t max_pt = pt.second;
			for(const auto & eta: eta_ranges) { // over etas
				Float_t min_eta = eta.first;
				Float_t max_eta = eta.second;
				for(const auto & flavor: flavor_ranges) { // over flavors
					// create the histogram
					auto f2str = [] (float f) -> std::string {
						std::string strf = std::to_string(f);
						return strf.substr(0, strf.find('.') + 2);
					};
					auto flavor2str = [] (float flavor) -> std::string {
						if(TMath::AreEqualRel(flavor, 11.0, 0.1) == kTRUE) 	return "e";
						else 												return "mu";
					};
					auto maxPt2str = [f2str] (float max_pt) -> std::string {
						if(max_pt == std::numeric_limits<float>::max()) return "inf";
						else											return f2str(max_pt);
					};
					auto floats2charName = [f2str,maxPt2str,flavor2str] (float flavor, float max_pt, float min_pt, float max_eta, float min_eta) -> std::string {
						std::string name = "csv_" + flavor2str(flavor) + "_[" + f2str(min_pt) + ",";
						name.append(maxPt2str(max_pt) + "]_[" + f2str(min_eta) + "," + f2str(max_eta) + "]");
						return name;
					};
					auto floats2charTitle = [f2str,maxPt2str,flavor2str] (float flavor, float max_pt, float min_pt, float max_eta, float min_eta) -> std::string {
						std::string title = "CSV " + flavor2str(flavor) + " pt=[" + f2str(min_pt) + "," + maxPt2str(max_pt) + "] ";
						title.append("eta=[" + f2str(min_eta) + "," + f2str(max_eta) + "]");
						return title;
					};
					std::string name = floats2charName(flavor, max_pt, min_pt, max_eta, min_eta);
					std::string title = floats2charTitle(flavor, max_pt, min_pt, max_eta, min_eta);
					//std::cout << name.c_str() << std::endl;
					//std::cout << title << std::endl;
					//std::cout << std::boolalpha << lept_pdgid == flavor << std::endl;
					TH1F * histo = new TH1F(TString(name.c_str()), TString(title.c_str()),100, -4.0, 4.0);
					histo -> SetDirectory(f);
					for(Long64_t i = 0; i < nEntries; ++i) { // over all events (ntuples??)
						tree -> GetEntry(i);
						bool condition = (TMath::AreEqualRel(lept_pdgid, flavor, 0.1) == kTRUE) && lept_eta >= min_eta && lept_eta < max_eta;
						condition = condition && lept_pt >= min_pt && lept_pt < max_pt;
						//std::cout << lept_pdgid << std::endl;
						if(condition) histo -> Fill(lept_sip);
					}
					histo -> Write();
					
				}
			}
		}
		
	}
	//f -> Write();
	f -> Close();
	sigPointers.close();
	
	return EXIT_SUCCESS;
}

std::string trim(std::string s) {
	s = s.substr(0, s.find(";")); // lose the comment
	boost::algorithm::trim(s); // lose whitespaces around the string
	return s;
}

InputData * parse(int argc, char ** argv) {
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
	return new InputData(sbdFiles, dir, tree);
}