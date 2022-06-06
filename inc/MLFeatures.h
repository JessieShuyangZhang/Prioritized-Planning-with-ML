#pragma once
#include "MLFeaturesInstance.h"
#include "MLFeaturesAgent.h"

class MLFeatures
{
public:
	MLFeatures() {};
	void printFormattedFeatures(int rank, int qid, ofstream& ofs, vector<double>& feature_vec);
	void generateFeatureVec(MLFeaturesInstance& map_features, MLFeaturesAgent& agent_features, vector<double>& vec);
	void printSolutionHeader(ofstream& ofs, vector<int>& best_priority_ordering, int best_cost, double best_runtime);

};