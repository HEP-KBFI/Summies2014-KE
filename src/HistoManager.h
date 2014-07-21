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
	void closeFile();
	
private:
	Ranges ranges;
	TFile * file;
	std::map<TString, Float_t> branches;
	std::pair<TString, Float_t> flavorVar;
	std::pair<TString, Float_t> xval;
	std::map<TString, TH1F *> histograms;
	const std::shared_ptr<InputData> input;
	
	TString f2s(Float_t f); // float to string
	TString getHistoName(std::map<TString, int> rangeIndices);
	TString getHistoTitle(std::map<TString, int> rangeIndices);
	TString getHistoString(std::map<TString, int> rangeIndices, TString delim);
	int getRangeIndex(TString name, Float_t val) const;
};