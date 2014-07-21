#include "Common.h"
#include "InputData.h"
#include "FilePointer.h"

#include <memory> // std::shared_ptr

#include <TString.h>
#include <TH1F.h>
#include <TFile.h>

class HistoManager {

public:
	HistoManager(const std::shared_ptr<InputData> & input) : input(input) { }
	~HistoManager() { }
	
	void initRanges();
	void createFile(TString option);
	void cd();
	void process(std::shared_ptr<SingleFilePointer> & sfp);
	void write();
	void close();
private:
	std::unique_ptr<TFile> file;
	std::shared_ptr<InputData> input;
	
	std::pair<TString, Float_t> flavorVar;
	std::pair<TString, Float_t> xVar;
	Ranges variableRanges;
	
	std::map<TString, std::unique_ptr<TH1F> >  histograms;
	std::map<TString, Float_t> branchReferences;
	
	TString f2s(Float_t f); // float to string
	TString getHistoName(std::map<TString, int> rangeIndices);
	TString getHistoTitle(std::map<TString, int> rangeIndices);
	TString getHistoString(std::map<TString, int> rangeIndices, TString delim);
	int 	getRangeIndex(TString name, Float_t val) const;
};