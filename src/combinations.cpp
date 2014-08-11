#include <algorithm>
#include <iostream>
#include <string>
#include <chrono>
#include <random>
 
float comb(std::vector<float> & v, int N, int K) {
	std::string bitmask(K, 1); // K leading 1's
	bitmask.resize(N, 0); // N-K trailing 0's
	float sum_prob = 0;
	do {
		float prob = 1;
		for (int i = 0; i < N; ++i) {
			if (bitmask[i]) prob *= v[i];
		}
		sum_prob += prob;
	} while (std::prev_permutation(bitmask.begin(), bitmask.end()));
	return sum_prob;
}
 
int main(void) {
	int Nj = 4, Ntag = 2;
	
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937_64 gen(seed);
	std::uniform_real_distribution<float> dis(0,1);
	std::vector<float> v;
	for(int i = 1; i <= Nj; ++i) v.push_back(dis(gen));
	for(auto val: v) std::cout << val << "\t";
	std::cout << std::endl << comb(v, Nj, Ntag) << std::endl;
	
	return 0;
}