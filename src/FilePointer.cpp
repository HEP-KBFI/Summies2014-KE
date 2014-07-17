#include "FilePointer.h"

#include <iterator> // std::advance

FilePointer::~FilePointer() {
	//input.reset(); // not necessary, actually
	key = "";
}

MultipleFilePointer::~MultipleFilePointer() {
	this -> reset();
}
void MultipleFilePointer::openAllFiles() {
	const auto stringList = input -> getFileNames(key);
	for(const std::string label: stringList) {
		std::string path = input -> getDir();
		path.append(label + ".root");
		files[label] = std::unique_ptr<TFile> (TFile::Open(path.c_str(), "read"));
		if(files[label] -> IsZombie()) {
			std::string msg = "Error on opening the file " + label + ". Abort.\n";
			throw msg;
		}
	}
	return;
}
void MultipleFilePointer::openAllTrees() { // loads all TTree's of particular type (sig/bkg/data) into memory
	for(auto & kv: files) {
		if (kv.second -> IsOpen()) {
			std::unique_ptr<TTree> temp(dynamic_cast<TTree *> (kv.second -> Get(input -> getTreeName())));
			if(temp) trees.emplace(kv.first, std::move(temp));
		}
		else {
			std::string msg = "The file " + std::string(kv.first) + " is not opened. Abort.\n";
			throw msg;
		}
	}
	return;
}
void MultipleFilePointer::reset() {
	trees.clear();
	files.clear();
	return;
}
TTree * MultipleFilePointer::getTree(int n) const {
	auto it = trees.begin();
	std::advance(it, n);
	return (*it).second.get();
}
TFile * MultipleFilePointer::getFile(int n) const {
	auto it = files.begin();
	std::advance(it, n);
	return (*it).second.get();
}
int MultipleFilePointer::getLength() const {
	return trees.size();
}

SingleFilePointer::~SingleFilePointer() {
	this -> reset();
	fileName = "";
}
void SingleFilePointer::openFile() {
	const auto stringList = input -> getFileNames(key);
	auto it = stringList.begin();
	std::advance(it, counter);
	fileName = (*it);
	std::string path = input -> getDir();
	path.append(fileName + ".root");
	file = std::unique_ptr<TFile> (TFile::Open(path.c_str(), "read"));
	if(file -> IsZombie()) {
		std::string msg = "Error on opening the file " + fileName + ". Abort.\n";
		throw msg;
	}
}
void SingleFilePointer::openTree() { // loads TTree into memory
	if (file -> IsOpen()) {
		std::unique_ptr<TObject> temp(file -> Get(input -> getTreeName()));
		tree = std::unique_ptr<TTree> (dynamic_cast<TTree *> (temp.get()));
		if(tree) temp.release();
	}
	else {
		std::string msg = "The file " + fileName + " is not opened. Abort.\n";
		throw msg;
	}
}
void SingleFilePointer::close() {
	if(tree != NULL) {
		tree.reset();
	}
	if(file != NULL) {
		file -> Close();
		file.reset();
	}
}
bool SingleFilePointer::hasNext() const {
	return input -> getLength(key) - counter > 0;
}
void SingleFilePointer::reset() {
	this -> close();
	counter = 0;
}
const std::string SingleFilePointer::getFileName() const {
	return fileName;
}
TTree * SingleFilePointer::getTree() const {
	return tree.get();
}
TFile * SingleFilePointer::getFile() const {
	return file.get();
}
int SingleFilePointer::getLength() const {
	return input -> getLength(key);
}
void SingleFilePointer::next() {
	++counter;
}
SingleFilePointer& SingleFilePointer::operator++() {
	++counter;
	return (*this);
}
void SingleFilePointer::operator++(int) {
	++(*this);
}