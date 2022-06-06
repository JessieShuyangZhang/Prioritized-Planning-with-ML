#include "MLFeatures.h"
#include "common.h"

void MLFeatures::generateFeatureVec(MLFeaturesInstance& instance_features, MLFeaturesAgent& agent_features, vector<double>& vec)
{
	assert(vec.size() == 0);

	vec.reserve(29); //29 features in total = 26 agent features + 3 map features
	vector<double> instance_features_vec = instance_features.getAllFeatures();
	vector<double> agent_feature_vec = agent_features.getAllFeatures();
	vec.insert(vec.end(), instance_features_vec.begin(), instance_features_vec.end());
	vec.insert(vec.end(), agent_feature_vec.begin(), agent_feature_vec.end());
}

void MLFeatures::printSolutionHeader(ofstream& ofs, vector<int>& best_priority_ordering, int best_cost, double best_runtime) {
	ofs << "#number of agents: " << best_priority_ordering.size() << endl;
	ofs << "#priority ordering: ";
	for (int i : best_priority_ordering)
		ofs << i << " ";
	ofs << endl;
	ofs << "#Sum of costs = " << best_cost << endl;
	ofs << "#runtime = " << best_runtime << " seconds." << endl;
}

void MLFeatures::printFormattedFeatures(int rank, int qid, ofstream& ofs, vector<double>& feature_vec)
{
	/*
	feature sequence:
	0-2: num_agents,obstacle_density,agent_sparcity
	3-5: mdd_width (mean max min)
	6-8: start_to_other_starts (mean max min)
	9-11: goal_to_other_goals (mean max min)
	12-15: start_goal_distance -- MDD depth, Manhattan distance, their ratio(float), difference
	16: mdd_ratio
	17: mdd_intersection
	18: num_singletons
	19-20: num_vertex_conflicts -- by pair of agent, by raw num of conflits
	21: num_other_goals_on_mdd
	22: num_other_starts_on_mdd
	23: num_other_mdds_on_goal
	24: num_other_mdds_on_start
	25-26: num_edge_conflicts -- by pair of agent, by raw num of conflits
	27-28: num_cardinal_conflicts -- by pair of agent, by raw num of conflits
	*/

	ofs << rank << " qid:" << qid;
	for (int i = 0; i < feature_vec.size(); i++) {
		ofs << " " << i+1 << ":" << feature_vec[i];
	}
	ofs << endl;
}