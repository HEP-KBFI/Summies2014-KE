#include <map> // std::map
#include <string> // std::string
#include <vector> // std::vector
#include <iostream> // std::cerr, std::endl
#include <utility> // std::make_pair

#include <TString.h>
#include <TTree.h>
#include <TFile.h>
#include <TH1F.h>

#include "Common.h"
#include "InputData.h"
#include "FilePointer.h"
#include "HistoManager.h"

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
 *   - transition to the files needed for this project
 *   - consider multiple file case
 *   - libEvent.so + MakeClass + MakeSelector + Process or custom PROOF?
 *   - look into dlopen (probable dyn lib linking path mismatch), or just cheat with LD_LIBRARY_PATH
 *   - documentation
 *   - proper error handling?
 *   - logging?
 */

std::string trim(std::string);
InputData * parse(int, char **);

int main(int argc, char ** argv) {
	std::shared_ptr<InputData> input(new InputData(argc, argv));
	
	std::shared_ptr<SingleFilePointer> sigPointers(new SingleFilePointer(input, SIGNAL));
	HistoManager hm(input);
	
	sigPointers -> openFile();
	sigPointers -> openTree();
	//
	
	hm.initRanges();
	hm.createFile("recreate");
	hm.cd();
	hm.process(sigPointers);
	hm.write();
	hm.closeFile();
	//
	sigPointers -> close();
	/*
	
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
	
	TFile * f = new TFile("histos.root", "recreate"); // create single file
	f -> cd(); // cd into it
	TTree * tree = sigPointers -> getTree(); // use single file atm
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
			//histos.at(name) -> Sumw2(); // enable bin errors; ugh.. only error bars remain
		}
		histos.at(name) -> Fill(lept_sip); // fill the histogram
	}
	for(auto & kv: histos) {
		kv.second -> Write();
	}
	
	f -> Close();
	sigPointers -> close();
	*/
	
	return EXIT_SUCCESS;
}



