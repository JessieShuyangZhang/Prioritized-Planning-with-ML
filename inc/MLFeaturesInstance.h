#pragma once
#include "Instance.h"
#include "PP.h"

class MLFeaturesInstance 
{
public: 
	MLFeaturesInstance(Instance& instance, PP& pp);
	inline const vector<double>& getAllFeatures() { return feature_vec; }
	inline int getNumAgents() { return num_agents; }
	inline double getObstacleDensity() { return obstacle_density; }
	inline double getAgentSparcity() { return agent_sparcity; }
	void hashMDDnodeOccurances();
	void computeVertexConflicts();
	void computeEdgeConflicts();
	unordered_map<int, int> mddnode_occurances; //<node_loc, occurance>
	unordered_map<int, int> goal_locations_hash;
	unordered_map<int, int> start_locations_hash;
	unordered_map< pair <int, int>, vector<int> > vertexTimestep_agentids;
	unordered_map< tuple <int, int, int>, vector<int> > edgeTimestep_agentids;
	unordered_map<int, set<int>> agent_vertexConf_bypair;
	unordered_map<int, set<int>> agent_edgeConf_bypair;//result of this one is different from my previous
	unordered_map<int, set<int>> agent_cardinalConf_bypair;
	unordered_map<int, int> agent_vertexConf_byconf;
	unordered_map<int, int> agent_edgeConf_byconf;
	unordered_map<int, int> agent_cardinalConf_byconf;

private: 
	Instance& instance;
	PP& pp;
	int num_agents;
	double obstacle_density;
	double agent_sparcity;
	vector<double> feature_vec;
};