#include "Jet.hpp"

Jet::Jet(Float_t pt, Float_t eta, Float_t flavor, Float_t csv, int index, std::string type)
	: pt(pt), eta(eta), flavor(flavor), csv(csv), index(index), type(type) { }
Float_t Jet::getPt() const { return pt; }
Float_t Jet::getEta() const { return eta; }
Float_t Jet::getFlavor() const { return flavor; }
Float_t Jet::getCSV() const { return csv; }
int Jet::getIndex() const { return index; }
std::string Jet::getType() const { return type; }
void Jet::setName(std::string name) { this -> name = name; }
std::string Jet::getName() const { return name; }

std::ostream & operator << (std::ostream & stream, const Jet & jet) {
	stream << "jet pt: " << jet.getPt() << std::endl;
	stream << "jet eta: " << jet.getEta() << std::endl;
	stream << "jet flavor: " << jet.getFlavor() << std::endl;
	stream << "jet CSV: " << jet.getCSV() << std::endl;
	return stream;
}

bool operator == (const Jet & jetL, const Jet & jetR) {
	bool returnValue = true;
	returnValue = returnValue && TMath::AreEqualAbs(jetL.flavor, jetR.flavor, 0.1);
	returnValue = returnValue && TMath::AreEqualAbs(jetL.eta, jetR.eta, 1e-6);
	returnValue = returnValue && TMath::AreEqualAbs(jetL.pt, jetR.pt, 1e-6);
	returnValue = returnValue && TMath::AreEqualAbs(jetL.csv, jetR.csv, 1e-6);
	return returnValue;
}