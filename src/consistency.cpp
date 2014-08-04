#include <string>
#include <cstdlib>

#include <TFile.h>
#include <TH1F.h>

int main(void) {
	TFile * f = TFile::Open("res/TT_csvGen_histograms.root");
	TFile * g = TFile::Open("res/TT_csv_histograms.root");
	TFile * o = new TFile("out.root", "recreate");
	TH1F * fh = dynamic_cast<TH1F *> (f -> Get("csv_b_[160,inf]_[1.6,2.5]"));
	TH1F * gh = dynamic_cast<TH1F *> (g -> Get("csv_b_[160,inf]_[1.6,2.5]"));
	fh -> Add(gh, -1);
	fh -> Scale(1.0 / gh -> Integral());
	fh -> SetDirectory(o);
	fh -> Write();
	f -> Close();
	g -> Close();
	o -> Close();
	return EXIT_SUCCESS;
}