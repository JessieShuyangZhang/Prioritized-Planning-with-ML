#pragma once
#include "common.h"

class SVMRankAPI
{
public:
	SVMRankAPI(string exePath, string outPath, string train_out_fname, string test_out_fname,
		int num_agents, string model_filename, string predictions_filename);
	void trainAndPredictTest(bool validation=false, int k=-1);
	void readTestFileOrderings(int test_num_agent, string test_out_fname);
	void readModelWeights();
	void setTestFeatures(vector<vector<double>>& feature_vec, vector<int>& test_orderings);
	void weightsDotProdFeatures(int num_agents_test);
	void readRawPrediction(int test_num_agent, string predictions_fname);
	void randomRestart(int scen, bool softmax);
	void rankPredictions(int test_num_agent, bool randomness);
	void rawToNormalized(string rawfile, string normalizedfile);
	void groupRanking(string beforefile, string afterfile, int agents_per_group);
	void fixQid(string beforefile, string afterfile);
	void kFoldSplit(string beforefile, int agents_per_group, int k, vector<string>& afterfiles);
	
	vector<vector<int> > orderings;
private: 
	string exePath;
	string outPath;
	string train_out_fname; //including folder path
	string test_out_fname; //including folder path
	string model_fname;
	string predictions_fname;
	string svm_learn = "svm_rank_learn";
	string svm_classify = "svm_rank_classify";
	int num_agents;
	vector<double> model_weights;
	//helper function
	void normalization_cleanup(vector<string>& comments, vector<int>& ranks);
	vector<vector<vector<double>>> all_testfile_features; // (25 scen) x num_of_agents x (29 features-per-agent). Requires too much memory?

	vector<vector<int>> all_testfile_orderings; //(25 scen) x num_of_agents
	vector<vector<double>> all_testfile_dotprod; //(25 scen) x num_of_agents. This is the raw prediction value that needs to be ranked
};
