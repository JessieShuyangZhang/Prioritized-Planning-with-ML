#pragma once
#include "common.h"

class LiblinearAPI
{
public:
	LiblinearAPI(string exePath, string train_out_fname, string test_out_fname,
		int num_agents, string weights_filename);

	void trainLiblinear();
	void readModelWeights();
	void readTestFileFeatures(int num_agents_test);
	void weightsDotProdFeatures(int num_agents_test);
	void rankPredictions(int num_agents_test, bool randomness = false);
	void randomRestart(int scen, bool softmax = false);

	void setTestFeatures(vector<vector<double>>& feature_vec, vector<int>& test_orderings);
	vector<vector<int>> orderings;

private:
	string exePath;
	string outPath; //this is the folder path
	string train_out_fname; //including folder path
	string test_out_fname; //including folder path
	string model_fname; //including folder path
	string weights_fname; //including folder path
	int num_agents;
	vector<double> model_weights;
	vector<vector<vector<double>>> all_testfile_features; // (25 scen) x num_of_agents x (29 features-per-agent)
	vector<vector<int>> all_testfile_orderings; //(25 scen) x num_of_agents
	vector<vector<double>> all_testfile_dotprod; //(25 scen) x num_of_agents. This is the raw prediction value that needs to be ranked
};