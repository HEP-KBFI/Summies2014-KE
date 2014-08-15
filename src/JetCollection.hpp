#pragma once

#include <vector> // std::vector<>

#include <TMath.h>

#include "Jet.hpp"

class JetCollection {
public:
	JetCollection();
	void add (Int_t nJets, Float_t * pt, Float_t * eta, Float_t * flavor, Float_t * csv, std::string type);
	void add(Jet j);
	std::vector<Jet>::iterator begin();
	std::vector<Jet>::iterator end();
	std::vector<Jet>::const_iterator begin() const;
	std::vector<Jet>::const_iterator end() const;
	void sortPt();
	Jet & getJet(int i);
	std::size_t size() const;
private:
	std::vector<Jet> jets;
};