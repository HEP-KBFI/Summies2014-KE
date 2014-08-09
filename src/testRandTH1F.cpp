#include <iostream> // std::cout, std::endl;

#include "RandTH1F.hpp"

int main() {
	RandTH1F r("res/TT_csv_cumulative.root");
	r.openFile();
	r.loadHistograms();
	auto keys = r.getKeys();
	for(auto name: keys) {
		r.generateRandom(name, 1e5);
		std::cout << name << std::endl;
	}
	return 0;
}