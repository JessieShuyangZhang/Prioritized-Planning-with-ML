#pragma once
#include "common.h"

class LiblinearFeatures
{
public:
	void printFeatureVecDiff(vector<double>& vecLo, vector<double>& vecHi, int label, ofstream& ofs); 	
	//vecLo: feature vec w/ lower priority; label can be 1 or -1

	void inferAllDependencyPairs(vector<set<int>>& dependency_graph);
	//infer all dependency pairs from a dependency graph and store them in the returned dependency matrix

	void printDependencyGraph(vector<vector<double>>& all_feature_vecs, vector<set<int>>& dependency_graph, ofstream& ofs);

	int getDependencyPairCount(vector<set<int>>& dependency_graph);
	void printWeights(int dependency_pair_count, ofstream& weight_file);
	void printFeaturesByDepPairOccurrence(vector<vector<double>>& all_raw_feature_vecs, vector<set<pair<int, int>>> &deppair_by_occurance,
		ofstream& ofs, bool infer_dependencies = false, bool is_validation = false, ofstream* val_ofs = nullptr);
	int getDepPairCntByOccurrence(vector<set<pair<int, int>>>& deppair_by_occurance);
	void printWeightsByDepPairOccurrence(vector<set<pair<int, int>>>& deppair_by_occurance, ofstream& weight_file);
};