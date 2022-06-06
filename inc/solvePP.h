#pragma once
#include "common.h"
#include "Instance.h"
#include "PP.h"
#include "LiblinearAPI.h"
#include "SVMRankAPI.h"

void solvePPLiblinear(std::ofstream& ofs, std::ofstream& weights_ofs, Instance& instance, int pp_screen, int pp_runs,
	int& scen, vector<vector<int> >& SVM_Trained_orderings, vector<vector<double> >& all_costs, LiblinearAPI& liblinear_api,
	int qid = 0, bool is_validation = false, int curr_learning_numagent = -1);

void solvePPSVMRank(std::ofstream& ofs, Instance& instance, int pp_screen,	
	vector<int>& pp_runs, int& scen, bool is_test, vector<double>& all_ml_overhead,
	vector<vector<int> >& SVM_Trained_orderings, vector<vector<double> >& all_costs, 
	vector< vector < vector <double> > >& rrfirst_byproduct, int qid = 0,
	int curr_learning_numagent = -1, 
	bool random_restart = false, bool fixedtime_restart = false,
	boost::optional<LiblinearAPI&> liblinear_api = boost::optional<LiblinearAPI&>(),
	boost::optional<SVMRankAPI&> svm_rank_api = boost::optional<SVMRankAPI&>(),
	boost::optional<vector<vector<double>>&> all_runtime = boost::optional<vector<vector<double>>&>(),
	boost::optional<vector<vector<int>>&> all_failed_agent_idx = boost::optional<vector<vector<int>>&>(),
	boost::optional<vector<vector<int>>&> all_restart_count = boost::optional<vector<vector<int>>&>(),
	double restart_time_constraint = 5.0);

void solvePP(Instance& instance, int pp_screen,	vector<int>& pp_runs, int scen, 
	bool is_test, vector<double>& all_ml_overhead, vector<vector<int> >& SVM_Trained_orderings,
	vector<vector<double> >& all_costs, vector< vector < vector <double> > >& rrfirst_byproduct,
	int qid = 0, bool random_restart = false, bool fixedtime_restart = false,
	boost::optional<LiblinearAPI&> liblinear_api = boost::optional<LiblinearAPI&>(),
	boost::optional<SVMRankAPI&> svm_rank_api = boost::optional<SVMRankAPI&>(),
	boost::optional<vector<vector<double>>&> all_runtime = boost::optional<vector<vector<double>>&>(),
	boost::optional<vector<vector<int>>&> all_failed_agent_idx = boost::optional<vector<vector<int>>&>(),
	boost::optional<vector<vector<int>>&> all_restart_count = boost::optional<vector<vector<int>>&>(),
	double restart_time_constraint = 5.0); //invoked by driver_test for SVMRank

void solvePP(std::ofstream& ofs, Instance& instance, int pp_screen,
	int pp_runs, int& scen, bool is_test, int qid = 0, int curr_learning_numagent = -1,
	boost::optional<SVMRankAPI&> svm_rank_api = boost::optional<SVMRankAPI&>()); //invoked by driver_train for SVMRank

void solvePP(std::ofstream& ofs, std::ofstream& weights_ofs, Instance& instance, int pp_screen,
	int pp_runs, int& scen, LiblinearAPI& liblinear_api, int qid = 0,  bool is_validation = false,
	int curr_learning_numagent = -1); //invoked by driver_train for Liblinear

void genSVMRankRawFeatureFile(Instance& instance, PP& pp, ofstream& ofs,
	vector<int>& best_priority_ordering, int best_cost, double best_runtime, int qid);

void genTestFileNoLabel(std::ofstream& ofs, Instance& instance, int pp_screen, int qid, double& runtime);

void comparisonAnalysis(std::ofstream& ofs, vector<vector<double> >& all_costs,
	vector<vector<int> >& all_failed_agent_idx,
	boost::optional<vector<vector<double>>&> all_runtime = boost::optional<vector<vector<double>>&>(),
	boost::optional<vector<vector<int>>&> all_restart_count = boost::optional<vector<vector<int>>&>());

void analyzeSucRateAndCost(vector<vector<double> >& all_costs, vector<double>& success_rate, vector<double>& avg_costs);

void analyzeCostRank(std::ofstream& ofs, vector<vector<double> >& all_costs, vector<double>& all_average_rank);

int computeSumStartGoalDist(Instance& instance);

template <typename U>
void averageEachCol(vector<double>& averages, const vector<vector<U> >& original);

template <typename V>
void averageEachRow(vector<double>& averages, const vector<vector<V> >& original);

void rankCost(vector<double>& partial_costs, vector<int>& cost_rank);

vector<vector<double>> getOriginalFeatureVectors(Instance& instance, PP& pp, vector<int>& best_ordering);

vector<vector<double>> normalizeFeatureVectors(vector<vector<double>>& raw_features);

void computePPOrdering(int i, PP& pp, int scen, bool is_test, vector<vector<int> >& SVM_Trained_orderings, bool random_restart=false);

void computePPOrderingCurrLearning(int i, Instance& instance, PP& pp, int scen,
	bool random_restart, int num_agent_test, 
	boost::optional<LiblinearAPI&> liblinear_api = boost::optional<LiblinearAPI&>(),
	boost::optional<SVMRankAPI&> svm_rank_api = boost::optional<SVMRankAPI&>());

void store_failed_agent_idx(int i, int idx, PP& pp,
	boost::optional<vector<vector<int>>&> all_failed_agent_idx = boost::optional<vector<vector<int>>&>());

//-------------------------for debug------------------------------------
template <typename T>
void print1DVector(std::ostream& ofs, const vector<T>& vec, string message = "", bool pythonStyle=false);

template <typename T>
void print2DVector(std::ofstream& ofs, const vector<vector<T> >& vec, string message = "");

void printDependencyPairs(vector<set<int>>& dependency_graph);

void floydWarshallAlgorithm(vector<vector<int>>& dependency_matrix, int num_agents);

void DFS(vector<vector<int>> const& graph, int v, vector<bool>
	& discovered, vector<int>& departure, int& time);

vector<int> doTopologicalSort(vector<vector<int>> const& graph, int N);

void topologicalSortUtil(vector<vector<int>> const& graph, int v, vector<bool>& visited,
	std::stack<int>& Stack);

vector<int> topologicalSort(vector<vector<int>> const& graph, int V);

vector<int> BFS(int s, int V, vector<vector<int>> const& graph);