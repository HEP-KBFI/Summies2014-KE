#pragma once

#include <string> // std::string
#include <TMath.h>

// taken form RTypes.h
#define kRed   632
#define kGreen 416
#define kBlue  600
#define FL_EPS 0.1 // epsilon for flavor comparisons

std::string flavorNames    [3] =    {"c", "b", "light"};
std::string flavorStrings  [3] = 	{"c", "b", "l"};
std::string ptRangeStrings [6] = 	{"[20,30]", "[30,40]", "[40,60]", "[60,100]", "[100,160]", "[160,inf]"};
std::string etaRangeStrings[3] = 	{"[0,0.8]", "[0.8,1.6]", "[1.6,2.5]"};

Int_t colorRanges[3] = {kBlue, kRed, kGreen + 3};
//Int_t XendpointMultisample[3] = {50, 20, 400};
Int_t XendpointMultisample[3] = {400, 400, 400};

std::string getName(int flavorIndex, int ptIndex, int etaIndex, std::string csvString) {
	std::string s = csvString;
	s.append(flavorStrings[flavorIndex]);
	s.append("_");
	s.append(ptRangeStrings[ptIndex]);
	s.append("_");
	s.append(etaRangeStrings[etaIndex]);
	return s;
}

std::string getName(int flavorIndex, int ptIndex, int etaIndex) {
	return getName(flavorIndex, ptIndex, etaIndex, "csv_");
}

std::string getAbbrName(int ptIndex, int etaIndex) {
	return std::string(ptRangeStrings[ptIndex] + "_" + etaRangeStrings[etaIndex]);
}

std::string getHistoTitle(int ptIndex, int etaIndex) {
	std::string title = "";
	std::string ptString = ptRangeStrings[ptIndex];
	std::string etaString = etaRangeStrings[etaIndex];
	title += "p_{t}#in" + ptString.substr(0, ptString.size() - 1) + ") GeV   ";
	title += "|#eta|#in" + etaString.substr(0, etaString.size() - 1) +")";
	return title;
}

std::string getTexTableFormat(int flavorIndex, int ptIndex, int etaIndex) {
	std::string tex = "";
	std::string delim = " & ";
	std::string ptString = ptRangeStrings[ptIndex];
	std::string etaString = etaRangeStrings[etaIndex];
	tex += flavorStrings[flavorIndex] + delim;
	tex += "$" + ptString.substr(0, ptString.size() - 1) + ")$" + delim;
	tex += "$" + etaString.substr(0, etaString.size() - 1) + ")$";
	return tex;
}

std::string getHistoTitle(int flavorIndex, int ptIndex, int etaIndex) {
	std::string title = "";
	std::string ptString = ptRangeStrings[ptIndex];
	std::string etaString = etaRangeStrings[etaIndex];
	title += flavorNames[flavorIndex] + " jet   ";
	title += "p_{t}#in" + ptString.substr(0, ptString.size() - 1) + ") GeV   ";
	title += "|#eta|#in" + etaString.substr(0, etaString.size() - 1) +")";
	return title;
}

int getFlavorIndex(Float_t flavor) {
	if		(TMath::AreEqualAbs(flavor, 4, FL_EPS)) return 0;
	else if	(TMath::AreEqualAbs(flavor, 5, FL_EPS)) return 1;
	else if	(TMath::Abs(flavor) < 4 || TMath::AreEqualAbs(flavor, 21, FL_EPS))	return 2;
	return -1;
}

int getPtIndex(Float_t pt) {
	if		(20.0 <= pt && pt < 30.0) 	return 0;
	else if	(30.0 <= pt && pt < 40.0) 	return 1;
	else if	(40.0 <= pt && pt < 60.0) 	return 2;
	else if (60.0 <= pt && pt < 100.0)	return 3;
	else if (100.0 <= pt && pt < 160.0)	return 4;
	else if (160.0 <= pt) 				return 5;
	return -1;
}

int getEtaIndex(Float_t eta) {
	if		(0.0 <= eta && eta < 0.8)	return 0;
	else if	(0.8 <= eta && eta < 1.6)	return 1;
	else if	(1.6 <= eta && eta < 2.5)	return 2;
	return -1;
}