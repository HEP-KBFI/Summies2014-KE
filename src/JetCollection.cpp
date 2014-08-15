#include "JetCollection.hpp"

JetCollection::JetCollection() { }
void JetCollection::add (Int_t nJets, Float_t * pt, Float_t * eta, Float_t * flavor, Float_t * csv, std::string type) {
	for(Int_t i = 0; i < nJets; ++i) {
		jets.push_back(Jet(pt[i], eta[i], flavor[i], csv[i], i, type));
	}
}
void JetCollection::add(Jet j) {
	jets.push_back(j);
}
std::vector<Jet>::iterator JetCollection::begin() { return jets.begin(); }
std::vector<Jet>::iterator JetCollection::end() { return jets.end(); }
std::vector<Jet>::const_iterator JetCollection::begin() const { return jets.begin(); }
std::vector<Jet>::const_iterator JetCollection::end() const { return jets.end(); }
void JetCollection::sortPt() {
	std::sort(jets.begin(), jets.end(),
		[] (Jet J1, Jet J2) -> bool {
			return J1.getPt() > J2.getPt();
		}
	);
}
Jet & JetCollection::getJet(int i) {
	return jets[i];
}
std::size_t JetCollection::size() const {
	return jets.size();
}