#include <algorithm> // std::prev_permutation, std::transform
#include <iostream> // std::cout, std::endl
#include <string> // std::string
#include <chrono> // std::chrono
#include <random> // std::mt19937_64, std::uniform_real_distribution<>
#include <vector> // std::vector
#include <cstdlib> // EXIT_SUCCESS
#include <numeric> // std::accumulate


float comb(std::vector<float> & v, int N, int K) {
	std::string bitmask(K, 1); // K leading 1's
	bitmask.resize(N, 0); // N-K trailing 0's
	float sum_prob = 0;
	do {
		float prob = 1;
		for (int i = 0; i < N; ++i) {
			if (bitmask[i]) prob *= v[i];
			else prob *= (1 - v[i]);
		}
		std::cout << prob << std::endl;
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
	
	std::cout << "individual probabilities:" << std::endl;
	for(auto val: v) std::cout << std::fixed << val << "\t";
	std::cout << std::endl << std::endl << "combined probabilities:" << std::endl;
	float combi = comb(v, Nj, Ntag);
	std::cout << std::endl << "the sum of combined probability: \t";
	std::cout << combi << std::endl;
	
	return EXIT_SUCCESS;
}