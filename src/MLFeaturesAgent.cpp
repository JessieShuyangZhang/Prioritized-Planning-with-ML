#include "MLFeaturesAgent.h"
#include <algorithm>
#include <numeric>

MLFeaturesAgent::MLFeaturesAgent(Instance& instance, int agent_id, PP& pp, MLFeaturesInstance& ml_features_instance)
	:instance(instance), agent_id(agent_id), pp(pp), agent(pp.agents[agent_id]),
	num_vertex_conflicts(2, 0), num_edge_conflicts(2, 0), num_cardinal_conflicts(2, 0),
	all_features_computed(false), ml_features_instance(ml_features_instance),
	has_cardinal_conflict(instance.num_of_agents, false)
{}

const vector<double>& MLFeaturesAgent::getAllFeatures()
{
	if (!all_features_computed)
		computeAllFeatures();
	return feature_vec;
}

void MLFeaturesAgent::findMeanMaxMin(vector<int>& vec, vector<double>& results)
{
	double sum = 0;
	int max = -MAX_COST;
	int min = MAX_COST;
	for (int i = 0; i < vec.size(); i++) {
		int curr_val = vec[i];
		sum += curr_val;
		if (max < curr_val)
			max = curr_val;
		if (min > curr_val)
			min = curr_val;
	}
	results = { sum / vec.size(), (double)max, (double)min };
}

void MLFeaturesAgent::computeMDDWidth()
{
	vector<int> widths;
	std::transform(
		agent.mdd.levels.begin(),
		agent.mdd.levels.end(),
		std::back_inserter(widths), [](const list<MDDNode*>& level) {
			return level.size();
		}
	);
	findMeanMaxMin(widths, mdd_width);
}

vector<double> MLFeaturesAgent::meanMaxMinToMultipleNodes(int loc, const vector<int>& other_locs)
{
	vector<int> distances_to_other;
	const vector<int>* distances_to_all = instance.getDistances(loc);
	for (int i = 0; i < other_locs.size(); i++) {
		if (loc == other_locs[i])
			continue;
		distances_to_other.emplace_back(distances_to_all->at(other_locs[i]));
	}

	vector<double> ret;
	findMeanMaxMin(distances_to_other, ret);
	return ret;
}

void MLFeaturesAgent::computeGoalToOtherGoals()
{
	goal_to_other_goals = meanMaxMinToMultipleNodes(agent.goal_location, instance.goal_locations);
}

void MLFeaturesAgent::computeStartToOtherStarts()
{
	start_to_other_starts = meanMaxMinToMultipleNodes(agent.start_location, instance.start_locations);
}

void MLFeaturesAgent::computeStartGoalDistance()
{
	int mdd_depth = agent.mdd.levels.size();
	int start_goal_manhattan = instance.getManhattanDistance(agent.start_location, agent.goal_location);
	double ratio;
	if (start_goal_manhattan == 0) {
		//eg."13:1 14:0 15:inf" which means "mdd_depth=1, start_goal_manhattan=0, ratio=infinity"
		ratio = 1;
	}
	else {
		ratio = (double)mdd_depth / start_goal_manhattan;
	}
	double difference = abs(mdd_depth - start_goal_manhattan);
	start_goal_distance = { (double)mdd_depth, (double)start_goal_manhattan, ratio, difference };
}

void MLFeaturesAgent::computeMDDRatio()
{
	double num_mdd_nodes = mdd_width[0] * agent.mdd.levels.size();
	mdd_ratio = num_mdd_nodes / instance.map_size;
}

void MLFeaturesAgent::computeSingletons()
{
	num_singletons = 0;
	for (int i = 1; i < agent.mdd.levels.size() - 1; i++) { //ignore start and goal
		if (agent.mdd.levels[i].size() == 1)
			++num_singletons;
	}
}

void MLFeaturesAgent::computeMDDIntersections()
{
	mdd_intersections = 0;

	//go through all nodes in my mdd
		//for each node, check if hashmap already contains this loc. 
	for (auto level : agent.mdd.levels) {
		for (auto node : level) {
			int occurance = ml_features_instance.mddnode_occurances[node->location];
			if (occurance > 1) { //loc is hashed by other agent(s)
				mdd_intersections += (occurance - 1);
			}
		}
	}
}

void MLFeaturesAgent::computeVertexConflicts()
{
	auto it_bypair = ml_features_instance.agent_vertexConf_bypair.find(agent_id);
	if (it_bypair != ml_features_instance.agent_vertexConf_bypair.end()) {
		num_vertex_conflicts[0] = ml_features_instance.agent_vertexConf_bypair[agent_id].size();
	}

	auto it_byconf = ml_features_instance.agent_vertexConf_byconf.find(agent_id);
	if (it_byconf != ml_features_instance.agent_vertexConf_byconf.end()) {
		num_vertex_conflicts[1] = ml_features_instance.agent_vertexConf_byconf[agent_id];
	}
}

void MLFeaturesAgent::computeEdgeConflicts()
{
	auto it_bypair = ml_features_instance.agent_edgeConf_bypair.find(agent_id);
	if (it_bypair != ml_features_instance.agent_edgeConf_bypair.end()) {
		num_edge_conflicts[0] = ml_features_instance.agent_edgeConf_bypair[agent_id].size();
	}

	auto it_byconf = ml_features_instance.agent_edgeConf_byconf.find(agent_id);
	if (it_byconf != ml_features_instance.agent_edgeConf_byconf.end()) {
		num_edge_conflicts[1] = ml_features_instance.agent_edgeConf_byconf[agent_id];
	}
}
void MLFeaturesAgent::computeCardinalConflicts() 
{
	auto it_bypair = ml_features_instance.agent_cardinalConf_bypair.find(agent_id);
	if (it_bypair != ml_features_instance.agent_cardinalConf_bypair.end()) {
		num_cardinal_conflicts[0] = ml_features_instance.agent_cardinalConf_bypair[agent_id].size();
	}

	auto it_byconf = ml_features_instance.agent_cardinalConf_byconf.find(agent_id);
	if (it_byconf != ml_features_instance.agent_cardinalConf_byconf.end()) {
		num_cardinal_conflicts[1] = ml_features_instance.agent_cardinalConf_byconf[agent_id];
	}

}

void MLFeaturesAgent::computeOtherStartsGoalsOnMDD()
{
	num_other_goals_on_mdd = 0;
	num_other_starts_on_mdd = 0;
	//loop thru my mdd
		//for each node see if its location is another agent's goal
	for (auto level : agent.mdd.levels) {
		for (auto node : level) {
			num_other_goals_on_mdd += ml_features_instance.goal_locations_hash[node->location];
			num_other_starts_on_mdd += ml_features_instance.start_locations_hash[node->location];
		}
	}
	--num_other_goals_on_mdd; //exclude self goal
	--num_other_starts_on_mdd; //exclude self start
}

void MLFeaturesAgent::computeOtherMDDsOnGoal()
{
	num_other_mdds_on_goal = 0;
	// find agent.goal_location in the mdd hash table
	num_other_mdds_on_goal += (ml_features_instance.mddnode_occurances[agent.goal_location] - 1);
}

void MLFeaturesAgent::computeOtherMDDsOnStart()
{
	num_other_mdds_on_start = 0;
	// find agent.goalstartation in the mdd hash table
	num_other_mdds_on_start += (ml_features_instance.mddnode_occurances[agent.start_location] - 1);
}

void MLFeaturesAgent::computeAllFeatures()
{
	computeMDDWidth();
	computeStartToOtherStarts();
	computeGoalToOtherGoals();
	computeStartGoalDistance();
	computeMDDRatio();
	computeMDDIntersections();
	computeSingletons();
	computeVertexConflicts(); //contains computation for cardinal conflict
	computeOtherStartsGoalsOnMDD();
	computeOtherMDDsOnGoal();
	computeOtherMDDsOnStart();
	computeEdgeConflicts(); //contains computation for cardinal conflict
	computeCardinalConflicts();

	feature_vec.reserve(14); //17 agent features. Add more later
	feature_vec.insert(feature_vec.end(), mdd_width.begin(), mdd_width.end());
	feature_vec.insert(feature_vec.end(), start_to_other_starts.begin(), start_to_other_starts.end());
	feature_vec.insert(feature_vec.end(), goal_to_other_goals.begin(), goal_to_other_goals.end());
	feature_vec.insert(feature_vec.end(), start_goal_distance.begin(), start_goal_distance.end());
	feature_vec.insert(feature_vec.end(), mdd_ratio);
	feature_vec.insert(feature_vec.end(), mdd_intersections);
	feature_vec.insert(feature_vec.end(), num_singletons);
	feature_vec.insert(feature_vec.end(), num_vertex_conflicts.begin(), num_vertex_conflicts.end());
	feature_vec.insert(feature_vec.end(), num_other_goals_on_mdd);
	feature_vec.insert(feature_vec.end(), num_other_starts_on_mdd);
	feature_vec.insert(feature_vec.end(), num_other_mdds_on_goal);
	feature_vec.insert(feature_vec.end(), num_other_mdds_on_start);
	feature_vec.insert(feature_vec.end(), num_edge_conflicts.begin(), num_edge_conflicts.end());
	feature_vec.insert(feature_vec.end(), num_cardinal_conflicts.begin(), num_cardinal_conflicts.end());
	all_features_computed = true;
}

//obsolete debug helper
void MLFeaturesAgent::printFeatures()
{
	computeAllFeatures();
	cout << "MDD Width: "; printVectorToConsole(mdd_width);
	cout << "Start to other starts (mean max min): "; printVectorToConsole(start_to_other_starts);
	cout << "Goal to other goals (mean max min): "; printVectorToConsole(goal_to_other_goals);
	cout << "Start goal distance (MDD,Manh,ratio,diff): "; printVectorToConsole(start_goal_distance);
	cout << "MDD ratio: " << mdd_ratio << endl;
}

void MLFeaturesAgent::printVectorToConsole(vector<double> vec)
{
	for (int i = 0; i < vec.size(); i++) {
		cout << vec[i] << " ";
	}
	cout << endl;
}
