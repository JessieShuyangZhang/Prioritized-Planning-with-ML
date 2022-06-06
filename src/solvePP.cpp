#include "solvePP.h"
#include "Instance.h"
#include "PP.h"
#include "MLFeaturesInstance.h"
#include "MLFeaturesAgent.h"
#include "MLFeatures.h"
#include "liblinearFeatures.h"
#include "LiblinearAPI.h"
#include "SVMRankAPI.h"
#include <numeric>      // std::iota
#include <algorithm>    // std::sort, std::stable_sort


vector<vector<double>> getOriginalFeatureVectors(Instance& instance, PP& pp, vector<int>& best_ordering) //why do you need best ordering?
{
    vector<vector<double>> all_raw_feature_vecs(best_ordering.size()); //size = num_of_agents
    MLFeaturesInstance ml_features_instance(instance, pp); //only pp.agent is used
    ml_features_instance.hashMDDnodeOccurances();
    for (auto i : best_ordering) {
        MLFeaturesAgent ml_features_agent(instance, i, pp, ml_features_instance);//only pp.agent is used
        MLFeatures ml_features;
        vector<double> feature_vec;
        ml_features.generateFeatureVec(ml_features_instance, ml_features_agent, feature_vec);

        all_raw_feature_vecs[i] = feature_vec; //all_raw_feature_vecs[i] has agent i's feature vec (not subject to pp ordering)
    }
    return all_raw_feature_vecs; //TODO: this is expensive. Is there a better way?
}

void computePPOrdering(int i, PP& pp, int scen, bool is_test, 
    vector<vector<int> >& SVM_Trained_orderings, bool random_restart) 
{
    //------ computes PP ordering using different methods: LH heuristic, SH heuristic, SVM_Trained_orderings, Random ------
    if (i == 0)
        pp.computeLHOrdering(random_restart);
    else if (i == 1)
        pp.computeSHOrdering(random_restart);
    else if (is_test && i == 2) {
        pp.ordering = SVM_Trained_orderings[scen];
        //print1DVector(cout, pp.ordering);
    }
    else {
        pp.computeRandomOrdering();
    }
    //print1DVector(cout, pp.ordering);
}

// TODO: record ML overhead HERE
void computePPOrderingCurrLearning(int i, Instance& instance, PP& pp, int scen, 
    bool random_restart, int num_agent_test,
    boost::optional<LiblinearAPI&> liblinear_api,
    boost::optional<SVMRankAPI&> svm_rank_api) //assume max(i) will be ~100
{
    //------ computes PP ordering using different methods: LH heuristic, SH heuristic, ML_Trained_orderings, Random ------
    if (i == 0)
        pp.computeLHOrdering(random_restart);
    else if (i == 1)
        pp.computeSHOrdering(random_restart);
    else if (i >= 2 && i<=10) {
        // Read in model file, generate our feature vec, dotprod, get random sampling prediction (total ordering)   

        //liblinear_api.weightsDotProdFeatures(num_agent_test);
        if (liblinear_api != boost::none) {
            liblinear_api->rankPredictions(num_agent_test, true);
            pp.ordering = liblinear_api->orderings[0];
        }
        else {
            svm_rank_api->rankPredictions(num_agent_test, true);
            pp.ordering = svm_rank_api->orderings[0];

        }
        //print1DVector(cout, pp.ordering);
    }
    else {
        pp.computeRandomOrdering();
    }
    //print1DVector(cout, pp.ordering);
}

void store_failed_agent_idx(int i, int agent_id, PP& pp, boost::optional<vector<vector<int>>&> all_failed_agent_idx)
{
    if (agent_id != -1) { //no solution found
        auto failed_agent = std::find(pp.ordering.begin(), pp.ordering.end(), agent_id);
        int failed_index = failed_agent - pp.ordering.begin();
        all_failed_agent_idx.get()[i].push_back(failed_index);
    }
}

void store_failed_agent_idx(int i, int agent_id, PP& pp, vector<vector<double>>& all_failed_agent_idx)
{
    if (agent_id != -1) { //no solution found
        auto failed_agent = std::find(pp.ordering.begin(), pp.ordering.end(), agent_id);
        int failed_index = failed_agent - pp.ordering.begin();
        all_failed_agent_idx[i].push_back(failed_index);
    }
}

void solvePPLiblinear(std::ofstream& ofs, std::ofstream& weights_ofs, Instance& instance, int pp_screen, int pp_runs,
    int& scen, vector<vector<int> >& SVM_Trained_orderings, vector<vector<double> >& all_costs,
    LiblinearAPI& liblinear_api, int qid, bool is_validation, int curr_learning_numagent) {

    //driver_test won't call this function. It will invoke solvePPSVMRank instead

    PP pp(instance, pp_screen);
    pp.preprocess(true, true, true);
    int num_agent = instance.num_of_agents;

    if (curr_learning_numagent != -1) {
        vector<int> dummy_ordering(num_agent);
        std::iota(dummy_ordering.begin(), dummy_ordering.end(), 0);
        vector<vector<double>> all_orig_feature_vecs = getOriginalFeatureVectors(instance, pp, dummy_ordering);
        vector<vector<double>> all_norm_feature_vecs = normalizeFeatureVectors(all_orig_feature_vecs);
        liblinear_api.setTestFeatures(all_norm_feature_vecs, dummy_ordering);
    }

    vector<tuple<int, vector<int>, vector<set<int>>>> cost_ordering_dependency;
    for (int i = 0; i < pp_runs; i++)
    {
        //when training, use random restart on LH and SH
        int sum_of_costs = MAX_COST;
        double total_runtime = 0;
         
        if (curr_learning_numagent == -1) {
            computePPOrdering(i, pp, scen - 1, false, SVM_Trained_orderings);
            sum_of_costs = pp.run();
            total_runtime = pp.runtime;
        }
        else {
            //computePPOrderingCurrLearning(i, instance, pp, scen - 1, true, num_agent, liblinear_api);
            computePPOrderingCurrLearning(i, instance, pp, scen - 1, true, num_agent, liblinear_api);
            sum_of_costs = pp.run();
            total_runtime += pp.runtime;
            if(i<=10){
            while (abs(sum_of_costs - MAX_COST) < 0.0001 && total_runtime <= 5) { //time constraint
               pp.reset();
               computePPOrderingCurrLearning(i, instance, pp, scen - 1, true, num_agent, liblinear_api);
               sum_of_costs = pp.run();
               total_runtime += pp.runtime;
            }
            }
        }
        // int sum_of_costs = pp.run();   
        // sum_of_costs = pp.run();

        // cout << "Iteration " << i << "\t\t runtime = " << pp.runtime << " seconds.";
        cout << "Iteration " << i << "\t\t runtime = " << total_runtime << " seconds.";
        cout << " sum of costs = " << sum_of_costs << endl;
        if (sum_of_costs < MAX_COST) {
            cost_ordering_dependency.push_back(make_tuple(sum_of_costs, pp.ordering, pp.dependency_graph));//manual_dependency_graph));// 
        }
        pp.reset();
    }

    //------------------------get the top 5% solutions and print their dependency feature vectors for liblinear------------------------
    std::sort(cost_ordering_dependency.begin(), cost_ordering_dependency.end());
    int top_picks = 5;
    int cut_off_index = std::min(top_picks, (int)cost_ordering_dependency.size()); // Pick TOP 5% or less
    if (cut_off_index == 0) { //no solution is found within pp_runs (usually 100 runs)
        cout << "scen "<<scen<< " no solution, running again" << endl;
        scen -= 1;
        return; //is this right? but what about test files? 
    }
    
    vector<vector<int>> total_dependency_matrix(num_agent, vector<int>(num_agent, MAX_COST));
    //will hold all pairs (including inferred) from top5 graphs

    unordered_map<pair<int, int>, int> dep_pair_count; //store all top5 dependency pairs. pair<i,j> means i->j, i has higher priority
    vector<vector<double>> all_orig_feature_vecs = getOriginalFeatureVectors(instance, pp, std::get<1>(cost_ordering_dependency[0]));
    vector<vector<double>> all_norm_feature_vecs = normalizeFeatureVectors(all_orig_feature_vecs);
    for (int index = 0; index < cut_off_index; index++) {
            
        //------------------------ extracting LIBLINEAR features ------------------------
        LiblinearFeatures liblinear_features;        
        liblinear_features.inferAllDependencyPairs(std::get<2>(cost_ordering_dependency[index])); // update dependency graph in-place

        //--------------------- cache the top 5 dependency graph ---------------------
        vector<set<int>>& temp_graph = std::get<2>(cost_ordering_dependency[index]);
        for (int i = 0; i < (int)temp_graph.size(); i++) {
            if (temp_graph[i].empty())
                continue;
            for (auto j : temp_graph[i]) { //j has higher priority than i, aka j->i
                dep_pair_count[make_pair(j, i)] += 1; 
            }
        }
    }

    //--------------------- print conflicting and common pairs ---------------------
    // string dependency_stats_fname = "../" + std::to_string(num_agent)+ "_dependency_stats.txt";
    //remove(dependency_stats_fname.c_str());
    // std::ofstream stats_ofs(dependency_stats_fname, std::ios_base::app); //concatenate
    vector<set<pair<int, int>>> deppair_by_occurance(cut_off_index);
    //stats_ofs << "\npairs with conflicts:\n";
    int conflict_pair_cnt = 0;
        
    for (auto pair_occurrence : dep_pair_count) {
        if (pair_occurrence.second == 0)
            continue;
        std::pair<int, int> this_pair = pair_occurrence.first;
        std::pair<int, int> conflict_pair = make_pair(this_pair.second, this_pair.first);
        if (dep_pair_count.find(conflict_pair) != dep_pair_count.end()) { //found conflict pair   
            conflict_pair_cnt++;

            //beware of these two lines
            dep_pair_count[this_pair] = 0;
            dep_pair_count[conflict_pair] = 0;
        }
        else {
            deppair_by_occurance[pair_occurrence.second - 1].insert(this_pair); //store non-conflicting pairs
        }
    }
    //stats_ofs << conflict_pair_cnt << " pairs.\n";
    if (is_validation) {
        // stats_ofs << "\n#pairs to be trained, scen " << scen << ":\n";
        // for (int i = 0; i < deppair_by_occurance.size(); i++) {
        //     //stats_ofs << deppair_by_occurance[i].size() << " pairs occured " << i + 1 << " times \n";           
        //     for (auto pair : deppair_by_occurance[i]) {
        //         stats_ofs << "<" << pair.first << "," << pair.second << "> ";
        //     }
        //     stats_ofs << "\n";            
        // }
    }

    // manually construct total dependency graph and make sure it's a DAG (there's no cycles)
    for (int i = deppair_by_occurance.size() - 1; i >= 0; i--) { //from high occ to low occ
        for (auto pair : deppair_by_occurance[i]) {
            int a = pair.first;
            int b = pair.second;
            //want to add a->b, first check if there is already a path from b->a
            vector<int> traversed = BFS(b, num_agent, total_dependency_matrix);
            auto find_a = std::find(traversed.begin(), traversed.end(), a);
            if(find_a == traversed.end()){
                total_dependency_matrix[a][b] = i+1; //add edge a->b, value is the occurrence
            }
            
            //total_dependency_matrix[i][j] represents i->j (i must be planned before j)
        }
    }

    // construct inferred, cleaned-up dependency graph (all pairs with occurrences) from total_dependency_matrix
    vector<set<pair<int, int>>> manual_deppair_by_occ(cut_off_index);
    for (int i = 0; i < total_dependency_matrix.size(); i++) {
        for (int j = 0; j < total_dependency_matrix[i].size(); j++) {
            int occ = total_dependency_matrix[i][j];
            if(occ != MAX_COST)
                manual_deppair_by_occ[occ - 1].insert(make_pair(i,j));
        }
    }

    //------ extract feature vecs from common top5 pairs. May keep some conflicted pairs with high occurrence ------

    // take non-conflicting pairs. use weighted weight file
    LiblinearFeatures liblinear_features;
    liblinear_features.printFeaturesByDepPairOccurrence(all_norm_feature_vecs, manual_deppair_by_occ, ofs);//deppair_by_occurance
    liblinear_features.printWeightsByDepPairOccurrence(manual_deppair_by_occ, weights_ofs);//deppair_by_occurance
    ofs.close();
    // stats_ofs.close();
}

void solvePPSVMRank(std::ofstream& ofs, Instance& instance, int pp_screen, vector<int>& pp_runs,
    int& scen, bool is_test, vector<double>& all_ml_overhead, vector<vector<int> >& SVM_Trained_orderings,
    vector<vector<double> >& all_costs, vector< vector < vector <double> > >& rrfirst_byproduct,
    int qid, int curr_learning_numagent,
    bool random_restart, bool fixedtime_restart,
    boost::optional<LiblinearAPI&> liblinear_api,
    boost::optional<SVMRankAPI&> svm_rank_api,
    boost::optional<vector<vector<double>>&> all_runtime, 
    boost::optional<vector<vector<int>>&> all_failed_agent_idx,
    boost::optional<vector<vector<int>>&> all_restart_count,
    //boost::optional<vector<vector<vector<double>>>&> rrfirst_byproduct,
    double restart_time_constraint)
{
    PP pp(instance, pp_screen);
    pp.preprocess(true, true, true); 
    int num_agent = instance.num_of_agents;
    if (curr_learning_numagent != -1) { //set up the SVMRank model to curr-learn on 
        vector<int> dummy_ordering(num_agent);
        std::iota(dummy_ordering.begin(), dummy_ordering.end(), 0);
        vector<vector<double>> all_orig_feature_vecs = getOriginalFeatureVectors(instance, pp, dummy_ordering);
        vector<vector<double>> all_norm_feature_vecs = normalizeFeatureVectors(all_orig_feature_vecs);
        svm_rank_api->setTestFeatures(all_norm_feature_vecs, dummy_ordering);
        svm_rank_api->weightsDotProdFeatures(num_agent);
    }

    double total_runtime = 0;
    int best_cost = MAX_COST;
    double best_runtime;
    vector<int> best_priority_ordering;
    vector<set<int>> best_dependency_graph;
    for (auto i: pp_runs)
    {
        int sum_of_costs = MAX_COST;
        total_runtime = 0;
        int failed_agent_id = -1;
        int restart_counter = 0;
        double ML_total_overheadsec = 0;

        //these are for rr-best comparison analysis 
        int best_costs_self = MAX_COST; //best cost of the same method
        int first_costs_self = MAX_COST; //first solution cost of the same method
        double best_runtime_self = MAX_COST;//best runtime of the same method

        if (!random_restart) {
            // curr_learn to generate train data
            if (!is_test && curr_learning_numagent != -1) {
                computePPOrderingCurrLearning(i, instance, pp, scen, false, num_agent, boost::none, svm_rank_api);
                sum_of_costs = pp.run();
                total_runtime += pp.runtime;
                if (i <= 10) {
                    while (abs(sum_of_costs - MAX_COST) < 0.0001 && total_runtime <= 3) { //time constraint
                        pp.reset();
                        computePPOrderingCurrLearning(i, instance, pp, scen, false, num_agent, boost::none, svm_rank_api);
                        best_costs_self = pp.run();
                        total_runtime += pp.runtime;
                    }
                }
            }
            // deterministic testing
            else {
                computePPOrdering(i, pp, scen, is_test, SVM_Trained_orderings);
                best_costs_self = pp.run();
                total_runtime = pp.runtime;
                // all_runtime.get()[i].push_back(total_runtime);
            }
        }
        else {
            // only want rr-first, not rr-best
            if (!fixedtime_restart) {
                while (abs(best_costs_self - MAX_COST) < 0.0001 && total_runtime <= restart_time_constraint) { //time constraint
                    // cout<<"in while loop"<<endl;
                    if (i == 2) {
                        if (liblinear_api != boost::none) {
                            //cout << "liblinear random restart" << endl;
                            liblinear_api->randomRestart(scen, true); //false==bucket, true==softmax
                            SVM_Trained_orderings[scen] = liblinear_api->orderings[scen];
                        }
                        else {
                            //cout << "svmrank random restart" << endl;
                            svm_rank_api->randomRestart(scen, true); //false==bucket, true==softmax
                            SVM_Trained_orderings[scen] = svm_rank_api->orderings[scen];
                        }
                    }
                    computePPOrdering(i, pp, scen, is_test, SVM_Trained_orderings, random_restart);
                    pp.reset();
                    failed_agent_id = -1;
                    // cout<<"time left:"<<time_left<<endl;
                    best_costs_self = pp.run(failed_agent_id, restart_time_constraint - total_runtime);
                    store_failed_agent_idx(i, failed_agent_id, pp, all_failed_agent_idx);
                    total_runtime += pp.runtime;
                    restart_counter++;
                }
            }
            // want rr-best
            // TODO: extract rr-first from this process
            else {
                bool found_sol = false;
                while (total_runtime <= restart_time_constraint) {
                    if (i == 2) {
                        clock_t start_overheadtime = clock();
                        if (liblinear_api != boost::none) {
                            liblinear_api->randomRestart(scen, true); //false==bucket, true==softmax
                            SVM_Trained_orderings[scen] = liblinear_api->orderings[scen];
                        }
                        else {
                            svm_rank_api->randomRestart(scen, true); //false==bucket, true==softmax
                            SVM_Trained_orderings[scen] = svm_rank_api->orderings[scen];
                        }
                        ML_total_overheadsec += (double)(clock() - start_overheadtime) / CLOCKS_PER_SEC;
                    }
                    computePPOrdering(i, pp, scen, is_test, SVM_Trained_orderings, random_restart);
                    pp.reset();
                    failed_agent_id = -1;
                    sum_of_costs = pp.run(failed_agent_id, restart_time_constraint - total_runtime);
                    store_failed_agent_idx(i, failed_agent_id, pp, all_failed_agent_idx);
                    store_failed_agent_idx(i, failed_agent_id, pp, rrfirst_byproduct[2]);
                    total_runtime += pp.runtime;
                    restart_counter++;
                    if (sum_of_costs < best_costs_self) {
                        best_costs_self = sum_of_costs;
                        best_runtime_self = pp.runtime;
                    }
                    if (!found_sol && sum_of_costs != MAX_COST) { //first sol
                        found_sol = true;
                        first_costs_self = sum_of_costs;
                        cout << "first solution cost = " << first_costs_self << endl;
                        rrfirst_byproduct[1][i].push_back(total_runtime);
                        rrfirst_byproduct[3][i].push_back(restart_counter);
                    }
                }
                if (i == 2) {
                    cout << "ML total overhead = " << ML_total_overheadsec << " sec" << endl;
                    all_ml_overhead.push_back(ML_total_overheadsec/ (ML_total_overheadsec + total_runtime));
                }
                if(!found_sol){ //no sol within runtime limit
                    rrfirst_byproduct[1][i].push_back(total_runtime);
                    rrfirst_byproduct[3][i].push_back(restart_counter);
                }
            }
            all_runtime.get()[i].push_back(total_runtime);
            all_restart_count.get()[i].push_back(restart_counter);
        }

        // pp.printOrdering();
        // pp.printDependencyGraph();
        if (is_test) {
            //normalize costs and store them into all_costs
            //did not normalize MAX_COST(no solution)
            int sum_of_start_goal_dist = computeSumStartGoalDist(instance);
            if (best_costs_self < MAX_COST) {
                //all_costs[i][scen] = sum_of_costs / (double)sum_of_start_goal_dist;
                all_costs[i].push_back(best_costs_self / (double)sum_of_start_goal_dist);
            }
            else {
                //all_costs[i][scen] = sum_of_costs;
                all_costs[i].push_back(best_costs_self);
            }
            if (first_costs_self < MAX_COST && fixedtime_restart) {
                rrfirst_byproduct[0][i].push_back(first_costs_self / (double)sum_of_start_goal_dist);
            }
            else if (first_costs_self == MAX_COST && fixedtime_restart) {
                rrfirst_byproduct[0][i].push_back(first_costs_self);
            }
        }
        if (!random_restart && (curr_learning_numagent == -1 && !is_test)) {
            cout << "Iteration " << i << "\t\t runtime = " << pp.runtime << " seconds.";
            cout << " sum of costs = " << best_costs_self;
        }
        else {
            cout << "Iteration " << i << "\t\t total runtime = " << total_runtime << " seconds.";
            cout << " sum of costs = " << best_costs_self;
        }
        if (best_costs_self < best_cost)
        {
            cout << "\t\t -- Find a better solution with sum of costs = " << best_costs_self;
            best_cost = best_costs_self;
            best_runtime = best_runtime_self;
            best_priority_ordering = pp.ordering; //this is buggy if using fixed time constraint
            best_dependency_graph = pp.dependency_graph; //this is buggy if using fixed time constraint
        }
        cout << endl;
        pp.reset();
    }
    if (best_cost < MAX_COST)
    {
        //std::ofstream ofs(out_fname, std::ios_base::app); //concatenate
        cout << "#number of agents: " << best_priority_ordering.size() << endl;
        cout << "#priority ordering: ";
        for (int i : best_priority_ordering)
            cout << i << " ";
        cout << endl;
        //printDependencyPairs(best_dependency_graph);
        cout << "#Sum of costs = " << best_cost << endl;
        cout << "#runtime = " << best_runtime << " seconds." << endl;

        if (!is_test) {
            genSVMRankRawFeatureFile(instance, pp, ofs,
                best_priority_ordering, best_cost, best_runtime, qid);
        }
    }
    else if (best_cost == MAX_COST && !is_test) {
        /*cout << "scen " << scen << " no solution, running again" << endl;
        scen -= 1;*/
        //comment out this when generating custom agent file
    }
}

//invoked by driver_test
void solvePP(Instance& instance, int pp_screen, vector<int>& pp_runs,
    int scen, bool is_test, vector<double>& all_ml_overhead,
    vector<vector<int> >& SVM_Trained_orderings, vector<vector<double> >& all_costs,
    vector< vector < vector <double> > > &rrfirst_byproduct, int qid,
    bool random_restart, bool fixedtime_restart,
    boost::optional<LiblinearAPI&> liblinear_api, boost::optional<SVMRankAPI&> svm_rank_api,
    boost::optional<vector<vector<double>>&> all_runtime,
    boost::optional<vector<vector<int>>&> all_failed_agent_idx, boost::optional<vector<vector<int>>&> all_restart_count, 
    //boost::optional<vector<vector<vector<double>>>&> rrfirst_byproduct,
    double restart_time_constraint) {
    std::ofstream ofs("../dummy_ofs_empty.txt", std::ios_base::app); //dummy file if testing
    return solvePPSVMRank(ofs, instance, pp_screen, pp_runs, scen, is_test, all_ml_overhead, SVM_Trained_orderings,
        all_costs, rrfirst_byproduct, qid, -1, random_restart, fixedtime_restart, liblinear_api, svm_rank_api, all_runtime,
        all_failed_agent_idx, all_restart_count, 
        restart_time_constraint);
}

void solvePP(std::ofstream& ofs, Instance& instance, int pp_screen,
    int pp_runs, int& scen, bool is_test, int qid, int curr_learning_numagent,
    boost::optional<SVMRankAPI&> svm_rank_api) {  //invoked by driver_train for SVMRank
    vector<vector<int> > SVM_Trained_orderings;
    vector<vector<double> > all_costs;
    vector<int> pp_runs_vec(pp_runs);
    std::iota (pp_runs_vec.begin(),pp_runs_vec.end(),0);
    vector<vector<vector<double>>> rrfirst_byproduct;
    vector<double> dummy_ml_overhead;
    return solvePPSVMRank(ofs, instance, pp_screen, pp_runs_vec, scen, is_test, dummy_ml_overhead, SVM_Trained_orderings,
        all_costs, rrfirst_byproduct, qid, curr_learning_numagent, false,false, boost::none, svm_rank_api, boost::none,
        boost::none, boost::none,
        5.0);
}

void solvePP(std::ofstream& ofs, std::ofstream& weights_ofs, Instance& instance, int pp_screen,
    int pp_runs, int& scen, LiblinearAPI& liblinear_api, int qid, bool is_validation, int curr_learning_numagent) {
    vector<vector<int> > SVM_Trained_orderings;
    vector<vector<double> > all_costs;
    return solvePPLiblinear(ofs, weights_ofs, instance, pp_screen, pp_runs, scen,
        SVM_Trained_orderings, all_costs, liblinear_api, qid,  is_validation, curr_learning_numagent);
}

void genSVMRankRawFeatureFile(Instance& instance, PP& pp, ofstream& ofs, 
    vector<int>& best_priority_ordering, int best_cost, double best_runtime, int qid) {
    //TODO: refactor this code and getOriginalFeatureVectors() and modify class MLFeatures
    MLFeatures ml_features;
    MLFeaturesInstance ml_features_instance(instance, pp);

    //print 4 lines starting with '#'
    ml_features.printSolutionHeader(ofs, best_priority_ordering, best_cost, best_runtime);

    ml_features_instance.hashMDDnodeOccurances();
    for (int i = 0; i < best_priority_ordering.size(); i++) {
        MLFeaturesAgent ml_features_agent(instance, best_priority_ordering[i], pp, ml_features_instance);
        vector<double> feature_vec;
        ml_features.generateFeatureVec(ml_features_instance, ml_features_agent, feature_vec);
        ml_features.printFormattedFeatures(i, qid, ofs, feature_vec);
    }
    ofs.close();
}

void genTestFileNoLabel(std::ofstream& ofs, Instance& instance, int pp_screen, int qid, double& runtime) {
    PP pp(instance, pp_screen);
    pp.preprocess(true, true, true);
    int num_agent = instance.num_of_agents;
    vector<int> dummy_ordering(num_agent);
    std::iota(dummy_ordering.begin(), dummy_ordering.end(), 0);
    clock_t start_gen_testfile = clock();
    genSVMRankRawFeatureFile(instance, pp, ofs, dummy_ordering, -1, -1, qid); //print invalid runtime, cost
    runtime += (double)(clock() - start_gen_testfile); 
}

void comparisonAnalysis(std::ofstream& ofs, vector<vector<double> >& all_costs, 
    vector<vector<int> >& all_failed_agent_idx,
    boost::optional<vector<vector<double>>&> all_runtime,
    boost::optional<vector<vector<int>>&> all_restart_count)
{
    //------------analyze success rate and average cost------------
    vector<double> success_rate;
    vector<double> avg_costs;
    vector<double> avg_runtime;
    // print out all_costs
    ofs << "all cost size: " << all_costs.size() << endl;
    ofs << "all cost[0] size: " << all_costs[0].size() << endl;
    print2DVector(ofs, all_costs, "all_costs");
    print2DVector(ofs, all_runtime.get(), "all_runtime:");

    analyzeSucRateAndCost(all_costs, success_rate, avg_costs);

    print1DVector(ofs, avg_costs, "normalized average costs:\n", true); //print out avg_costs
    print1DVector(ofs, success_rate, "success rate:\n", true); //print out success_rate

    bool isMatrix = true;
    for(int i=0; i<all_costs.size()-1; i++){
        if(all_costs[i].size() !=all_costs[i+1].size()){
            isMatrix = false;
            break;
        }
    }
    if(isMatrix){
        //------------------------analyze ranking------------------------
        vector<double> all_average_rank;
        analyzeCostRank(ofs, all_costs, all_average_rank);

        // print all_average_rank
        print1DVector(ofs, all_average_rank, "all_average_ranks:\n", true);
    }

    //------------------------average runtime------------------------
    vector<double> all_average_runtime;
    averageEachRow(all_average_runtime, all_runtime.get());
    print1DVector(ofs, all_average_runtime, "all_average_runtime:\n", true);
    

    //-------------------failed agent, restart count-------------------
    vector<double> all_average_failed_agent_idx;
    averageEachRow(all_average_failed_agent_idx, all_failed_agent_idx);
    print1DVector(ofs, all_average_failed_agent_idx, "all_average_failed_agent_idx\n", true);

    vector<double> all_average_restart_count;
    averageEachRow(all_average_restart_count, all_restart_count.get());
    print1DVector(ofs, all_average_restart_count, "all_average_restart_count:\n", true);
}

void analyzeSucRateAndCost(vector<vector<double> >& all_costs, vector<double>& success_rate, vector<double>& avg_costs) {
    double random_cost_sum = 0;
    int random_no_solution_count = 0;
    for (int i = 0; i < all_costs.size(); i++) {
        if (i <= 2) { //LH, SH, SVM
            int no_solution_count = 0;
            double curr_sum = 0;
            for (int j = 0; j < all_costs[i].size(); j++) {
                if (abs(all_costs[i][j] - MAX_COST) < 0.000001) {
                    no_solution_count++;
                }
                else {
                    curr_sum += all_costs[i][j];
                }
            }
            cout << "i=" << i << " no solution count=" << no_solution_count << endl;
            cout << "i=" << i << "solved count=" << (all_costs[i].size() - no_solution_count) << endl;
            success_rate.push_back(((double)(all_costs[i].size() - no_solution_count)) / all_costs[i].size());
            double avg_cost = ((double)curr_sum) / (all_costs[i].size() - no_solution_count);
            avg_costs.push_back(avg_cost);
        }
        else { //5 Random
            for (int j = 0; j < all_costs[i].size(); j++) {
                if (abs(all_costs[i][j] - MAX_COST) < 0.000001) {
                    random_no_solution_count++;
                }
                else {
                    random_cost_sum += all_costs[i][j];
                }
            }
        }
    }
    int random_total_runs = all_costs[0].size() * (all_costs.size() - 3);
    cout << "random: total runs=" << random_total_runs << endl;
    cout << "random: no solution count=" << random_no_solution_count << endl;
    success_rate.push_back((double)(random_total_runs - random_no_solution_count) / random_total_runs);
    avg_costs.push_back(((double)random_cost_sum) / (random_total_runs - random_no_solution_count));
}

void analyzeCostRank(std::ofstream& ofs, vector<vector<double> >& all_costs, vector<double>& all_average_rank) {
    vector<vector<double> > all_ranks;
    for (int col = 0; col < all_costs[0].size(); col++) {
        vector<double> partial_costs;
        vector<vector<int> > partial_ranks;
        for (int row = 0; row <= 2; row++) {
            partial_costs.push_back(all_costs[row][col]);
        }
        for (int row = 3; row < all_costs.size(); row++) {
            partial_costs.push_back(all_costs[row][col]);
            vector<int> cost_rank;
            rankCost(partial_costs, cost_rank);
            partial_ranks.push_back(cost_rank);
            //debug:print partial_costs and cost_rank            
            /*print1DVector(ofs,partial_costs, "partial_costs");
            print1DVector(ofs,cost_rank, "cost_rank");*/

            partial_costs.pop_back();
        }
        //debug:print partial_ranks

        //compute average rank and push into total_avg_ranks
        vector<double> average_rank;
        averageEachCol(average_rank, partial_ranks);
        all_ranks.push_back(average_rank);
    }
    //debug:print all_ranks

    //average over all ranks
    averageEachCol(all_average_rank, all_ranks);
}

template <typename U>
void averageEachCol(vector<double>& averages, const vector<vector<U> >& original) {
    if (original.size() == 0 || averages.size() != 0) { //assume averages is an empty array
        return;
    }
    //compute average rank and push back into total_avg_ranks
    for (int j = 0; j < original[0].size(); j++) {
        U curr_sum = 0;
        for (int i = 0; i < original.size(); i++) {
            curr_sum += original[i][j];
        }
        averages.push_back(curr_sum / (double)original.size());
    }
}

template <typename V>
void averageEachRow(vector<double>& averages, const vector<vector<V> >& original) {
    if (original.size() == 0 || averages.size() != 0) { //assume averages is an empty array
        return;
    }
    for (int i = 0; i < original.size(); i++) {
        double sum = 0;
        int num_scens = original[i].size();
        for (int j = 0; j < num_scens; j++) {
            sum += original[i][j];
        }
        averages.push_back((double)sum / num_scens);
    }
}

int computeSumStartGoalDist(Instance& instance) {
    int sum_of_dist = 0;
    for (int i = 0; i < instance.num_of_agents; i++) {
        int start_loc = instance.start_locations[i];
        int goal_loc = instance.goal_locations[i];
        int dist = instance.getManhattanDistance(start_loc, goal_loc);
        sum_of_dist += dist;
    }
    return sum_of_dist;
}

void rankCost(vector<double>& partial_costs, vector<int>& cost_rank) {
    vector<int> temp = sort_indexes<double>(partial_costs);
    vector<int> custom_rank(partial_costs.size());
    std::iota(custom_rank.begin(), custom_rank.end(), 0);
    for (int i = 1; i < temp.size(); i++) {
        if (abs(partial_costs[temp[i]] - partial_costs[temp[i - 1]]) < 0.000001) {
            custom_rank[i] = custom_rank[i - 1];
        }
    }
    vector<tuple<int, int> > index_rank;
    for (int i = 0; i < custom_rank.size(); i++) {
        index_rank.push_back(make_tuple(temp[i], custom_rank[i]));
    }
    std::sort(index_rank.begin(), index_rank.end());
    for (int i = 0; i < index_rank.size(); i++) {
        cost_rank.push_back(get<1>(index_rank[i]));
    }
}

//-------------------------for debug------------------------------------
template <typename T>
void print1DVector(std::ostream& ofs, const vector<T>& vec, string message, bool pythonStyle)
{
    ofs << message;
    if (pythonStyle) ofs << "[";
    for (int i = 0; i < vec.size(); i++) {
        ofs << vec[i] << ((pythonStyle && (i != vec.size()-1)) ? ", " : " ");
    }
    if (pythonStyle) ofs << "],";
    ofs << endl;
}

template <typename T>
void print2DVector(std::ofstream& ofs, const vector<vector<T> >& vec, string message)
{
    ofs << message << endl;
    ofs << "[" << endl;
    for (int i = 0; i < vec.size(); i++) {
        ofs << "[";
        for (int j = 0; j < vec[i].size(); j++) {
            if (abs(vec[i][j] - MAX_COST) < 0.000001) {
                ofs << "'NaN'" << ", ";
            }
            else {
                ofs << vec[i][j] << ", ";
            }
        }
        ofs << "]," << endl;
    }
    ofs << "]" << endl;
}

void printDependencyPairs(vector<set<int>>& dependency_graph) {
    for (int i = 0; i < (int)dependency_graph.size(); i++)
    {
        if (dependency_graph[i].empty())
            continue;
        for (auto j : dependency_graph[i])
            cout << j << "->" << i << ", ";
        cout << endl;
    }
}

void floydWarshallAlgorithm(vector<vector<int>>& dependency_matrix, int num_agents) {
    //run Floyd-Warshall algorithm
    for (int k = 0; k < num_agents; k++) {
        for (int i = 0; i < num_agents; i++) {
            for (int j = 0; j < num_agents; j++) {
                if (dependency_matrix[i][j] > (dependency_matrix[i][k] + dependency_matrix[k][j])
                    && (dependency_matrix[i][k] != MAX_COST
                        && dependency_matrix[k][j] != MAX_COST)) {
                    dependency_matrix[i][j] = dependency_matrix[i][k] + dependency_matrix[k][j];
                }
            }
        }
    }
}

// A recursive function used by topologicalSort
void topologicalSortUtil(vector<vector<int>> const& graph, int v, vector<bool>& visited,
    std::stack<int>& Stack)
{
    // Mark the current node as visited.
    visited[v] = true;

    // Recur for all the vertices
    // adjacent to this vertex
    for (int i = 0; i < graph[v].size(); i++) {
        if (graph[v][i] == MAX_COST) {
            continue;
        }
        if (!visited[i]) {
            topologicalSortUtil(graph, i, visited, Stack);
        }
    }

    // Push current vertex to stack
    // which stores result
    Stack.push(v);
}

// The function to do Topological Sort.
// It uses recursive topologicalSortUtil()
vector<int> topologicalSort(vector<vector<int>> const& graph, int V)
{
    stack<int> Stack;

    // Mark all the vertices as not visited
    vector<bool> visited(V, false);

    // Call the recursive helper function to store Topological Sort 
    // starting from all vertices one by one
    for (int i = 0; i < V; i++)
        if (visited[i] == false)
            topologicalSortUtil(graph, i, visited, Stack);

    // Print contents of stack
    vector<int> topological_ordering;
    while (Stack.empty() == false) {
        //cout << Stack.top() << " ";
        topological_ordering.push_back(Stack.top());
        Stack.pop();
    }
    return topological_ordering;
}


vector<int> BFS(int s, int V, vector<vector<int>> const& graph) //traverse from a given . V: total # vertex, aka number of agents
{
    vector<int> ret;//return all traversed vertex

    // Mark all the vertices as not visited
    vector<bool> visited = vector<bool>(V, false);
    
    // Create a queue for BFS
    list<int> queue;

    // Mark the current node as visited and enqueue it
    visited[s] = true;
    queue.push_back(s);

    while (!queue.empty())
    {
        // Dequeue a vertex from queue and print it
        s = queue.front();
        //cout << s << " ";
        ret.push_back(s);
        queue.pop_front();

        // Get all adjacent vertices of the dequeued
        // vertex s. If a adjacent has not been visited,
        // then mark it visited and enqueue it
        for (int i = 0; i < graph[s].size(); i++) {
            if (graph[s][i] == MAX_COST)
                continue;
            if (!visited[i]) {
                visited[i] = true;
                queue.push_back(i);
            }
        }
    }
    return ret;
}