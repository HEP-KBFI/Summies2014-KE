#include <vector> // std::vector<>
#include <map> // std::map<>
#include <memory> // std::shared_ptr<>

#include <TFile.h>
#include <TH1F.h>

class RandTH1F {
public:
	RandTH1F(std::string cumulDistFilename);
	~RandTH1F();
	void openFile(void);
	void loadHistograms(void);
	void generateRandom(TString histogramName, Int_t nEntries);
	bool hasNext(TString histogramName);
	Float_t getRandom(TString histogramName);
	std::vector<Float_t> & getRandomVector(TString histogramName);
	std::vector<TString> & getKeys();
private:
	std::string cumulDistFilename;
	std::unique_ptr<TFile> in;
	std::map<TString, std::shared_ptr<TH1F> > histoMap;
	std::map<TString, std::vector<Float_t> > randomValues;
	std::vector<TString> keys;
	std::shared_ptr<TKey> key;
	
	static Int_t bruteSearch(std::shared_ptr<TH1F> h, Double_t r);
	static Float_t randLinpolEdge(std::shared_ptr<TH1F> h, Float_t r,
								  Int_t (* search)(std::shared_ptr<TH1F> h, Double_t r));
};