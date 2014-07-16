#pragma once

#include <map> // std::map
#include <string> // std::string
#include <vector> // std::vector

#include <TString.h>

class InputData {

typedef std::map<std::string, std::vector<std::string> > StringMap;

public:
	InputData(StringMap sbdFiles, std::string dir, TString tree)
		: sbdFiles(sbdFiles), dir(dir), tree(tree) { }
	~InputData();
	const std::string getDir() const;
	const TString getTreeName() const;
	const std::vector<std::string> & getFileNames(std::string key) const;
	int getLength(std::string key) const;
private:
	StringMap 	sbdFiles;
	std::string dir;
	TString 	tree;
};