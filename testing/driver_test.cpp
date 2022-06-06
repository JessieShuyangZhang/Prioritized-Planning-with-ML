#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
//#include "ECBS.h"
#include "PP.h"
#include "common.h"
#include "MLFeaturesInstance.h"
#include "MLFeaturesAgent.h"
#include "MLFeatures.h"
#include "solvePP.h"
#include "SVMRankAPI.h"
#include "LiblinearAPI.h"

void print1DVectorCannotUseSolvepp(std::ostream& ofs, const vector<double>& vec, string message, bool pythonStyle)
{
    ofs << message;
    if (pythonStyle) ofs << "[";
    for (int i = 0; i < vec.size(); i++) {
        ofs << vec[i] << ((pythonStyle && (i != vec.size()-1)) ? ", " : " ");
    }
    if (pythonStyle) ofs << "],";
    ofs << endl;
}

void printValidationLossAnalysis(int scen, int top_picks, vector<vector<int>>& ML_trained_orderings,
	vector<vector<set<pair<int, int>>>>& deppair_by_occurance) {
	cout << "scen " << scen + 1 << endl;
	int wrong_pair_cnt = 0;
	int total_pair_cnt = 0;
	vector<int>& predicted_ordering = ML_trained_orderings[scen];
	for (int occur = 0; occur < top_picks; occur++) {
		cout << "occurrence " << occur + 1 << ":";
		for (auto pair : deppair_by_occurance[scen][occur]) { //in pair<i,j>: j has higher priority than i
			total_pair_cnt += (occur + 1);
			auto p_first = std::find(predicted_ordering.begin(), predicted_ordering.end(), pair.first);
			auto p_second = std::find(predicted_ordering.begin(), predicted_ordering.end(), pair.second);
			int i_index = p_first - predicted_ordering.begin();
			int j_index = p_second - predicted_ordering.begin();
			if (i_index < j_index) { //that's a conflict
				cout << "<" << pair.first << "," << pair.second << "> ";
				wrong_pair_cnt += (occur + 1);
			}
		}
		cout << endl;
	}
	cout << "total pairs in ground truth: " << total_pair_cnt << endl;
	cout << "pairs with wrong prediction: " << wrong_pair_cnt << endl;
	cout << "loss: " << (double)wrong_pair_cnt / total_pair_cnt << endl;
	cout << endl;
}

void pp100_dotplotdata()
{
	string map_name = "ost003d"; //room-32-32-4, maze-32-32-2, lak303d, ost003d, random-32-32-20, warehouse-10-20-10-2-1
	int num_agent_test = 400;
	string repo_level_path, out_path;
	if (ONWINDOWS) {
		repo_level_path = "../../../../";
		out_path = "../";
	}
	else if (ONLINUX) {
		repo_level_path = "../";
		out_path = "../out/build/x64-Release/";
	}
	std::ofstream ofs("ppvariance.txt", std::ios_base::app);
	ofs << map_name << "\n num_agents = " << num_agent_test << "\n[";
	string map_folder_path = repo_level_path + "map/";
	string map_file = map_folder_path + map_name + ".map";
	string scenario_file = map_folder_path + map_name + ".map-scen-random/" + map_name + "-random-1.scen";
	Instance instance(map_file, scenario_file, false, num_agent_test);
	int sum_start_goal_dist = computeSumStartGoalDist(instance);
	for (int runs_per_scen = 0; runs_per_scen < 100; runs_per_scen++) {		
		PP pp(instance, 1);
		pp.preprocess(true, true, true);
		pp.computeRandomOrdering();
		int sum_of_costs = pp.run();
		cout << sum_of_costs << endl;
		ofs << ( (sum_of_costs == MAX_COST) ? sum_of_costs : ((double)sum_of_costs / sum_start_goal_dist) )<< ", ";
	}
	ofs << "]" << endl;
	ofs.close();
}

int main(int argc, char** argv)
{
	bool normalize_flag = false;
	if (normalize_flag) {
		int num_agents = 200;
		// If on linux, change "../ to "../out/build/x64-Release/
		string out_path = "../liblinear_files/warehouse-10-20-10-2-1/num_agent_" + std::to_string(num_agents) + "/";
		SVMRankAPI svm_rank_api("", out_path, "", "", num_agents, "", "");
		svm_rank_api.rawToNormalized("test_1to25_fakelabel_raw.dat", "test_1to25_fakelabel_norm.dat");
		//svm_rank_api.rawToNormalized("test_1to25_raw.dat", "test_1to25_norm.dat");
	}
	bool group_ranking = false;
	if (group_ranking) {
		int num_agents = 400;
		string out_path = "../svm_rank_files/ost003d/num_agent_" + std::to_string(num_agents) + "/";
		SVMRankAPI svm_rank_api("", out_path, "", "", num_agents, "", "");
		svm_rank_api.groupRanking("train_1to25_norm.dat", "train_1to25_norm_group5.dat", 5);
		//svm_rank_api.groupRanking("train_1to25_norm.dat", "train_1to25_norm_group10.dat", 10);
	}
	bool fix_qid = false; 
	if (fix_qid) {
		int num_agents = 20;
		string out_path = "../svm_rank_files/random-32-32-20/num_agent_" + std::to_string(num_agents) + "/";
		SVMRankAPI svm_rank_api("", out_path, "", "", num_agents, "", "");
		svm_rank_api.fixQid("train_1to25_norm_group5.dat", "train_1to25_norm_group5_qidfixed.dat");
		//svm_rank_api.groupRanking("train_1to25_norm.dat", "train_1to25_norm_group10.dat", 10);
	}
	bool get_dotplot_data = false;
	if (get_dotplot_data) {
		pp100_dotplotdata();
		return 0;
	}	
	//return 0;

	vector<int> num_agents_to_train = { 55 };
	 vector<int> num_agents_to_test = { 55 };
	for (auto agentNumTrain : num_agents_to_train) {

		int num_agent_train = agentNumTrain;
		  for (auto agentNumTest : num_agents_to_test) {
			// ========================================= CUSTOMIZE begin ========================================
			int num_agent_test = agentNumTest;// agentNumTrain;
			vector<int> pp_runs = {0,1,2,3}; //0: LH; 1: SH; 2: ML; 3: R; must be ascending sorted
			string map_name = "random-32-32-20"; //random-32-32-20;  warehouse-10-20-10-2-1
			bool useLiblinear = true;
			string train_file = useLiblinear? "train_25x1top5_commondep_liblinear": "train_1to25_norm_group5.dat";
			string test_file = "test_1to25_fakelabel_norm.dat"; //
			bool has_model_file = false;
			bool rank_random_sampling = true;
			bool random_restart = true;
			bool fixedtime_restart = true;
			double restart_time_constraint = 60.0;
			if(map_name=="lak303d" || map_name=="ost003d" || map_name=="brc202d")
				restart_time_constraint = 600.0;
			
			bool use_custom_agent_file = false;			
			srand((int)time(0));  //comment this back after done with testing random-32-32-20.map for 150 agents
			//srand(908);
			// ========================================= CUSTOMIZE end ==========================================			
			bool liblinearValidation = false;
			bool testTopologicalOrder = false;

			int total_scenarios = 25; //<=25. folder: random-32-32-20.map-scen-random
			string repo_level_path, out_path;
			if (ONWINDOWS) {
				repo_level_path = "../../../../";
				out_path = "../";
			}
			else if (ONLINUX) {
				repo_level_path = "../";
				out_path = "../out/build/x64-Release/";
			}
			string map_folder_path = repo_level_path + "map/";
			string map_file = map_folder_path + map_name + ".map";
			string scenario_file = map_folder_path + map_name + ".map-scen-random/" + map_name + "-random-1.scen";
			if (use_custom_agent_file) {
				scenario_file = map_folder_path + map_name + ".map-scen-random/" + map_name + "-random-agents" + std::to_string(num_agent_test) + "-1.scen";
			}
			cout << scenario_file << endl; //for debug

			string scen_sub = scenario_file.substr(0, scenario_file.size() - 6); //6 is the len of '1.scen'
			string scen_postfix = ".scen";

			out_path += useLiblinear ? "liblinear_files" : "svm_rank_files";
			out_path += "/" + map_name + "/num_agent_";
			string train_out_path = out_path + std::to_string(num_agent_train) + "/";
			string test_out_path = out_path + std::to_string(num_agent_test) + "/";

			string train_out_fname = train_out_path + train_file; //string train_out_fname = "training_on_same_model";	
			string test_out_fname = test_out_path + test_file;
			string weights_fname = train_out_fname + "_weights";

			string svm_model_fname = "model_train" + std::to_string(num_agent_train);
			string train_num_test_num_postfix = "_train" + std::to_string(num_agent_train) + "_test" + std::to_string(num_agent_test);
			string svm_prediction_fname = "prediction" + train_num_test_num_postfix;
			string comparison_results_fname = train_out_path + "result" + train_num_test_num_postfix + (useLiblinear ? "_liblinear" : ".txt");


		//pretend to generate test features online
		double online_gen_testfile_sec = 0.0;
		clock_t start_online_gen_testfile = clock();
		/*string offline_gen_testfile_cmd = (ONLINUX ? "./" : "") + repo_level_path + "training/pp_train -m " +
			map_file + " -a " + scenario_file + " -o test.csv -k " + std::to_string(agentNumTrain);
		cout<<offline_gen_testfile_cmd<<endl;
		if (ONWINDOWS) {
			std::replace(offline_gen_testfile_cmd.begin(), offline_gen_testfile_cmd.end(), '/', '\\');
		}
		system(offline_gen_testfile_cmd.c_str());*/


			//---------------train SVM and generate prediction on test file---------------
			SVMRankAPI svm_rank_api(repo_level_path + (ONWINDOWS ? "svm_rank_windows/" : "svm_rank_linux/"),
				train_out_path, train_out_fname, test_out_fname,
				num_agent_train, svm_model_fname, svm_prediction_fname);
			LiblinearAPI liblinear_api(repo_level_path + "liblinear_weights_windows/", train_out_fname, test_out_fname,
				num_agent_train, weights_fname);
			vector<vector<int>> topo_orderings;
			if (useLiblinear) {
				if (testTopologicalOrder) {
					string dependency_stats_fname = "..\\svm_rank_files\\num_agent_" + std::to_string(num_agent_test) + "\\dependency_stats.txt";
					std::ifstream stats_ifile(dependency_stats_fname);
					string line;
					while (std::getline(stats_ifile, line)) {
						line.pop_back(); //remove white space at end of line
						vector<string> topo_order_strings = Split(line, ' ');
						vector<int> topo_order;
						for (int i = 0; i < topo_order_strings.size(); i++) {
							topo_order.push_back(boost::lexical_cast<int>(topo_order_strings[i]));
						}
						topo_orderings.push_back(topo_order);
					}
					cout << "topo_orderings.size: " << topo_orderings.size() << endl;
					cout << "topo_orderings[0].size: " << topo_orderings[0].size() << endl;
				}
				else {
					if(!has_model_file)
						liblinear_api.trainLiblinear();
					liblinear_api.rankPredictions(num_agent_test, rank_random_sampling);
					cout << "liblinear_api.orderings.size: " << liblinear_api.orderings.size() << endl;
					cout << "liblinear_api.orderings[0].size: " << liblinear_api.orderings[0].size() << endl;
				}
				//return 0;
			}
			else {
				svm_rank_api.trainAndPredictTest(false); //false = no cross validation
				svm_rank_api.rankPredictions(num_agent_test, rank_random_sampling);
				cout << "svm_rank_api.orderings.size: " << svm_rank_api.orderings.size() << endl;
				cout << "svm_rank_api.orderings[0].size: " << svm_rank_api.orderings[0].size() << endl;
				//return 0;
			}
			vector<vector<int>>& ML_trained_orderings = useLiblinear ? (testTopologicalOrder ? topo_orderings : liblinear_api.orderings) : svm_rank_api.orderings;

		online_gen_testfile_sec += (double)(clock() - start_online_gen_testfile) / CLOCKS_PER_SEC;
		//cout<<"online rank prediction took " << online_gen_testfile_sec <<" seconds"<<endl;
			//return 0;

			//---------------Run pp on LH, SH, SVM, Random Orderings---------------
			vector<vector<double> > all_costs_norm;
			vector<vector<double>> all_runtime;
			vector<vector<int>> all_failed_agent_idx;
			vector<vector<int>> all_restart_count;
			vector<vector<vector<double>>> rrfirst_byproduct(4, vector<vector<double>>(4, vector<double>(0)));
			vector<double> all_ml_overhead;

			for (int i = 0; i < 4; i++) { // Assume maximum run is 4
				vector<double> vec(0, 0);
				vector<int> vec_int(0, 0);
				all_costs_norm.push_back(vec);
				all_runtime.push_back(vec);
				all_failed_agent_idx.push_back(vec_int);
				all_restart_count.push_back(vec_int);
			}
			int counter = 0;
			for (int scen = 1; scen <= total_scenarios; scen++) { //generate training data
				//hardcoded for test_agent=200, delete immediately after running!! (deleted)

				string scenario_fname = scen_sub + boost::lexical_cast<string>(scen) + scen_postfix;
				cout << scenario_fname << endl; //for debug. delete later

				//Run test
				// load the instance
				bool first_agents; // true - read the first N start and goal locations from the file;
								   // false - read random N start locations and random N goal locations from the file.
				first_agents = true; //comment this back after done with testing random-32-32-20.map for 150 agents
				//first_agents = false;

				//srand((int)time(0));
				Instance instance(map_file, scenario_fname, first_agents, num_agent_test);

				// solve the instance
				boost::optional<LiblinearAPI&> liblinearapi_param = liblinear_api;
				boost::optional<SVMRankAPI&> svmrankapi_param = svm_rank_api;
				if (useLiblinear) {
					svmrankapi_param = boost::none;
				}
				else {
					liblinearapi_param = boost::none;
				}
				solvePP(instance, 1, pp_runs, counter, true, all_ml_overhead, ML_trained_orderings,
					all_costs_norm, rrfirst_byproduct, 0,
					random_restart, fixedtime_restart, liblinearapi_param, svmrankapi_param, all_runtime,
					all_failed_agent_idx, all_restart_count, 
					restart_time_constraint); //scen
				counter++;
			}

			vector<int> no_solu_scens;
			vector<int> yes_solu_scens;
			for (int scen = 0; scen < all_costs_norm[2].size(); scen++) { //loop thru ML PP results
				if (all_costs_norm[2][scen] == MAX_COST) { //no solution
					no_solu_scens.push_back(scen);
				}
				else {
					yes_solu_scens.push_back(scen);
				}
			}

			//TODO: Read in the depdency stats file for the "ground truth" dependency graph that the model trained on
			// then compare each dep pair with the ML_trained_orderings
			if (liblinearValidation) {
				int top_picks = 5;
				vector<vector<set<pair<int, int>>>> deppair_by_occurance(total_scenarios, vector<set<pair<int, int>>>(top_picks));
				std::ifstream dep_ifile(train_out_path + "dependency_stats.txt");
				string line;
				for (int i = 0; i < total_scenarios; i++) {
					std::getline(dep_ifile, line); //first line empty
					std::getline(dep_ifile, line); //2nd line comment
					for (int occur = 0; occur < top_picks; occur++) {
						std::getline(dep_ifile, line); //all pairs
						vector<string> pairs_rawform = Split(line, ' ');
						for (int j = 0; j < pairs_rawform.size(); j++) {
							if (pairs_rawform[j].size() == 0) continue;
							pairs_rawform[j] = pairs_rawform[j].substr(1, pairs_rawform[j].size() - 2);
							vector<string> two_int = Split(pairs_rawform[j], ',');
							deppair_by_occurance[i][occur].insert(
								make_pair(boost::lexical_cast<int>(two_int[0]), boost::lexical_cast<int>(two_int[1])));

						}
					}
				}

				//compare ML-predicted ordering with ground truth dependency graph
				cout << "scenarios with ML solution: \n";
				for (auto scen : yes_solu_scens) { //total_scenarios
					printValidationLossAnalysis(scen, top_picks, ML_trained_orderings, deppair_by_occurance);
				}
				cout << "\nscenarios without ML solution: \n";
				for (auto scen : no_solu_scens) { //total_scenarios
					printValidationLossAnalysis(scen, top_picks, ML_trained_orderings, deppair_by_occurance);
				}
			}

			//---------------------------Compare Costs---------------------------

			std::ofstream ofs(comparison_results_fname, std::ios_base::app);
			// std::vector<double> testfile_predict_overhead = {online_gen_testfile_sec};
			// print1DVectorCannotUseSolvepp(ofs, testfile_predict_overhead, "generate testfile and rank prediction (sec):", true);

			if (fixedtime_restart) {
				ofs << "\n// rr-best" << endl;
				print1DVectorCannotUseSolvepp(ofs, all_ml_overhead, "ML overhead (not ratio):", true);
				print1DVectorCannotUseSolvepp(ofs, all_runtime[2], "ML-T/P total runtime:", true);
			}
			comparisonAnalysis(ofs, all_costs_norm, all_failed_agent_idx, all_runtime, all_restart_count);

			if (fixedtime_restart) {
				// change array type in a very stupid way
				vector<vector<int>> rrfirst_failedidx(rrfirst_byproduct[2].size(), vector<int>(0));
				for (int i = 0; i < rrfirst_byproduct[2].size(); i++) {
					for (int j = 0; j < rrfirst_byproduct[2][i].size(); j++) {
						rrfirst_failedidx[i].push_back(round(rrfirst_byproduct[2][i][j]));
					}
				}

				vector<vector<int>> rrfirst_restartcnt(rrfirst_byproduct[3].size(), vector<int>(0));
				for (int i = 0; i < rrfirst_byproduct[3].size(); i++) {
					for (int j = 0; j < rrfirst_byproduct[3][i].size(); j++) {
						rrfirst_restartcnt[i].push_back(round(rrfirst_byproduct[3][i][j]));
					}
				}
				//rrfirst_byproduct  [0]:all_costs, [1]:all_runtime, [2]:all_failed_agent_idx, [3]:all_restart_count
				ofs << "\n// rr-first" << endl;
				comparisonAnalysis(ofs, rrfirst_byproduct[0], rrfirst_failedidx, rrfirst_byproduct[1], rrfirst_restartcnt);
			}
		  }
	}
}