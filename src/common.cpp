#include "common.h"

std::ostream& operator<<(std::ostream& os, const Path& path)
{
	for (const auto& state : path)
	{
		os << state.location; // << "(" << state.is_single() << "),";
	}
	return os;
}


bool isSamePath(const Path& p1, const Path& p2)
{
	if (p1.size() != p2.size())
		return false;
	for (unsigned i = 0; i < p1.size(); i++)
	{
		if (p1[i].location != p2[i].location)
			return false;
	}
	return true;
}

template <typename T>
vector<int> sort_indexes(const vector<T>& v) {

	// initialize original index locations
	vector<int> idx(v.size());
	std::iota(idx.begin(), idx.end(), 0);

	// sort indexes based on comparing values in v
	// using std::stable_sort instead of std::sort
	// to avoid unnecessary index re-orderings
	// when v contains elements of equal values 
	std::stable_sort(idx.begin(), idx.end(),
		[&v](int i1, int i2) {return v[i1] < v[i2]; });

	return idx;
}

std::vector<std::string> Split(const std::string& str, char delim, bool only_fisrt) //=false
{
	std::vector<std::string> retVal;
	size_t start = 0;
	size_t delimLoc = str.find_first_of(delim, start);
	while (delimLoc != std::string::npos)
	{
		retVal.emplace_back(str.substr(start, delimLoc - start));
		start = delimLoc + 1;
		delimLoc = str.find_first_of(delim, start);
		if (only_fisrt) {
			retVal.emplace_back(str.substr(start));
			return retVal;
		}
	}
	retVal.emplace_back(str.substr(start));
	return retVal;
}

vector<double> normalize_vector(const vector<double>& to_normalize) //used to normalize feature vec, and dotprod 'scores'
{
	vector<double> retvec(to_normalize.size(), 0);	
	auto result = std::minmax_element(to_normalize.begin(), to_normalize.end());
	auto min_val = result.first;
	auto max_val = result.second;
	if (*max_val - *min_val == 0) {
		return retvec;
	}
	for (int i = 0; i < retvec.size(); i++) {
		retvec[i] = (to_normalize[i] - *min_val) / (*max_val - *min_val);
	}
	return retvec;
}


template <typename V>
void transpose(vector<vector<V> >& b) //in-place
{
	if (b.size() == 0)
		return;
	vector<vector<V> > trans_vec(b[0].size(), vector<V>());
	for (int i = 0; i < b.size(); i++) {
		for (int j = 0; j < b[i].size(); j++) {
			trans_vec[j].push_back(b[i][j]);
		}
	}
	b = trans_vec;
}

vector<vector<double>> normalizeFeatureVectors(vector<vector<double>>& raw_features) {
	assert(raw_features.size() != 0);
	assert(raw_features[0].size() != 0);

	transpose<double>(raw_features);

	vector<vector<double>> norm_features;
	for (int i = 0; i < raw_features.size(); i++) {
		norm_features.push_back(normalize_vector(raw_features[i]));
	}

	transpose<double>(norm_features);
	return norm_features; //TODO: this is expensive. Is there a better way?
}

int getDependencyPairCount(vector<set<int>>& dependency_graph) {
	int pair_count = 0;
	for (int i = 0; i < (int)dependency_graph.size(); i++) {
		pair_count += dependency_graph[i].size();
	}
	return pair_count;
}

vector<int> softmax_ordering(vector<int>& agents_to_arrange, vector<double>& predictions_to_arrange) {
	assert(agents_to_arrange.size() == predictions_to_arrange.size()); //debug
	int num_agents = agents_to_arrange.size();
	vector<int> ordering;
	double curr_total_sum = std::accumulate(predictions_to_arrange.begin(), predictions_to_arrange.end(), 0.0);
	
	while (!agents_to_arrange.empty()) {		
		double running_sum = 0;
		double curr_total_sum = std::accumulate(predictions_to_arrange.begin(), predictions_to_arrange.end(), 0.0);
		double cutoff_value = fRand(0, curr_total_sum);
		for (int k = 0; k < predictions_to_arrange.size(); k++) {
			running_sum += predictions_to_arrange[k];
			if (running_sum >= cutoff_value) {
				ordering.push_back(agents_to_arrange[k]);
				agents_to_arrange.erase(agents_to_arrange.begin() + k);
				predictions_to_arrange.erase(predictions_to_arrange.begin() + k);
				running_sum = 0;
				break;
			}
		}
	}
	return ordering;
}

double fRand(double fMin, double fMax)
{
	double f = (double)rand() / RAND_MAX;
	return fMin + f * (fMax - fMin);
}

void MLRandomRestart(const vector<vector<double>>& all_testfile_dotprod, const vector<vector<int>>& all_testfile_orderings, 
	vector<vector<int>>& orderings, int scen, bool softmax, 
	double bias, double target_sum, double softmax_beta) 
{
	if (scen >= all_testfile_dotprod.size()) { //sanity check
		cout << "scen=" << scen << ", all_testfile_dotprod.size()=" << all_testfile_dotprod.size() << endl;
		return;
	}

	/*double bias = 0.4;
	double target_sum = 100.0;
	double softmax_beta = 0.5;*/
	vector<double> testfile_dotprod;
	if (!softmax) {
		//MIN-MAX normalize all_testfile_dotprod. make them all between 0 and 1
		testfile_dotprod = normalize_vector(all_testfile_dotprod[scen]);
		std::for_each(testfile_dotprod.begin(), testfile_dotprod.end(), [&bias](double& d) { d += bias; });
	}
	else {
		//use softmax function on all_testfile_dotprod[scen]
		testfile_dotprod = softmax_vector<double>(all_testfile_dotprod[scen], softmax_beta);
	}
	double sum = std::accumulate(testfile_dotprod.begin(), testfile_dotprod.end(), 0.0);
	std::for_each(testfile_dotprod.begin(), testfile_dotprod.end(), [&sum, &target_sum](double& d) { d = d * target_sum / sum; });
	vector<int> agents_to_arrange = all_testfile_orderings[scen];
	vector<double> predictions_to_arrange = testfile_dotprod;

	orderings[scen] = softmax_ordering(agents_to_arrange, predictions_to_arrange);
}
