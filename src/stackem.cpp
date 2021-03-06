#include <boost/program_options.hpp>

#include <cstdlib> // EXIT_SUCCESS, std::exit
#include <iostream> // std::cout, std::cerr, std::endl
#include <vector> // std::vector<>

#include <TFile.h>
#include <TH1F.h>
#include <TKey.h>
#include <TCanvas.h>
#include <TROOT.h>
#include <TClass.h>
#include <THStack.h>
#include <TLegend.h>

// poached from Rtypes.h
#define kWhite 		0
#define kBlack 		1
#define kGray 		920
#define kRed 		632
#define kGreen  	416
#define kBlue 		600
#define kYellow 	400
#define kMagenta 	616
#define kCyan 		432
#define kOrange 	800
#define kSpring 	820
#define kTeal		840
#define kAzure 		860
#define kViolet 	880
#define kPink		900

/*
 * STACK 'EM v.01 !!!!!!!!!
 */

int main(int argc, char ** argv) {
	
	namespace po = boost::program_options;
	
	std::string inFilename, outFilename;
	bool useLogy = false;
	Int_t dimX, dimY;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "prints this message")
			("input,i", po::value<std::string>(&inFilename), "input *.root file")
			("output,o", po::value<std::string>(&outFilename), "output file name")
			("dimX,x", po::value<Int_t>(&dimX) -> default_value(900), "x dimension of the picture")
			("dimY,y", po::value<Int_t>(&dimY) -> default_value(600), "y dimension of the picture")
			("log,l", "sets the y axis to the logarithmic scale")
		;
		
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		
		if(vm.count("input") == 0 || vm.count("output") == 0) {
			std::cout << desc << std::endl;
			std::exit(EXIT_SUCCESS); // ugly
		}
		if(vm.count("log") > 0) {
			useLogy = true;
		}
	}
	catch(std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		std::exit(EXIT_FAILURE); // ugly
	}
	catch(...) {
		std::cerr << "exception of unkown type" << std::endl;
	}
	
	TFile * in = TFile::Open(inFilename.c_str(), "read");
	if(in -> IsZombie() || ! in -> IsOpen()) {
		std::cerr << "Couldn't open " << inFilename << "." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	std::vector<Int_t> colors = { kRed + 2, kRed + 3, kRed + 1, kRed - 7 };
	THStack * stack = new THStack("sh", "Multiplicity of jets");
	TCanvas canvas("c", "canvas", dimX, dimY);
	canvas.SetRightMargin(0.05);
	TLegend * legend = new TLegend(0.8, 0.75, 0.9, 0.9);
	TIter next(in -> GetListOfKeys());
	TKey * key;
	int histoIndex = 0;
	std::vector<TH1F *> histoVector;
	std::map<Double_t, int> histoIndices;
	while((key = dynamic_cast<TKey *>(next()))) {
		TClass * cl = gROOT -> GetClass(key -> GetClassName());
		if(! cl -> InheritsFrom("TH1F")) continue;
		TH1F * h = dynamic_cast<TH1F *> (key -> ReadObj());
		histoVector.push_back(h);
		histoIndices[h -> GetEntries()] = histoIndex++;
	}
	histoIndex = 1;
	Float_t nominator = 107.66 * 19.5 * 1000;
	for(auto kv: histoIndices) {
		//histoVector[kv.second] -> Scale(nominator / histoVector[kv.second] -> Integral());
		histoVector[kv.second] -> SetFillColor(colors[histoIndex++]);
		//histoVector[kv.second] -> SetLineWidth(1);
		histoVector[kv.second] -> SetLineColor(colors[histoIndex]);
		stack -> Add(histoVector[kv.second]);
		legend -> AddEntry(histoVector[kv.second], histoVector[kv.second] -> GetName());
	}
	if(useLogy) {
		canvas.SetLogy(1);
		stack -> SetMinimum(1);
	}
	else {
		stack -> SetMinimum(0);
	}
	stack -> Draw();
	legend -> Draw();
	stack -> GetXaxis() -> SetTitle("nr jets");
	stack -> GetYaxis() -> SetTitle("Events");
	canvas.Update();
	canvas.SaveAs(outFilename.c_str());
	in -> Close();
	
	return EXIT_SUCCESS;
}