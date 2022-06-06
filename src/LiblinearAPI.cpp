#include "LiblinearAPI.h"
#include "common.h"

LiblinearAPI::LiblinearAPI(string exePath, string train_out_fname, string test_out_fname, int num_agents, string weights_filename)
	:exePath(exePath), train_out_fname(train_out_fname), test_out_fname(test_out_fname), 
	num_agents(num_agents), weights_fname(weights_filename)
{
	if (ONWINDOWS) {
		std::replace(this->exePath.begin(), this->exePath.end(), '/', '\\');
		std::replace(this->weights_fname.begin(), this->weights_fname.end(), '/', '\\');
		std::replace(this->train_out_fname.begin(), this->train_out_fname.end(), '/', '\\');
		std::replace(this->test_out_fname.begin(), this->test_out_fname.end(), '/', '\\');
	}
	model_fname = train_out_fname + ".model";
}


void LiblinearAPI::trainLiblinear()
{
	//train -W weights.dat data_file
	// predict is useless	
	string train_cmd = " -c 128 -s 7 -W " + weights_fname + " " + train_out_fname + " " + model_fname;
	//string train_cmd = exePath + "train" + " -c 128 " + train_out_fname;
	if (ONLINUX) {
		train_cmd = "wine " + exePath + "train.exe" + train_cmd;
	}
	else if (ONWINDOWS) {
		train_cmd = exePath + "train" + train_cmd;
	}
	system(train_cmd.c_str());
}

void LiblinearAPI::readModelWeights()
{
	if (!model_weights.empty()) {
		return;
	}
	std::ifstream model_file(model_fname);
	string line; 
	for (int i = 0; i < 6; i++) { //ignore the first 6 lines
		std::getline(model_file, line);
	}
	while (std::getline(model_file, line)) {
		line.pop_back(); //remove white space at end of line
		model_weights.push_back(std::atof(line.c_str()));
	}

	//debug
	cout << "\nmodel weights:\n";
	for (int i = 0; i < model_weights.size(); i++) {
		cout << model_weights[i] << ',';
	}
	cout << endl;
}

void LiblinearAPI::setTestFeatures(vector<vector<double>>& feature_vec, vector<int>& test_orderings) {
	all_testfile_orderings.clear();
	all_testfile_features.clear();
	//only has one scen
	all_testfile_orderings.push_back(test_orderings);
	all_testfile_features.push_back(feature_vec);

	cout << "set test features complete" << endl;
}

void LiblinearAPI::readTestFileFeatures(int num_agents_test) 
{
	//assume the test file has the SVM-Rank format
	std::ifstream test_file(test_out_fname);	

	//the first num_agents_test lines will be for the first qid, the second num_agents_test lines for the second qid
	string line;
	while (std::getline(test_file, line)) {
		if (line[0] == '#') {
			if (line[1] == 'p') { //original priority ordering
				vector<string> split_line = Split(line, ' '); //ignore first two words
				vector<int> ordering;
				ordering.reserve(num_agents_test);
				for (int i = 0; i < num_agents_test; i++) {
					ordering.emplace_back(boost::lexical_cast<int>(split_line[i + 2]));
				}
				all_testfile_orderings.push_back(ordering);
			}
			continue;
		}
		vector<vector<double> > all_features; //row is one agent. column is one type of feature
		for (int line_cnt = 0; line_cnt < num_agents_test; line_cnt++) {
			if (line_cnt != 0) { //assuming there is no # lines in between a single qid block
				std::getline(test_file, line);
			}
			vector<string> split_line = Split(line, ' '); //first two will be rank and qid so ignore
			vector<double> features_per_agent;
			
			for (int i = 2; i < split_line.size(); i++) {
				vector<string> single_string_split = Split(split_line[i], ':');
				double feature = std::atof((single_string_split[1]).c_str());
				features_per_agent.push_back(feature);
			}
			all_features.push_back(features_per_agent);
		}
		all_testfile_features.push_back(all_features);
	}

	//debug
	cout << "\nall_testfile_features:(";
	cout << all_testfile_features.size() << " x " << all_testfile_features[0].size() << " x " << all_testfile_features[0][0].size();
	cout << ")" << endl;

	assert(all_testfile_features.size() == all_testfile_orderings.size());
}

void LiblinearAPI::weightsDotProdFeatures(int num_agents_test) //'scores' of each agent
{
	if (model_weights.empty()) {
		readModelWeights();
	}
	if (all_testfile_features.empty()) {
		//all_testfile_orderings.clear();
		readTestFileFeatures(num_agents_test);
	}

	for (int scen = 0; scen < all_testfile_features.size(); scen++) {
		vector<double> dotproducts;
		dotproducts.reserve(num_agents_test);
		for (int agent = 0; agent < all_testfile_features[scen].size(); agent++) {
			double dotprod = 0;
			for (int feature = 0; feature < all_testfile_features[scen][agent].size(); feature++) {
				dotprod += all_testfile_features[scen][agent][feature] * model_weights[feature];
			}
			dotproducts.emplace_back(dotprod);
		}
		all_testfile_dotprod.push_back(dotproducts);
	}
}


void LiblinearAPI::randomRestart(int scen, bool softmax) {
	MLRandomRestart(all_testfile_dotprod, all_testfile_orderings, orderings, scen, softmax, 
    0.4, 100.0, 0.5); //last one is beta 
}


void LiblinearAPI::rankPredictions(int num_agents_test, bool randomness)
{
	all_testfile_dotprod.clear();
	weightsDotProdFeatures(num_agents_test);
	orderings.clear();
	orderings.resize(all_testfile_dotprod.size());

	if (randomness) {
		for (int i = 0; i < all_testfile_dotprod.size(); i++) {
			this->randomRestart(i, true); //default to use softmax			
		}
	}
	else {
		for (int i = 0; i < all_testfile_orderings.size(); i++) {
			//sort all_testfile_dotprod
			vector<int> ranking = sort_indexes<double>(all_testfile_dotprod[i]);
			std::reverse(ranking.begin(), ranking.end()); //sort in descending order
			//orderings.push_back(ranking);
			//based on all_testfile_orderings, generate the predicted PP ordering
			vector<int> true_ordering;
			for (int j = 0; j < ranking.size(); j++) {
				true_ordering.push_back(all_testfile_orderings[i][ranking[j]]);
			}
			orderings[i] = true_ordering;
		}
	}
}