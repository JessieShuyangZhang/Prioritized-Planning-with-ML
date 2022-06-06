#pragma once
#include "common.h"
#include "PP.h"
#include "MDD.h"
#include "Instance.h"
#include "MLFeaturesInstance.h"

class MLFeaturesAgent //one instance of MLFeatures per agent
{
public:
	MLFeaturesAgent(Instance& instance, int agent_id, PP& pp, MLFeaturesInstance& ml_features_instance);
	const vector<double>& getAllFeatures();
	void computeAllFeatures(); //Not done yet. Add in others later
	void computeMDDWidth();
	void computeStartToOtherStarts();
	void computeGoalToOtherGoals();
	void computeStartGoalDistance();
	void computeMDDRatio();
	void computeSingletons();
	void computeMDDIntersections();
	void computeVertexConflicts();
	void computeOtherStartsGoalsOnMDD();
	void computeOtherMDDsOnGoal();
	void computeOtherMDDsOnStart();
	void computeEdgeConflicts();
	void computeCardinalConflicts();
	void printFeatures(); //for debugging

private:
	Instance& instance;
	PP& pp;
	MLFeaturesInstance& ml_features_instance;
	Agent& agent;
	int agent_id;
	vector<int> priority_ordering;

	//helper function/variables
	bool all_features_computed;
	vector<double> feature_vec; //holds all features after computeAllFeatures();
	void findMeanMaxMin(vector<int>& vec, vector<double>& results);
	vector<double> meanMaxMinToMultipleNodes(int loc, const vector<int>& other_locs);
	vector<bool> has_cardinal_conflict; //pairwise cardinal conflict flag

	//just for debugging
	void printVectorToConsole(vector<double> vec);

	//29 features in total = 26 agent features + 3 map features
	vector<double> mdd_width; //mean max min (3)
	vector<double> start_to_other_starts; //mean max min (3)
	vector<double> goal_to_other_goals; //mean max min (3)
	vector<double> start_goal_distance; //MDD depth, Manhattan distance, their ratio(float), difference (4)
	double mdd_ratio; // num of cells in mdd devided by map size
	int mdd_intersections; //intersecting nodes in my mdd with others' mdd
	int num_singletons;
	vector<int> num_vertex_conflicts; //by pair of agent, by raw num of conflits (2)
	vector<int> num_edge_conflicts;
	vector<int> num_cardinal_conflicts;
	int num_other_goals_on_mdd; //my MDD is on how many other goal nodes
	int num_other_starts_on_mdd;
	int num_other_mdds_on_goal; //my goal node is on how many other agents' MDD
	int num_other_mdds_on_start;
};