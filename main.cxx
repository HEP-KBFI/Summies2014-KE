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

#define INF std::numeric_limits<float>::max()

/**
 * @note
 *   - boost linked statically, root dynamically
 *   - assumptions -- the same name for a TTree for all input files
 *         -# the same name for a TTree for all input files
 *         -# single file to work with
 * 
 * @todo
 *   - parse ranges from the config file
 *   - split different classes into separate files, keep only main() in the current file
 *   - transition to 
 *   - consider multiple file case
 *   
 *   - look into dlopen (probable dyn lib linking path mismatch), or just cheat with LD_LIBRARY_PATH
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
	std::map<Float_t, std::string> flavor_ranges;
	
	// to config file -> InputData (shouldn't be hardcoded)
	
	pt_ranges.push_back(std::make_pair(20.0, 30.0));
	pt_ranges.push_back(std::make_pair(30.0, 40.0));
	pt_ranges.push_back(std::make_pair(40.0, 60.0));
	pt_ranges.push_back(std::make_pair(60.0, 100.0));
	pt_ranges.push_back(std::make_pair(100.0, 160.0));
	// how else should I treat inf?
	pt_ranges.push_back(std::make_pair(160.0, INF));
	
	eta_ranges.push_back(std::make_pair(0, 0.8));
	eta_ranges.push_back(std::make_pair(0.8, 1.6));
	eta_ranges.push_back(std::make_pair(1.6, 2.5));
	
	// add jet tags based on the initial quark id's
	// (why are they floats ... ?)
	flavor_ranges[11.0] = "e"; // electron
	flavor_ranges[13.0] = "mu"; // muon
	
	/*
	 * loop over events once
	 * loop over ranges
	 * create name for the histo
	 * push a new entry to the map if it's not present
	 * add events
	 * ...
	 * profit
	 */
	
	TFile * f = new TFile("histos.root", "recreate"); // create single file
	f -> cd(); // cd into it
	TTree * tree = sigPointers.getTree(); // use single file atm
	std::map<std::string, TH1F *> histos; // map of histograms
	// LOOP OVER FILES?
	Long64_t nEntries = tree -> GetEntries(); // number of entries in the tree
	// associate the variables with the ones in the tree
	// does each jet has its own set of kinematic variables? probably so
	Float_t lept_pt; // 	kinematic variable
	Float_t lept_eta; // 	kinematic variable
	Float_t lept_pdgid; // 	particle id
	Float_t lept_sip; // 	"CSV"
	tree -> SetBranchAddress(std::string("f_lept1_pt").c_str(), &lept_pt);
	tree -> SetBranchAddress(std::string("f_lept1_eta").c_str(), &lept_eta);
	tree -> SetBranchAddress(std::string("f_lept1_pdgid").c_str(), &lept_pdgid);
	tree -> SetBranchAddress(std::string("f_lept1_sip").c_str(), &lept_sip);
	auto rangeLookup = [] (const std::vector<std::pair<Float_t, Float_t> > & ranges, Float_t f) -> int { // define a lookup macro
		int index = 0;
		for(const auto & kv: ranges) {
			if(f >= kv.first && f <= kv.second) return index;
			++index;
		}
		return -1; // if not in the range
	};
	auto f2str = [] (float f) -> std::string {
		std::string strf = std::to_string(f);
		return strf.substr(0, strf.find('.') + 2);
	};
	auto maxPt2str = [f2str] (float max_pt) -> std::string {
		if(max_pt == INF)	return "inf";
		else				return f2str(max_pt);
	};
	auto floats2charName = [f2str,maxPt2str] (std::string flavor, float min_pt, float max_pt, float min_eta, float max_eta) -> std::string {
		std::string name = "sip_" + flavor + "_[" + f2str(min_pt) + ",";
		name.append(maxPt2str(max_pt) + "]_[" + f2str(min_eta) + "," + f2str(max_eta) + "]");
		return name;
	};
	auto floats2charTitle = [f2str,maxPt2str] (std::string flavor, float min_pt, float max_pt, float min_eta, float max_eta) -> std::string {
		std::string title = "SIP " + flavor + " pt=[" + f2str(min_pt) + "," + maxPt2str(max_pt) + "] ";
		title.append("eta=[" + f2str(min_eta) + "," + f2str(max_eta) + "]");
		return title;
	};
	
	for(Long64_t i = 0; i < nEntries; ++i) { // loop over events once
		tree -> GetEntry(i);
		// CONDITION FOR JETS
		if(flavor_ranges.count(lept_pdgid) == 0) continue; // not in range
		int iPt = rangeLookup(pt_ranges, lept_pt);
		if(iPt == -1) continue; // not in range
		int iEta = rangeLookup(eta_ranges, lept_eta);
		if(iEta == -1) continue; // not in range
		std::string name = floats2charName(flavor_ranges[lept_pdgid], pt_ranges[iPt].first, pt_ranges[iPt].second, eta_ranges[iEta].first, eta_ranges[iEta].second);
		std::string title = floats2charTitle(flavor_ranges[lept_pdgid], pt_ranges[iPt].first, pt_ranges[iPt].second, eta_ranges[iEta].first, eta_ranges[iEta].second);
		if(histos.count(name) == 0) {
			histos[name] = new TH1F(TString(name.c_str()), TString(title.c_str()),100, -4.0, 4.0); // histo ranges to config file!
			histos.at(name) -> SetDirectory(f);
		}
		histos.at(name) -> Fill(lept_sip); // fill the histogram
	}
	for(auto & kv: histos) {
		kv.second -> Write();
	}
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