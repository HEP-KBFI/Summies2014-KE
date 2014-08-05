#include <cstdlib> //EXIT_SUCCESS, std::abs
#include <iostream> // std::cout
#include <map> // std::map<>
#include <cmath> // std::fabs
#include <vector> // std::vector<>

#include <TFile.h>
#include <TTree.h>

int main(void) {
	
	/*********** input ******************************************/
	
	// std::string inFilename = "/hdfs/cms/store/user/liis/TTH_Ntuples_jsonUpdate/DiJetPt_TTJets_SemiLeptMGDecays_8TeV-madgraph.root"
	std::string inFilename = "res/TT100k.root";
	std::string treeName = "tree";
	
	/*********** jets *******************************************/
	
	int maxNumberOfHJets = 2;
	int maxNumberOfAJets = 20;
	
	Int_t nhJets;
	Int_t naJets;
	
	Float_t hJet_pt[maxNumberOfHJets];
	Float_t hJet_eta[maxNumberOfHJets];
	Float_t hJet_csv[maxNumberOfHJets];
	Float_t hJet_flavour[maxNumberOfHJets];
	//Float_t hJet_phi[maxNumberOfHJets];
	//Float_t hJet_e[maxNumberOfHJets];
	//Float_t hJet_genPt[maxNumberOfHJets];
	Float_t aJet_pt[maxNumberOfAJets];
	Float_t aJet_eta[maxNumberOfAJets];
	Float_t aJet_csv[maxNumberOfAJets];
	Float_t aJet_flavour[maxNumberOfAJets];
	//Float_t aJet_phi[maxNumberOfAJets];
	//Float_t aJet_e[maxNumberOfAJets];
	//Float_t aJet_genPt[maxNumberOfAJets];
	
	//Float_t hJet_csvGen[maxNumberOfHJets]; // SAMPLING
	//Float_t aJet_csvGen[maxNumberOfAJets]; // SAMPLING
	
	//Long64_t hJet_csvN[maxNumberOfHJets]; // SAMPLING
	//Long64_t aJet_csvN[maxNumberOfAJets]; // SAMPLING
	
	/*********** sample vars ************************************/
	
	//Float_t hJet_csvGen[maxNumberOfHJets];
	//Float_t aJet_csvGen[maxNumberOfAJets];
	
	//Long64_t hJet_csvN[maxNumberOfHJets];
	//Long64_t aJet_csvN[maxNumberOfAJets];
	
	/*********** leptons ****************************************/
	
	int maxVLeptons = 2;
	int maxALeptons = 100; // there were up to40 leptons in the first 100k events
	
	Int_t nvlep;
	Int_t nalep;
	
	Float_t vLepton_pt[maxVLeptons];
	Float_t vLepton_eta[maxVLeptons];
	Float_t vLepton_pfCombRelIso[maxVLeptons];
	Int_t vLepton_type[maxVLeptons];
	//Float_t vLepton_id80[maxVLeptons];
	//Float_t vLepton_id95[maxVLeptons];
	//Float_t vLepton_charge[maxVLeptons];
	Float_t vLepton_idMVAtrig[maxVLeptons];
	//Float_t vLepton_idMVAnotrig[maxVLeptons];
	//Float_t vLepton_idMVApresel[maxVLeptons];
	//Float_t vLepton_particleIso[maxVLeptons];
	//Float_t vLepton_dxy[maxVLeptons];
	//Float_t vLepton_innerHits[maxVLeptons];
	
	Float_t aLepton_pt[maxALeptons];
	Float_t aLepton_eta[maxALeptons];
	Float_t aLepton_pfCombRelIso[maxALeptons];
	Int_t aLepton_type[maxALeptons];
	//Float_t aLepton_id80[maxALeptons];
	//Float_t aLepton_id95[maxALeptons];
	//Float_t aLepton_charge[maxALeptons];
	Float_t aLepton_idMVAtrig[maxALeptons];
	//Float_t aLepton_idMVAnotrig[maxALeptons];
	//Float_t aLepton_idMVApresel[maxALeptons];
	//Float_t aLepton_particleIso[maxALeptons];
	//Float_t aLepton_dxy[maxALeptons];
	//Float_t aLepton_innerHits[maxALeptons];
	
	/*********** open files *************************************/
	
	TFile * in = TFile::Open(inFilename.c_str(), "read");
	TTree * t = dynamic_cast<TTree *> (in -> Get(treeName.c_str()));
	
	/*********** jet branches ***********************************/
	
	t -> SetBranchAddress("nhJets", &nhJets);
	t -> SetBranchAddress("hJet_pt", &hJet_pt);
	t -> SetBranchAddress("hJet_eta", &hJet_eta);
	t -> SetBranchAddress("hJet_flavour", &hJet_flavour);
	//t -> SetBranchAddress("hJet_phi", &hJet_phi);
	//t -> SetBranchAddress("hJet_e", &hJet_e);
	//t -> SetBranchAddress("hJet_genPt", &hJet_genPt);
	//t -> SetBranchAddress("hJet_csvGen", &hJet_csvGen); // SAMPLING
	//t -> SetBranchAddress("hJet_csvN", &hJet_csvN); // SAMPLING
	t -> SetBranchAddress("hJet_csv", &hJet_csv);
	
	t -> SetBranchAddress("naJets", &naJets);
	t -> SetBranchAddress("aJet_pt", &aJet_pt);
	t -> SetBranchAddress("aJet_eta", &aJet_eta);
	t -> SetBranchAddress("aJet_flavour", &aJet_flavour);
	//t -> SetBranchAddress("aJet_phi", &aJet_phi);
	//t -> SetBranchAddress("aJet_e", &aJet_e);
	//t -> SetBranchAddress("aJet_genPt", &aJet_genPt);
	//t -> SetBranchAddress("aJet_csvGen", &aJet_csvGen); // SAMPLING
	//t -> SetBranchAddress("aJet_csvN", &aJet_csvN); // SAMPLING
	t -> SetBranchAddress("aJet_csv", &aJet_csv);
	
	/*********** lepton branches ********************************/
	
	t -> SetBranchAddress("nvlep", &nvlep);
	t -> SetBranchAddress("nalep", &nalep);
	
	t -> SetBranchAddress("vLepton_pt", &vLepton_pt);
	t -> SetBranchAddress("aLepton_pt", &aLepton_pt);
	t -> SetBranchAddress("vLepton_eta", &vLepton_eta);
	t -> SetBranchAddress("aLepton_eta", &aLepton_eta);
	t -> SetBranchAddress("vLepton_pfCombRelIso", &vLepton_pfCombRelIso);
	t -> SetBranchAddress("aLepton_pfCombRelIso", &aLepton_pfCombRelIso);
	t -> SetBranchAddress("vLepton_type", &vLepton_type);
	t -> SetBranchAddress("aLepton_type", &aLepton_type);
	t -> SetBranchAddress("vLepton_idMVAtrig", &vLepton_idMVAtrig);
	t -> SetBranchAddress("aLepton_idMVAtrig", &aLepton_idMVAtrig);
	
	/*********** loop over events *******************************/
	
	Long64_t nEvents = 1000;//t -> GetEntries();
	Float_t CSVM = 0.679;
	
	std::string tight = "tight";
	std::string loose = "loose";
	auto findElectronType = [tight,loose] (std::map<std::string, int> & leptons, Float_t pt, Float_t eta, Float_t relIso) {
		if		(pt > 26.0 && std::fabs(eta) < 2.1 && relIso < 0.12) leptons[tight]++;
		else if	(pt > 20.0 && std::fabs(eta) < 2.4 && relIso < 0.20) leptons[loose]++;
	};
	auto findMuonType = [tight,loose] (std::map<std::string, int> & leptons, Float_t pt, Float_t eta, Float_t relIso) {
		if		(pt > 30.0 && std::fabs(eta) < 2.5 && relIso < 0.10) leptons[tight]++;
		else if	(pt > 10.0 && std::fabs(eta) < 2.5 && relIso < 0.15) leptons[loose]++;
	};
	
	int counter = 0;
	
	for(Long64_t i = 0; i < nEvents; ++i) {
		t -> GetEntry(i);
		bool proceed = true;
		
		/********************** lepton cut ************************/
		
		std::map<std::string, int> leptons;
		leptons[tight] = 0;
		leptons[loose] = 0;
		
		for(Int_t vi = 0; vi < nvlep; ++vi) {
			Float_t pt = vLepton_pt[vi];
			Float_t eta = vLepton_eta[vi];
			Float_t relIso = vLepton_pfCombRelIso[vi];
			if(std::abs(vLepton_type[vi]) == 11) { // if electron
				findElectronType(leptons, pt, eta, relIso);
			}
			else if(std::abs(vLepton_type[vi]) == 13) { // if muon
				findMuonType(leptons, pt, eta, relIso);
			}
			if(leptons[tight] > 1) {
				proceed = false;
				break;
			}
			if(leptons[loose] > 0) {
				proceed = false;
				break;
			}
		}
		if(! proceed) continue;
		
		for(Int_t ai = 0; ai < nalep; ++ai) {
			Float_t pt = aLepton_pt[ai];
			Float_t eta = aLepton_eta[ai];
			Float_t relIso = aLepton_pfCombRelIso[ai];
			if(std::abs(vLepton_type[ai]) == 11) { // if electron
				findElectronType(leptons, pt, eta, relIso);
			}
			else if(std::abs(vLepton_type[ai]) == 13) { // if muon
				findMuonType(leptons, pt, eta, relIso);
			}
			if(leptons[tight] > 1) {
				proceed = false;
				break;
			}
			if(leptons[loose] > 0) {
				proceed = false;
				break;
			}
		}
		if(! proceed) continue;
		
		/********************** cut them jets ****************************/
		if(nhJets + naJets < 5) continue;
		
		std::string aJets = "aJets";
		std::string hJets = "hJets";
		std::map<std::string, std::vector<Int_t> > validJets;
		std::map<std::string, std::vector<Int_t> > passedWP;
		for(Int_t nh = 0; nh < nhJets; ++nh) {
			if(hJet_pt[nh] > 30.0 && std::fabs(hJet_eta[nh]) < 2.5) {
				validJets[hJets].push_back(nh);
				if(hJet_csv[nh] >= CSVM) {
					passedWP[hJets].push_back(nh);
				}
			}
		}
		for(Int_t ah = 0; ah < naJets; ++ah) {
			if(aJet_pt[ah] > 30.0 && std::fabs(aJet_eta[ah]) < 2.5) {
				validJets[aJets].push_back(ah);
				if(aJet_csv[aJets].push_back(ah)) {
					passedWP[aJets].push_back(ah);
				}
			}
		}
		if(validJets[hJets].size() + validJets[aJets] < 5) continue;
		if(passedWP[hJets].size() + passedWP[hJets].size() < 2) continue;
		
		
	}
	std::cout << counter << std::endl;
	/*********** close everything *******************************/
	
	in -> Close();
	
	return EXIT_SUCCESS;
}