/* Copyright (C) Jiaoyang Li
* Unauthorized copying of this file, via any medium is strictly prohibited
* Confidential
* Written by Jiaoyang Li <jiaoyanl@usc.edu>, May 2020
*/

/*driver.cpp
* Solve a MAPF instance on 2D grids.
*/
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include "ECBS.h"
#include "PP.h"
#include "common.h"
#include "MLFeaturesInstance.h"
#include "MLFeaturesAgent.h"
#include "MLFeatures.h"
#include "solvePP.h"
#include "SVMRankAPI.h"

bool runEECBS(const boost::program_options::variables_map& vm, Instance& instance)
{
    if (vm["suboptimality"].as<double>() < 1)
    {
        cerr << "Suboptimal bound should be at least 1!" << endl;
        return false;
    }

    high_level_solver_type s;
    if (vm["highLevelSolver"].as<string>() == "A*")
        s = high_level_solver_type::ASTAR;
    else if (vm["highLevelSolver"].as<string>() == "A*eps")
        s = high_level_solver_type::ASTAREPS;
    else if (vm["highLevelSolver"].as<string>() == "EES")
        s = high_level_solver_type::EES;
    else if (vm["highLevelSolver"].as<string>() == "NEW")
        s = high_level_solver_type::NEW;
    else
    {
        cout << "WRONG high level solver!" << endl;
        return false;
    }

    if (s == high_level_solver_type::ASTAR && vm["suboptimality"].as<double>() > 1)
    {
        cerr << "A* cannot perform suboptimal search!" << endl;
        return false;
    }

    heuristics_type h;
    if (vm["heuristics"].as<string>() == "Zero")
        h = heuristics_type::ZERO;
    else if (vm["heuristics"].as<string>() == "CG")
        h = heuristics_type::CG;
    else if (vm["heuristics"].as<string>() == "DG")
        h = heuristics_type::DG;
    else if (vm["heuristics"].as<string>() == "WDG")
        h = heuristics_type::WDG;
    else
    {
        cout << "WRONG heuristics strategy!" << endl;
        return false;
    }

    if ((h == heuristics_type::CG || h == heuristics_type::DG) && vm["lowLevelSolver"].as<bool>())
    {
        cerr << "CG or DG heuristics do not work with low level of suboptimal search!" << endl;
        return false;
    }

    heuristics_type h_hat; // inadmissible heuristics
    if (s == high_level_solver_type::ASTAR ||
        s == high_level_solver_type::ASTAREPS ||
        vm["inadmissibleH"].as<string>() == "Zero")
        h_hat = heuristics_type::ZERO;
    else if (vm["inadmissibleH"].as<string>() == "Global")
        h_hat = heuristics_type::GLOBAL;
    else if (vm["inadmissibleH"].as<string>() == "Path")
        h_hat = heuristics_type::PATH;
    else if (vm["inadmissibleH"].as<string>() == "Local")
        h_hat = heuristics_type::LOCAL;
    else if (vm["inadmissibleH"].as<string>() == "Conflict")
        h_hat = heuristics_type::CONFLICT;
    else
    {
        cout << "WRONG inadmissible heuristics strategy!" << endl;
        return false;
    }

    conflict_selection conflict = conflict_selection::EARLIEST;
    node_selection n = node_selection::NODE_CONFLICTPAIRS;

    srand(0);
    int runs = 1;
    //////////////////////////////////////////////////////////////////////
    // initialize the solver
    if (vm["lowLevelSolver"].as<bool>())
    {
        ECBS ecbs(instance, false, vm["screen"].as<int>());
        ecbs.setPrioritizeConflicts(vm["prioritizingConflicts"].as<bool>());
        ecbs.setDisjointSplitting(vm["disjointSplitting"].as<bool>());
        ecbs.setBypass(vm["bypass"].as<bool>());
        ecbs.setRectangleReasoning(vm["rectangleReasoning"].as<bool>());
        ecbs.setCorridorReasoning(vm["corridorReasoning"].as<bool>());
        ecbs.setHeuristicType(h, h_hat);
        ecbs.setTargetReasoning(vm["targetReasoning"].as<bool>());
        ecbs.setMutexReasoning(false);
        ecbs.setConflictSelectionRule(conflict);
        ecbs.setNodeSelectionRule(n);
        ecbs.setSavingStats(vm["stats"].as<bool>());
        ecbs.setHighLevelSolver(s, vm["suboptimality"].as<double>());
        //////////////////////////////////////////////////////////////////////
        // run
        double runtime = 0;
        int lowerbound = 0;
        for (int i = 0; i < runs; i++)
        {
            ecbs.clear();
            ecbs.solve(vm["cutoffTime"].as<double>(), lowerbound);
            runtime += ecbs.runtime;
            if (ecbs.solution_found)
                break;
            lowerbound = ecbs.getLowerBound();
            ecbs.randomRoot = true;
        }
        ecbs.runtime = runtime;
        if (vm.count("output"))
            ecbs.saveResults(vm["output"].as<string>(), vm["agents"].as<string>());
        /*size_t pos = vm["output"].as<string>().rfind('.');      // position of the file extension
        string output_name = vm["output"].as<string>().substr(0, pos);     // get the name without extension
        cbs.saveCT(output_name); // for debug*/
        if (vm["stats"].as<bool>())
        {
            ecbs.saveStats(vm["output"].as<string>(), vm["agents"].as<string>());
        }
        ecbs.clearSearchEngines();
    }
    else
    {
        CBS cbs(instance, false, vm["screen"].as<int>());
        cbs.setPrioritizeConflicts(vm["prioritizingConflicts"].as<bool>());
        cbs.setDisjointSplitting(vm["disjointSplitting"].as<bool>());
        cbs.setBypass(vm["bypass"].as<bool>());
        cbs.setRectangleReasoning(vm["rectangleReasoning"].as<bool>());
        cbs.setCorridorReasoning(vm["corridorReasoning"].as<bool>());
        cbs.setHeuristicType(h, h_hat);
        cbs.setTargetReasoning(vm["targetReasoning"].as<bool>());
        cbs.setMutexReasoning(false);
        cbs.setConflictSelectionRule(conflict);
        cbs.setNodeSelectionRule(n);
        cbs.setSavingStats(vm["stats"].as<bool>());
        cbs.setHighLevelSolver(s, vm["suboptimality"].as<double>());
        //////////////////////////////////////////////////////////////////////
        // run
        double runtime = 0;
        int lowerbound = 0;
        for (int i = 0; i < runs; i++)
        {
            cbs.clear();
            cbs.solve(vm["cutoffTime"].as<double>(), lowerbound);
            runtime += cbs.runtime;
            if (cbs.solution_found)
                break;
            lowerbound = cbs.getLowerBound();
            cbs.randomRoot = true;
        }
        cbs.runtime = runtime;
        if (vm.count("output"))
            cbs.saveResults(vm["output"].as<string>(), vm["agents"].as<string>());
        if (vm["stats"].as<bool>())
        {
            cbs.saveStats(vm["output"].as<string>(), vm["agents"].as<string>());
        }
        cbs.clearSearchEngines();
    }
    return true;
}

/* Main function */
int main(int argc, char** argv)
{
	namespace po = boost::program_options;
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")

		// params for the input instance and experiment settings
		("map,m", po::value<string>()->required(), "input file for map")
		("agents,a", po::value<string>()->required(), "input file for agents")
		("agentNum,k", po::value<int>()->default_value(0), "number of agents")
		("screen,s", po::value<int>()->default_value(1), "screen option (0: none; 1: results; 2:all)")
        ("solver", po::value<string>()->default_value("PP"), "MAPF solver (EECBS, PP)")

		// params for EECBS
		("cutoffTime,t", po::value<double>()->default_value(7200), "cutoff time (seconds)")
		("stats", po::value<bool>()->default_value(false), "write to files some statistics")
        ("output,o", po::value<string>(), "output file for schedule")
		// // params for CBS node selection strategies
		("highLevelSolver", po::value<string>()->default_value("EES"), "the high-level solver (A*, A*eps, EES, NEW)")
		("lowLevelSolver", po::value<bool>()->default_value(true), "using suboptimal solver in the low level")
		("inadmissibleH", po::value<string>()->default_value("Global"), "inadmissible heuristics (Zero, Global, Path, Local, Conflict)")
		("suboptimality", po::value<double>()->default_value(1.2), "suboptimality bound")
		// // params for CBS improvement
		("heuristics", po::value<string>()->default_value("WDG"), "admissible heuristics for the high-level search (Zero, CG,DG, WDG)")
		("prioritizingConflicts", po::value<bool>()->default_value(true), "conflict prioirtization. If true, conflictSelection is used as a tie-breaking rule.")
		("bypass", po::value<bool>()->default_value(true), "Bypass1")
		("disjointSplitting", po::value<bool>()->default_value(false), "disjoint splitting")
		("rectangleReasoning", po::value<bool>()->default_value(true), "rectangle reasoning")
		("corridorReasoning", po::value<bool>()->default_value(true), "corridor reasoning")
		("targetReasoning", po::value<bool>()->default_value(true), "target reasoning")
		;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);

	if (vm.count("help")) {
		cout << desc << endl;
		return 1;
	}

	po::notify(vm);

    // ========================================= CUSTOMIZE begin ========================================
    double test_feature_gen_runtime = 0.0;
    vector<int> num_agents_to_train = {vm["agentNum"].as<int>()}; //{50};
    //if SVMRank: "train_1to25_raw.dat"; LIBLINEAR: "train_25x1top5_commondep_liblinear"
    string train_file = "train_25x1top5_commondep_liblinear"; 
    string test_file = "test_1to25_fakelabel_raw.dat";
    for (auto agentNum : num_agents_to_train) {
        bool useLIBLINEAR = true; //if false then use SVM-Rank
        bool generateTestFile = false && (!useLIBLINEAR); //if using LIBLINEAR, then assume generating train file
        bool solvePPforTestFile = false;
        int curriculum_learning_numagent = -1; //-1 for no curr learning, otherwise will do curr learning
        int total_scenarios = 25;
        int instances_per_scen = (useLIBLINEAR || generateTestFile)? 1 : 100; //100: 99 for train, 1 for test
        int qid = 0;
        
            /* edit these if need to generate more agents than the map provides */
        bool generate_agents = false;
        bool save_agents = generate_agents && generateTestFile; //if generating test file, then need to save agents
		int num_of_rows = 0;
        int num_of_cols = 0;
        int num_of_obstacles = 0;
        int warehouse_width = 0;
        // ========================================= CUSTOMIZE end ==========================================
       
        string map_name = Split(vm["map"].as<string>(), '/').back();
        map_name = map_name.substr(0, map_name.size() - 4); //4 is the len of '.map'
        string scenario_file = vm["agents"].as<string>();
        string scen_sub = scenario_file.substr(0, scenario_file.size() - 6); //6 is the len of '1.scen'
        string scen_postfix = ".scen";
        
        string x64_level_path, repo_level_path;
        if (ONWINDOWS) {
		    repo_level_path = "../../../../";
            x64_level_path = "../";
        }
        else if (ONLINUX) {
		    repo_level_path = "../";
            x64_level_path = "../out/build/x64-Release/";
        }
        string out_path = x64_level_path + ((useLIBLINEAR || generateTestFile) ? "liblinear_files/" : "svm_rank_files/")
            + map_name + "/num_agent_";
        string currlearn_train_fpath = out_path + std::to_string(curriculum_learning_numagent) + "/";
        string currlearn_train_fname = currlearn_train_fpath 
            + (useLIBLINEAR? "train_25x1top5_commondep_liblinear": "train_1to25_norm_group5.dat");

        string train_out_fname = out_path + std::to_string(agentNum) + "/" + train_file;
        string test_out_fname = out_path + std::to_string(agentNum) + "/" + test_file;
        string liblinear_weights_fname = train_out_fname + "_weights";

        LiblinearAPI liblinear_api(repo_level_path + "liblinear_weights_windows/", currlearn_train_fname, "",
            curriculum_learning_numagent, currlearn_train_fname + "_weights");
        SVMRankAPI svm_rank_api(repo_level_path + (ONWINDOWS ? "svm_rank_windows/" : "svm_rank_linux/"),
            currlearn_train_fpath, currlearn_train_fname, "",
            curriculum_learning_numagent, "model_train" + std::to_string(curriculum_learning_numagent), 
            "prediction_train" + std::to_string(curriculum_learning_numagent) + "_test" + std::to_string(agentNum));
        bool LIBLINEAR_validation = false;
        //remove(train_out_fname.c_str()); //if file already exists, remove file to start fresh
        //remove(test_out_fname.c_str());
        // remove(liblinear_weights_fname.c_str());
        
        //vm["solver"].as<string>(), total_scenarios, scenario_file, training or testing bool, 
        int start_scen = 1;
        for (int scen = start_scen; scen <= total_scenarios; scen++) { //generate training data
            string scenario_fname = scen_sub + std::to_string(scen) + scen_postfix;
            if(generate_agents && save_agents){
                scenario_fname = scen_sub + "agents" + std::to_string(agentNum) +"-"+ std::to_string(scen) + scen_postfix;
            }
            cout << scenario_fname << endl; //for debug. delete later

            //Run 100 instances from each scenario. last one for testing, all others for training
            for (int inst = 0; inst < instances_per_scen; inst++) {
                qid++;
                // load the instance
                bool first_agents; // true - read the first N start and goal locations from the file;
                                // false - read random N start locations and random N goal locations from the file.
                string out_fname;
                if (inst == instances_per_scen - 1) { //last one for test file
                    if (useLIBLINEAR) {
                        //continue; //testing file is useless for liblinear
                        //LIBLINEAR_validation = true;
                        first_agents = false;
                        out_fname = train_out_fname;
                    }
                    else {
                        first_agents = true;
                        out_fname = test_out_fname;
                    }
                }
                else {
                    if (useLIBLINEAR) {
                        LIBLINEAR_validation = false;
                    }
                    first_agents = false;
                    out_fname = train_out_fname;
                }

                int pp_runs = 100; // number of runs for PP. Should be 100 for training data
                srand((int)time(0));
                // Instance instance(vm["map"].as<string>(), scenario_fname, first_agents,
                //     vm["agentNum"].as<int>());
                Instance instance(vm["map"].as<string>(), scenario_fname, first_agents, agentNum, 
                    generate_agents, save_agents, num_of_rows, num_of_cols, 
                    num_of_obstacles, warehouse_width);

                // solve the instance
                if (vm["solver"].as<string>() == "PP")
                {
                    std::ofstream ofs(out_fname, std::ios_base::app); //concatenate
                    if (generateTestFile && !solvePPforTestFile) { //generate test file without solving PP (give dummy labels, only features matter)    
                        genTestFileNoLabel(ofs, instance, vm["screen"].as<int>(), qid, test_feature_gen_runtime);
                        continue;
                    }
                    if (useLIBLINEAR) { //get Liblinear training file
                        std::ofstream weights_ofs(liblinear_weights_fname, std::ios_base::app); //concatenate
                        if (curriculum_learning_numagent == -1) {
                            LiblinearAPI dummy_api("", "", "", 0, "");
                            solvePP(ofs, weights_ofs, instance, vm["screen"].as<int>(), pp_runs, scen, dummy_api, 
                                qid, LIBLINEAR_validation, curriculum_learning_numagent);
                        }
                        else {
                            solvePP(ofs, weights_ofs, instance, vm["screen"].as<int>(), pp_runs, scen, liblinear_api, 
                                qid, LIBLINEAR_validation, curriculum_learning_numagent);
                        }
                    }
                    else { //get SVMRank training file, or validation file "validation_1to25_norm.dat"
                        solvePP(ofs, instance, vm["screen"].as<int>(), pp_runs, scen, false, qid, 
                            curriculum_learning_numagent, svm_rank_api);

                       
                    }
                }
                else if (vm["solver"].as<string>() == "EECBS")
                    runEECBS(vm, instance);
                else
                {
                    cerr << "MAPF solver " << vm["solver"].as<string>() << "does not exist!" << endl;
                    return -1;
                }
            }
        }
        bool normalize_flag = true;
        bool group_ranking = true;
        if (normalize_flag) {
            SVMRankAPI svm_rank_api("", out_path + std::to_string(agentNum) + "/", "", "", agentNum,"","");
            if(!useLIBLINEAR && (!generateTestFile)){
                svm_rank_api.rawToNormalized("train_1to25_raw.dat", "train_1to25_norm.dat");
            }
            if(generateTestFile || (!useLIBLINEAR)){
                svm_rank_api.rawToNormalized("test_1to25_fakelabel_raw.dat", "test_1to25_fakelabel_norm.dat");
            }
        }
        if (group_ranking && (!useLIBLINEAR) && (!generateTestFile)) {
            SVMRankAPI svm_rank_api("", out_path + std::to_string(agentNum) + "/", "", "", agentNum, "", "");
            svm_rank_api.groupRanking("train_1to25_norm.dat", "train_1to25_norm_group5.dat", 5);
            cout<<"group ranking done"<<endl;
        }
    }
    cout<<"Time to generate test files: " << test_feature_gen_runtime / CLOCKS_PER_SEC<<endl;
	return 0;
}
