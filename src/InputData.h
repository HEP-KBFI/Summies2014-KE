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
	
	const TString getDirName() const; // directory name of the input files
	const TString getTreeName() const; // tree name
	const TString getXVariableName() const; // variable name of the x-axis
	const TString getXTitleName() const; // title of the x-axis
	const TString getWeightVariableName() const; // weight variable name
	const TString getHistogramName() const; // name of the histogram file
	const TString getFlavor(Float_t code) const; // returns flavor name (for the histogram)
	const TString getFlavorVariableName() const; // returns the branch name for the flavor
	const TString getVariableAlias(TString varName); // map of human readable names for the variables
	const std::vector<TString> getVariableNames() const; // returns all names for the variables
	
	Int_t	getBins() const; // number of bins
	Float_t	getMinX() const; // min value of x
	Float_t	getMaxX() const; // max value of x
	bool 	hasFlavor(Float_t flavorNumber) const;
	int		getNumberOfFiles(std::string key) const;
	
	const Ranges getVariableRanges() const; // returns all ranges as a map
	const std::vector<std::pair<Float_t, Float_t> > getRange(TString name) const;
	const std::vector<std::string> & getFileNames(std::string key) const;
private:
	TString		dirName;
	TString 	treeName;
	TString		xVariableName;
	TString 	xTitleName;
	TString		weightVariableName;
	TString		histogramName;
	Int_t 		bins;
	Ranges		variableRanges;
	StringMap	files;
	
	std::map<TString, TString>					variableAliases;
	std::pair<Float_t, Float_t>					xVariableRange;
	std::pair<TString, std::vector<Float_t> > 	supportedFlavors;
	const std::map<Float_t, const TString> 	mcNumberScheme =
	
			{ {1, "d"}, {2, "u"}, {3, "s"}, {4, "c"}, {5, "b"}, {6, "t"},
			{7, "b'"}, {8, "t'"}, {11, "e"}, {12, "nu_e"}, {13, "mu"}, {14, "nu_mu"},
			{15, "tau"}, {16, "nu_tau"}, {21, "g"}, {9, "g"} };
	
	void		parse(int argc, char ** argv);
};