#pragma once

#include <memory> // std::shared_ptr, std::unique_ptr
#include <string> // std::string
#include <map> // std::map

#include <TFile.h>
#include <TTree.h>
#include <TString.h>

#include "InputData.h"

class FilePointer {
public:
	FilePointer(const std::shared_ptr<InputData> & input, std::string key) : input(input), key(key) { }
	~FilePointer();
	virtual void reset() = 0;
protected:
	std::shared_ptr<InputData>	input;
	std::string key;
};

class MultipleFilePointer : public FilePointer {
public:
	MultipleFilePointer(const std::shared_ptr<InputData> & input, std::string key) : FilePointer(input, key) { }
	~MultipleFilePointer();
	void openAllFiles();
	void openAllTrees();
	void reset();
	TTree * getTree(int n) const;
	TFile * getFile(int n) const;
	int getLength() const;
private:
	std::map<TString, std::unique_ptr<TFile> > files;
	std::map<TString, std::unique_ptr<TTree> > trees;
};

class SingleFilePointer : public FilePointer {
public:
	SingleFilePointer(const std::shared_ptr<InputData> & input, std::string key) : FilePointer(input, key) { }
	~SingleFilePointer();
	void openFile();
	void openTree();
	void close();
	bool hasNext() const;
	void reset();
	const std::string getFileName() const;
	TTree * getTree() const;
	TFile * getFile() const;
	int getLength() const;
	void next();
	SingleFilePointer& operator++();// pre-increment
	void operator++(int); // post-increment
private:
	int 					counter = 0;
	std::unique_ptr<TFile> 	file;
	std::unique_ptr<TTree> 	tree;
	std::string 			fileName;
};