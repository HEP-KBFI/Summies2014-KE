#pragma once

#include <map> // std::map
#include <string> // std::string
#include <vector> // std::vector
#include <utility> // std::pair

#include <TString.h>

class InputData {

public:
	InputData(int argc, char ** argv);
	~InputData();
	const std::string getDir() const; // directory name of the input files
	const TString getTreeName() const; // tree name
	const TString getXval() const; // branch name of the variable on the x-axis
	const TString getVar() const; // y-axis of the histogram
	const TString getXname() const; // x-axis of the histogram
	const TString getWeightvar() const; // branch name of the weight variable
	const TString getHname() const; // name of the histogram files
	Int_t getBins() const; // number of bins
	const Ranges getVarRanges() const; // variable ranges
	Float_t getMinX() const; // min value of x
	Float_t getMaxX() const; // max value of x
	bool hasFlavor(Float_t flavorNumber) const;
	const TString getFlavor(Float_t code) const; // returns flavor name (for the histogram)
	const TString getFlavorVar() const; // returns the branch name for the flavor
	const std::vector<Float_t> getFlavorCodes() const; // returns flavors of interest
	const Ranges getRanges() const; // returns all ranges as a map
	const std::vector<TString> getVarNames() const; // returns all names for the variables
	const std::vector<std::pair<Float_t, Float_t> > getRange(TString name) const;
	
	const std::vector<std::string> & getFileNames(std::string key) const;
	int getNumberOfFiles(std::string key) const;
private:
	std::string dir;
	TString 	tree;
	TString 	var;
	TString		xval;
	TString 	xname;
	TString		weightvar;
	TString		hname;
	Int_t 		bins;
	Ranges		varRanges;
	StringMap	sbdFiles;
	
	std::pair<Float_t, Float_t>					xrange;
	std::pair<TString, std::vector<Float_t> > 	flavors;
	const std::map<Float_t, TString> 			mcNumberScheme =
								{ {1, "d"}, {2, "u"}, {3, "s"},
								{4, "c"}, {5, "b"}, {6, "t"},
								{7, "b'"}, {8, "t'"}, {11, "e"},
								{12, "nu_e"}, {13, "mu"}, {14, "nu_mu"},
								{15, "tau"}, {16, "nu_tau"}, {21, "g"},
								{9, "g"} };
	
	
	void		parse(int argc, char ** argv);
};