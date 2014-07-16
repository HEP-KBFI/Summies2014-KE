#include "InputData.h"

InputData::~InputData() {
	sbdFiles.clear();
	dir = "";
	tree = "";
}

const std::string InputData::getDir() const {
	return dir;
}
const TString InputData::getTreeName() const {
	return tree;
}
const std::vector<std::string> & InputData::getFileNames(std::string key) const {
	return sbdFiles.at(key);
}
int InputData::getLength(std::string key) const {
	return sbdFiles.at(key).size();
}