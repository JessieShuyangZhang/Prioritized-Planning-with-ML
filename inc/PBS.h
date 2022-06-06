//#pragma once
//#include "common.h"
//#include "MDD.h"
//#include "PathTable.h"
//#include "SpaceTimeAStar.h"
//
//struct PBSNode
//{
//    int time_expanded = -1;
//    int time_generated = -1;
//
//    PBSNode* parent = nullptr;
//    pair<int, int> priority; // the former has lower priority than the latter task
//    list<pair<int, Path>> new_paths;
//    list<Conflict> conflicts;
//    Conflict chosen_conflict;
//};
//
//
//class PBS
//{
//public:
//    uint64_t num_expanded = 0;
//    uint64_t num_generated = 0;
//    vector<Path*> paths;
//    vector<vector<bool>> priorities; // pairwise priorities among agents
//
//    explicit PBS(Instance& instance, int screen): instance(instance), screen(screen) {}
//    ~PBS()
//    {
//        for (auto node : all_nodes)
//            delete node;
//        all_nodes.clear();
//        open_list = std::stack<PBSNode*>();
//    }
//    bool run(double time_limit);
//    void topologicalSort(list<Task*>& ordered_tasks);
//private:
//    Instance& instance;
//    int screen;
//
//    PBSNode* root_node = nullptr;
//    PBSNode* goal_node = nullptr;
//    std::stack<PBSNode*> open_list;
//    list<PBSNode*> all_nodes;
//
//    void topologicalSortUtil(Task* v, set<Task*> & visited, list<Task*> & stack);
//    void update(PBSNode& node);
//    void generateRoot();
//    bool addConstraint(PBSNode& node, Task& low, Task& high);
//    void generateSolution();
//    void getHigherPriorityAgents(int agent, set<int>& agents);
//    bool planPath(int agent, PBSNode& node);
//    bool findConflicts(PBSNode& node, int agent);
//    static bool findConflicts(PBSNode& node, int a1, int a2);
//};