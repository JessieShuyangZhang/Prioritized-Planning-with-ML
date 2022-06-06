#include "SVMRankAPI.h"
#include "common.h"

#include <numeric>      // std::iota
#include <algorithm>    // std::sort, std::stable_sort

template <typename T>
vector<int> sort_indexes(const vector<T>& v) { //unable to delete from this file, otherwise build error
	vector<int> idx(v.size());
	std::iota(idx.begin(), idx.end(), 0);
	std::stable_sort(idx.begin(), idx.end(),
		[&v](int i1, int i2) {return v[i1] < v[i2]; });
	return idx;
}

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&PCLOSE)> pipe(POPEN(cmd, "r"), PCLOSE);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}
//source: https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
//Replace popen and pclose with _popen and _pclose for Windows.

SVMRankAPI::SVMRankAPI(string exePath, string outPath, string train_out_fname, string test_out_fname, int num_agents, string model_filename, string predictions_filename)
	:exePath(exePath), outPath(outPath), train_out_fname(train_out_fname), test_out_fname(test_out_fname), num_agents(num_agents)
{
	if (ONWINDOWS) {
		std::replace(this->exePath.begin(), this->exePath.end(), '/', '\\');
		std::replace(this->outPath.begin(), this->outPath.end(), '/', '\\');
		std::replace(this->train_out_fname.begin(), this->train_out_fname.end(), '/', '\\');
		std::replace(this->test_out_fname.begin(), this->test_out_fname.end(), '/', '\\');
		std::replace(model_filename.begin(), model_filename.end(), '/', '\\');
		std::replace(predictions_filename.begin(), predictions_filename.end(), '/', '\\');
	}
	model_fname = outPath + model_filename;
	predictions_fname = outPath + predictions_filename;
}

void SVMRankAPI::trainAndPredictTest(bool validation, int k)
{
    if (!validation || k <= 0) {
		remove(predictions_fname.c_str());
        string train_cmd = (ONLINUX ? "./" : "") + exePath + svm_learn + " -c 20.0 -l 2 " + train_out_fname + " " + model_fname;
        system(train_cmd.c_str());
        string test_cmd = (ONLINUX ? "./" : "") + exePath + svm_classify + " " + test_out_fname + " " + model_fname + " " + predictions_fname;
        system(test_cmd.c_str());
    }
    //IGNORE THE WORK BELOW. Cross-validation abandoned for now
	else { //find the best model with least loss

		//hardcoded parameter choices
		vector<int> group_param_vals = { 1, 5, 10 };
		vector<double> svm_c_values = { 1, 10, 20, 100 };
		double min_loss = INT_MAX / 2;
		int min_loss_group_param = group_param_vals[0];
		double min_loss_svm_c = svm_c_values[0];
		string validation_fname;

		for (int i = 0; i < group_param_vals.size(); i++) {
			int agents_per_group = group_param_vals[i];

			// contruct new train_out_fname -- hardcoded for now, change later		
			string new_train_fname = outPath + "train_3to25_norm";
			if (agents_per_group == 1) {
				new_train_fname += ".dat";
			}
			else {
				new_train_fname += "_group" + boost::lexical_cast<string>(agents_per_group) + ".dat";
			}
			cout << "new_train_fname: " << new_train_fname << endl; //debug

			// split training file (k-1):1 
			vector<string> folds_files;
			bool split_again = false;
			if (split_again) {
				kFoldSplit(new_train_fname, agents_per_group, 5, folds_files);
			}
			else {
				folds_files.push_back(new_train_fname.substr(0, new_train_fname.size() - 4) + "_cv" + boost::lexical_cast<string>(k) + "_train.dat");
				folds_files.push_back(new_train_fname.substr(0, new_train_fname.size() - 4) + "_cv" + boost::lexical_cast<string>(k) + "_validate.dat");
			}
			if (agents_per_group == 1) {
				validation_fname = folds_files.back();
			}

			for (int j = 0; j < svm_c_values.size(); j++) {
				double svm_c = svm_c_values[j];

				//train on k-1 folds, and validate on the last fold (do it just for once)
				string new_train_cmd = exePath + svm_learn + " -c " + boost::lexical_cast<string>(svm_c) + " -l 2 " + folds_files[0] + " " + model_fname;
				system(new_train_cmd.c_str());
				string new_test_cmd = exePath + svm_classify + " " + validation_fname + " " + model_fname + " " + predictions_fname;
				string output = exec(new_test_cmd.c_str());
				string loss_on_test_set = Split(Split(output, '\n')[4], ' ').back();
				double loss = boost::lexical_cast<double>(loss_on_test_set);
				cout << "fold loss: " << loss << "    svm_c=" << svm_c << "    agent per group=" << agents_per_group << endl;

				//remember the loss value from command line -- record each value
				if (loss < min_loss) {
					min_loss = loss;
					min_loss_svm_c = svm_c;
					min_loss_group_param = agents_per_group;
				}
			}
		}

		//TODO: find out what svm_c and agent_per_group corresponded to the least loss value
		//construct new command to train & predict on those parameters with all training data combined
		cout << "best svm c param = " << min_loss_svm_c << endl;
		cout << "best group param = " << min_loss_group_param << endl;

		train_out_fname = outPath + "train_3to25_norm" + (min_loss_group_param == 1 ? "" : ("_group" + boost::lexical_cast<string>(min_loss_group_param))) + ".dat";
		string train_cmd = exePath + svm_learn + " -c " + boost::lexical_cast<string>(min_loss_svm_c) + " -l 2 " + train_out_fname + " " + model_fname;
		cout << train_cmd << endl;
		system(train_cmd.c_str());
		string test_cmd = exePath + svm_classify + " " + test_out_fname + " " + model_fname + " " + predictions_fname;
		cout << test_cmd << endl;
		system(test_cmd.c_str());
	}
}

void SVMRankAPI::readTestFileOrderings(int test_num_agent, string test_out_fname)
{
	std::ifstream testfile(test_out_fname);
	string testFileLine;
	while (std::getline(testfile, testFileLine)) {
		if (testFileLine[0] == '#' && testFileLine[1] == 'p') { //the comment about priority ordering
			vector<string> all_words = Split(testFileLine, ' ');
			vector<int> ordering;
			ordering.reserve(test_num_agent);
			for (int i = 0; i < test_num_agent; i++) {
				ordering.emplace_back(boost::lexical_cast<int>(all_words[i + 2])); //first two words are '#priority' and 'ordering:'
			}
			all_testfile_orderings.push_back(ordering);
		}
	}
}

void SVMRankAPI::setTestFeatures(vector<vector<double>>& feature_vec, vector<int>& test_orderings)
{
	all_testfile_orderings.clear();
	all_testfile_features.clear();

	//only has one scen
	all_testfile_orderings.push_back(test_orderings);
	all_testfile_features.push_back(feature_vec);

	cout << "set test features complete" << endl;
}

void SVMRankAPI::weightsDotProdFeatures(int num_agents_test)
{
	all_testfile_dotprod.clear();
	if (model_weights.empty()) {
		readModelWeights();
	}
	assert(!all_testfile_features.empty());

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

void SVMRankAPI::readModelWeights()
{
	if (!model_weights.empty()) {
		return;
	}
	model_weights.resize(29);
	std::ifstream model_file(model_fname);
	if (model_file.fail()) {
		cout << "model file does not exist, training to generate a model file" << endl;
		remove(predictions_fname.c_str());
		string train_cmd = (ONLINUX ? "./" : "") + exePath + svm_learn + " -c 20.0 -l 2 " + train_out_fname + " " + model_fname;
		system(train_cmd.c_str());
		model_file.open(model_fname);
	}
	
	string line;
	for (int i = 0; i < 11; i++) { //ignore the first 11 lines
		std::getline(model_file, line);
	}

	std::getline(model_file, line); //last line is the weights
	vector<string> split_line = Split(line, ' ');
	for (auto entry:split_line) {
		if (entry.size() <= 1) {
			continue;
		}
		vector<string> feature_info = Split(entry, ':');
		int feature_index = std::stoi(feature_info[0]) - 1;
		double feature_value = std::stof(feature_info[1]);

		model_weights[feature_index] = feature_value;
	}

	//debug
	cout << "\nmodel weights:\n";
	for (int i = 0; i < model_weights.size(); i++) {
		cout << model_weights[i] << ',';
	}
	cout << endl;
}

void SVMRankAPI::readRawPrediction(int test_num_agent, string predictions_fname)
{
	std::ifstream predfile(predictions_fname);
	if (predfile.fail()) {
		weightsDotProdFeatures(test_num_agent);
		return;
	}
	//the first num_agents lines will be for the first qid, the second num_agents lines for the second qid
	string line;
	while (std::getline(predfile, line)) {
		double raw_prediction = boost::lexical_cast<double>(line);
		vector<double> raw_preds;
		raw_preds.reserve(test_num_agent);
		raw_preds.emplace_back(raw_prediction);
		for (int i = 0; i < test_num_agent - 1; i++) {
			std::getline(predfile, line);
			raw_preds.emplace_back(boost::lexical_cast<double>(line));
		}
		all_testfile_dotprod.push_back(raw_preds);
	}
}

void SVMRankAPI::randomRestart(int scen, bool softmax) {
	MLRandomRestart(all_testfile_dotprod, all_testfile_orderings, orderings, scen, softmax,
    0.4, 100.0, -1); //last one is beta 
}


void SVMRankAPI::rankPredictions(int test_num_agent, bool randomness)
{
	all_testfile_dotprod.clear();
	readRawPrediction(test_num_agent, predictions_fname); // this calls weightsDotProdFeatures() if prediction file DNE
	orderings.clear();

	//------------- Read in the agents' total ordering in the test file -------------
	if (all_testfile_orderings.empty()) {
		readTestFileOrderings(test_num_agent, test_out_fname);
	}	

	//--------------------- get the predicted total ordering ---------------------
	assert(all_testfile_dotprod.size() == all_testfile_orderings.size());

	if (randomness) {
		orderings.resize(all_testfile_dotprod.size());
		for (int i = 0; i < all_testfile_dotprod.size(); i++) {
			this->randomRestart(i, true);
		}
	}
	else {
		orderings.reserve(all_testfile_dotprod.size()); //this should be the number of scens in the test file
		for (int i = 0; i < all_testfile_dotprod.size(); i++) {
			// sort_indexes for the raw_preds vector
			// use the return vector with the original ordering in the test file to get the predicted total ordering
			vector<int> ranking = sort_indexes<double>(all_testfile_dotprod[i]);
			vector<int> true_ordering;
			for (int j = 0; j < ranking.size(); j++) {
				true_ordering.push_back(all_testfile_orderings[i][ranking[j]]);
			}
			orderings.emplace_back(true_ordering);
		}
	}
}

void SVMRankAPI::rawToNormalized(string rawfile, string normalizedfile)
{
	int qid;
	vector<string> comments;
	vector<int> ranks; //the first int that appears in every line

	std::ifstream infile(outPath + rawfile);
	remove((outPath + normalizedfile).c_str());
	std::ofstream ofile(outPath + normalizedfile, std::ios_base::app); //concatenate
	//the first num_agents lines will be for the first qid, the second num_agents lines for the second qid
	string line;
	while (std::getline(infile, line)) {
		if (line[0] == '#') {
			comments.push_back(line);
			continue;
		}
		vector<vector<double> > all_features; //row is one agent. column is one type of feature
		for (int line_cnt = 0; line_cnt < num_agents; line_cnt++) {
			if (line_cnt != 0) { //assuming there is no # lines in between a single qid block
				std::getline(infile, line);
			}
			vector<string> split_line = Split(line, ' ');
			int rank = boost::lexical_cast<int>(split_line[0]);
			ranks.push_back(rank);
			vector<double> features_per_agent;
			for (int i = 1; i < split_line.size(); i++) {
				vector<string> single_string_split = Split(split_line[i], ':');
				if (i == 1) {
					qid = boost::lexical_cast<int>(single_string_split[1]);
					continue;
				}
				double feature = boost::lexical_cast<double>(single_string_split[1]);
				features_per_agent.push_back(feature);
			}
			all_features.push_back(features_per_agent);
		}

		//normalize all_features
		vector<vector<double> > all_features_normalized = normalizeFeatureVectors(all_features);

		//output to normalizedfile
		for (int i = 0; i < comments.size(); i++) {
			ofile << comments[i] << endl;
		}
		for (int i = 0; i < all_features_normalized.size(); i++) {
			ofile << ranks[i] << " qid:" << qid;
			for (int j = 0; j < all_features_normalized[i].size(); j++) {
				ofile << " " << j + 1 << ":" << all_features_normalized[i][j];
			}
			ofile << endl;
		}
		normalization_cleanup(comments, ranks);
	}
}

void SVMRankAPI::groupRanking(string beforefile, string afterfile, int agents_per_group)
{
	vector<string> comments;
	std::ifstream infile(outPath + beforefile);
	remove((outPath + afterfile).c_str());
	std::ofstream ofile(outPath + afterfile, std::ios_base::app); //concatenate
	string line;
	while (std::getline(infile, line)) {
		if (line[0] == '#') {
			comments.push_back(line);
			continue;
		}
		vector<int> ranks;
		vector<string> stuff_after_rank;
		for (int line_cnt = 0; line_cnt < num_agents; line_cnt++) {
			if (line_cnt != 0) { //assuming there is no # lines in between a single qid block
				std::getline(infile, line);
			}
			vector<string> split_line = Split(line, ' ', true);
			int rank = boost::lexical_cast<int>(split_line[0]);
			ranks.push_back(rank);
			stuff_after_rank.push_back(split_line[1]);
		}
		//assume rank range [0, num_agents-1] and is unique for each agent
		vector<int> new_ranks;
		for (int i = 0; i < ranks.size(); i++) {
			new_ranks.push_back(ranks[i] / agents_per_group);
		}

		//output to normalizedfile
		for (int i = 0; i < comments.size(); i++) {
			ofile << comments[i] << endl;
		}
		for (int agent_cnt = 0; agent_cnt < num_agents; agent_cnt++) {
			ofile << new_ranks[agent_cnt] << " " << stuff_after_rank[agent_cnt] << endl;
		}
		
		comments.clear();
	}

}

void SVMRankAPI::fixQid(string beforefile, string afterfile)
{
	vector<string> comments;
	std::ifstream infile(outPath + beforefile);
	remove((outPath + afterfile).c_str());
	std::ofstream ofile(outPath + afterfile, std::ios_base::app); //concatenate
	string line;
	int qid = 1;
	while (std::getline(infile, line)) {
		if (line[0] == '#') {
			comments.push_back(line);
			continue;
		}
		vector<int> ranks;
		vector<string> stuff_after_qid;
		for (int line_cnt = 0; line_cnt < num_agents; line_cnt++) {
			if (line_cnt != 0) { //assuming there is no # lines in between a single qid block
				std::getline(infile, line);
			}
			vector<string> split_line = Split(line, ' ', true);
			int rank = boost::lexical_cast<int>(split_line[0]);
			vector<string> split_split_line = Split(split_line[1], ' ', true);
			ranks.push_back(rank);
			stuff_after_qid.push_back(split_split_line[1]);
		}
		

		//output to new file
		for (int i = 0; i < comments.size(); i++) {
			ofile << comments[i] << endl;
		}
		for (int agent_cnt = 0; agent_cnt < num_agents; agent_cnt++) {
			ofile << ranks[agent_cnt] << " qid:" <<qid<<" "<< stuff_after_qid[agent_cnt] << endl;
		}
		
		comments.clear();
		qid++;
	}

}

void SVMRankAPI::normalization_cleanup(vector<string>& comments, vector<int>& ranks)
{
	ranks.clear();
	comments.clear();
}

//split it into 2 files with ratio (k-1):1
void SVMRankAPI::kFoldSplit(string beforefile, int agents_per_group, int k, vector<string>& afterfiles)
{
	std::ifstream infile(beforefile);
	string trainfolds = beforefile.substr(0, beforefile.size() - 4) + "_cv" + boost::lexical_cast<string>(k) + "_train.dat";
	string testfold = beforefile.substr(0, beforefile.size() - 4) + "_cv" + boost::lexical_cast<string>(k) + "_validate.dat";
	remove(trainfolds.c_str());
	remove(testfold.c_str());
	afterfiles = { trainfolds, testfold };

	vector<string> comments;
	string line;
	int counter = 0;
	while (std::getline(infile, line)) {
		if (line[0] == '#') {
			comments.push_back(line);
			continue;
		}
		vector<string> stuff;
		for (int line_cnt = 0; line_cnt < num_agents; line_cnt++) {
			if (line_cnt != 0) { //assuming there is no # lines in between a single qid block
				std::getline(infile, line);
			}
			stuff.push_back(line);
		}
		counter++;

		int fold = counter % k;
		string outfile = (fold == k - 1) ? testfold : trainfolds; // = beforefile.substr(0, beforefile.size() - 4) + "_fold_" + boost::lexical_cast<string>(fold) + ".dat";
		std::ofstream ofile(outfile, std::ios_base::app); //concatenate
		for (int i = 0; i < comments.size(); i++) {
			ofile << comments[i] << endl;
		}
		for (int i = 0; i < stuff.size(); i++) {
			ofile << stuff[i] << endl;
		}
		comments.clear();
	}
}
