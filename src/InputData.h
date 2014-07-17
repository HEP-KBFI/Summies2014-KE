#pragma once

#include <map> // std::map
#include <string> // std::string
#include <vector> // std::vector
#include <utility> // std::pair

#include <TString.h>

class InputData {

typedef std::map<std::string, std::pair<Float_t, Float_t> > Ranges;
typedef std::map<std::string, std::vector<std::string> > 	StringMap;

public:
	InputData(int argc, char ** argv);
	~InputData();
	const std::string getDir() const;
	const TString getTreeName() const;
	
	const std::vector<std::string> & getFileNames(std::string key) const;
	int getLength(std::string key) const;
private:
	std::string dir;
	TString 	tree;
	TString 	var;
	TString		xval;
	TString 	xname;
	TString		weightvar;
	int 		bins;
	Ranges		varRanges;
	StringMap	sbdFiles;
	std::pair<Float_t, Float_t>	xrange;
	
	std::string trim(std::string s);
	void		parse(int argc, char ** argv);
};