#include "liblinearFeatures.h"

void LiblinearFeatures::printFeatureVecDiff(vector<double>& vecLo, vector<double>& vecHi, int label, ofstream& ofs)
{
	vector<double> difference;
	for (int i = 0; i < vecLo.size(); i++) {
		double diff = (label == 1) ? (vecHi[i] - vecLo[i]) : (vecLo[i] - vecHi[i]);
		difference.push_back(diff); //define label +1 for feature vector Hi-Lo
	}
	ofs << ((label == 1) ? "+" : "") << label;
	for (int i = 0; i < difference.size(); i++) {
		ofs << " " << i + 1 << ":" << difference[i];
	}
	ofs << endl;
}

void LiblinearFeatures::inferAllDependencyPairs(vector<set<int>>& dependency_graph)
{
	int num_agents = (int)dependency_graph.size();
	vector<vector<int>> dependency_matrix(num_agents, vector<int>(num_agents, MAX_COST)); //use MAX_COST to represent INF
	for (int i = 0; i < num_agents; i++) {
		if (dependency_graph[i].empty())
			continue;

		for (auto j : dependency_graph[i]) { //j has higher priority than i
			dependency_matrix[j][i] = 1; //let weight be 1. It also represents j->i (j must be planned before i)
		}
	}
	//run Floyd-Warshall algorithm
	for (int k = 0; k < num_agents; k++) {
		for (int i = 0; i < num_agents; i++) {
			for (int j = 0; j < num_agents; j++) {
				if (dependency_matrix[i][j] > (dependency_matrix[i][k] + dependency_matrix[k][j])
					&& (dependency_matrix[i][k] != MAX_COST
						&& dependency_matrix[k][j] != MAX_COST)) {
					dependency_matrix[i][j] = dependency_matrix[i][k] + dependency_matrix[k][j];
				}
			}
		}
	}
	//turn returned matrix into a thorough dependency graph and use that for feature generation
	for (int i = 0; i < num_agents; i++) {
		for (int j = 0; j < num_agents; j++) {
			if (dependency_matrix[j][i] != MAX_COST) {
				dependency_graph[i].insert(j);
			}
		}
	}
}

void LiblinearFeatures::printDependencyGraph(vector<vector<double>>& all_feature_vecs, vector<set<int>>& dependency_graph, ofstream& ofs)
{
	for (int i = 0; i < (int)dependency_graph.size(); i++) {
		if (dependency_graph[i].empty())
			continue;
		for (auto j : dependency_graph[i]) { //j has higher priority than i
			int label = (rand() % 2 == 0) ? (1) : (-1);
			printFeatureVecDiff(all_feature_vecs[i], all_feature_vecs[j], label, ofs);
		}
	}
}

//TODO: refactor, use the shared function in common.cpp
int LiblinearFeatures::getDependencyPairCount(vector<set<int>>& dependency_graph) {
	int pair_count = 0;
	for (int i = 0; i < (int)dependency_graph.size(); i++) {
		pair_count += dependency_graph[i].size();
	}
	return pair_count;
}

void LiblinearFeatures::printWeights(int dependency_pair_count, ofstream& weight_file) {
	double weight = 1.0 / dependency_pair_count;
	for (int i = 0; i < dependency_pair_count; i++) {
		weight_file << weight << endl;
	}
}

void LiblinearFeatures::printFeaturesByDepPairOccurrence(vector<vector<double>>& all_raw_feature_vecs, 
	vector<set<pair<int, int>>>& deppair_by_occurance, ofstream& ofs, bool infer_dependencies, bool is_validation, ofstream* val_ofs)
{ 
	for (int i = 0; i < (int)deppair_by_occurance.size(); i++) {
		for (auto pair : deppair_by_occurance[i]) { //in pair<i,j>: i has higher priority than j
			int label = (rand() % 2 == 0) ? (1) : (-1);
			printFeatureVecDiff(all_raw_feature_vecs[pair.second], all_raw_feature_vecs[pair.first], label, ofs);
			if (is_validation) {
				printFeatureVecDiff(all_raw_feature_vecs[pair.second], all_raw_feature_vecs[pair.first], label, *val_ofs);
			}
		}
	}
}

int LiblinearFeatures::getDepPairCntByOccurrence(vector<set<pair<int, int>>>& deppair_by_occurance) {
	int total_cnt = 0;
	for (int i = 0; i < (int)deppair_by_occurance.size(); i++) {
		total_cnt += (i + 1) * deppair_by_occurance[i].size();
	}
	cout << "total dep pair count: " << total_cnt << endl; //for debug
	return total_cnt;
}


void LiblinearFeatures::printWeightsByDepPairOccurrence(vector<set<pair<int, int>>>& deppair_by_occurance, 
	ofstream& weight_file) {
	int total_cnt = getDepPairCntByOccurrence(deppair_by_occurance);
	double sum = 0;
	vector<double> weights;
	for (int i = 0; i < (int)deppair_by_occurance.size(); i++) {
		double weight = (i+1) / (double)total_cnt; //linear weighting
		for (auto pair : deppair_by_occurance[i]) {
			sum += weight;
			weights.push_back(weight);
		}
	}
	for (int i = 0; i < weights.size(); i++) {
		weights[i] /= sum;
		weight_file << weights[i] << endl;
	}
}