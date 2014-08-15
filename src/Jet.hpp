#pragma once

#include <string> // std::string
#include <iostream> // std::ostream

#include <TMath.h>

class Jet {
public:
	Jet(Float_t pt, Float_t eta, Float_t flavor, Float_t csv, int index, std::string type);
	Float_t getPt() const;
	Float_t getEta() const;
	Float_t getFlavor() const;
	Float_t getCSV() const;
	int getIndex() const;
	std::string getType() const;
	void setName(std::string name);
	std::string getName() const;
	friend std::ostream & operator << (std::ostream &, const Jet &);
	friend bool operator == (const Jet & jetL, const Jet & jetR);
private:
	Float_t pt;
	Float_t eta;
	Float_t flavor;
	Float_t csv;
	int index;
	std::string type;
	std::string name;
};