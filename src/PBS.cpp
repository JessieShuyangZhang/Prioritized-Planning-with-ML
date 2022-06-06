//#include "PBS.h"
//
//bool PBS::run(double time_limit)
//{
//    if (screen >= 2)
//        cout << "Generate Root node " << endl;
//    generateRoot();
//
//    while (!open_list.empty())
//    {
//        auto curr = open_list.top();
//        open_list.pop();
//        if (screen >= 2)
//            cout << "Select Node " << curr->time_generated << endl;
//        update(*curr);
//        auto rst = goalTest(*curr);
//        if (curr.conflicts.empty())
//        {
//            return true;
//        }
//        if (screen >= 2)
//            cout << "Expand Node " << curr->time_generated << endl;
//        curr->time_expanded = num_expanded;
//        num_expanded++;
//        curr->chosen_conflict = curr->conflicts.front();
//        PBSNode* child[2] = {new PBSNode(), new PBSNode()};
//        child[0]->parent = curr;
//        child[1]->parent = curr;
//        for (auto& conflict : curr->conflicts)
//        {
//            child[0]->conflicts.push_back(conflict);
//            child[1]->conflicts.push_back(conflict);
//        }
//        if(addConstraint(*child[0], *curr->chosen_conflict.first, *curr->chosen_conflict.second))
//        {
//            child[0]->time_generated = num_generated;
//            num_generated++;
//            if (screen >= 2)
//                cout << "Generate Node " << child[0]->time_generated << endl;
//            all_nodes.push_back(child[0]);
//        }
//        else
//        {
//            delete child[0];
//            child[0] = nullptr;
//        }
//        update(*curr);
//        if(addConstraint(*child[1], *curr->chosen_conflict.second, *curr->chosen_conflict.first))
//        {
//            child[1]->time_generated = num_generated;
//            num_generated++;
//            if (screen >= 2)
//                cout << "Generate Node " << child[1]->time_generated << endl;
//            all_nodes.push_back(child[1]);
//        }
//        else
//        {
//            delete child[1];
//            child[1] = nullptr;
//        }
//
//        if (child[0] != nullptr && child[1] != nullptr)
//        {
//            // compare the cost and put the one with more conflicts to the open list first
//            if (child[0]->conflicts.size() < child[1]->conflicts.size())
//            {
//                open_list.push(child[1]);
//                open_list.push(child[0]);
//            }
//            else
//            {
//                open_list.push(child[0]);
//                open_list.push(child[1]);
//            }
//        }
//        else if (child[0] != nullptr)
//            open_list.push(child[0]);
//        else if (child[1] != nullptr)
//            open_list.push(child[1]);
//    }
//
//    return false;
//}
//
//PBS_node_type PBS::goalTest(PBSNode& node)
//{
//    if (!node.conflicts.empty())
//        return PBS_node_type::NONGOALNODE;
//
//    for (auto & task : ordered_tasks)
//    {
//        if (paths[instance.getTaskID(*task)] == nullptr)
//        {
//            if(!planPath(*task, node))
//                return PBS_node_type::INFEASIBLENODE;
//            if(findConflicts(node, *task))
//                return PBS_node_type::NONGOALNODE;
//        }
//    }
//    return PBS_node_type::GOALNODE;
//}
//
//void PBS::generateRoot()
//{
//    topologicalSort(ordered_tasks);
//    tasks.resize(instance.getNumOfTasks());
//    for (auto & task : ordered_tasks)
//    {
//        tasks[instance.getTaskID(*task)] = task;
//    }
//    auto root = new PBSNode();
//    all_nodes.push_back(root);
//    root->time_generated = 0;
//    num_generated++;
//    open_list.push(root);
//}
//
//void PBS::generateSolution()
//{
//    solution.clear();
//    solution.resize(instance.agents.size());
//    for (auto & agent : instance.agents)
//    {
//        for (auto & task : agent.tasks)
//        {
//            auto path = paths[instance.getTaskID(*task)];
//            solution[agent.idx].insert(solution[agent.idx].end(), path->begin(), path->end());
//        }
//    }
//}
//
//bool PBS::planPath(Task& task, PBSNode& node)
//{
//    auto & agent = instance.agents[task.agent_idx];
//    ReservationTable rt(agent);
//    set<Task*> higher_tasks;
//    getHigherPriorityTasks(task, higher_tasks);
//    for (auto t : higher_tasks)
//    {
//        rt.addPath(*paths[instance.getTaskID(*t)], instance.agents[t->agent_idx]);
//    }
//
//    State initial_state;
//    if (task.prev == nullptr)
//    {
//        initial_state.vertex = agent.start_vertex;
//        initial_state.duration = make_pair(0, MAX_COST);
//    }
//    else
//    {
//        auto task_id = instance.getTaskID(*task.prev);
//        initial_state.vertex = paths[task_id]->back().vertex;
//        initial_state.duration = make_pair(paths[task_id]->back().duration.second, MAX_COST);
//    }
//    initial_state.task = &task;
//    if (task.type)
//        initial_state.operation = operation_t::MOVE;
//    else
//        initial_state.operation = operation_t::REACH;
//
//    double earliest_completion_time = -1;
//    for (const auto & t : task.preconditions)
//    {
//        earliest_completion_time = max(paths[instance.getTaskID(*t)]->back().duration.second, earliest_completion_time);
//    }
//
//    SIPP sipp(agent);
//    sipp.run(initial_state, task, rt, earliest_completion_time);
//    if (sipp.path.empty())
//    {
//        cout << "Fail to find path for agent " << agent.name << " with object "
//             << instance.objects[task.object_idx].name << endl;
//        return false;
//    }
//    cout << agent.name << " path:\t";
//    instance.printPath(sipp.path, agent);
//    node.new_paths.emplace_back(&task, sipp.path);
//    paths[instance.getTaskID(task)] = &node.new_paths.back().second;
//    return true;
//}
//
//void PBS::topologicalSort(list<Task*>& stack)
//{
//    stack.clear();
//    set<Task*> visited;
//
//    // Call the recursive helper function to store Topological
//    // Sort starting from all vertices one by one
//    std::queue<pair<list<Task*>::iterator, list<Task*>::iterator>> pts;
//    for (auto & agent : instance.agents)
//    {
//        assert(!agent.tasks.empty());
//        pts.emplace(agent.tasks.begin(), agent.tasks.end());
//    }
//    while(!pts.empty())
//    {
//        auto p = pts.front();
//        pts.pop();
//        if (visited.find(*p.first) == visited.end())
//            topologicalSortUtil(*p.first, visited, stack);
//        ++p.first;
//        if (p.first != p.second)
//            pts.push(p);
//    }
//    stack.reverse();
//    // Print
//    //cout << "Topological Sort: " << endl;
//    //for (auto & task : stack) {
//    //    cout << "\tTask " <<instance.getTaskID(*task) << ": Agent " << task->agent_idx << ", Object " << task->object_idx << ", Task type " << task->type << endl;
//    //}
//}
//
//void PBS::topologicalSortUtil(Task* v, set<Task*> & visited, list<Task*> & stack)
//{
//    // Mark the current node as visited.
//    visited.insert(v);
//
//    // Recur for all the vertices adjacent to this vertex
//    for (auto & i : v->preconditions)
//    {
//        if (visited.find(i) == visited.end())
//            topologicalSortUtil(i, visited, stack);
//    }
//    if (v->prev != nullptr and visited.find(v->prev) == visited.end())
//        topologicalSortUtil(v->prev, visited, stack);
//    if (!priorities.empty())
//    {
//        for (auto & i : priorities[instance.getTaskID(*v)])
//        {
//            if (visited.find(i) == visited.end())
//                topologicalSortUtil(i, visited, stack);
//        }
//    }
//    // Push current vertex to stack which stores result
//    stack.push_front(v);
//}
//
//void PBS::getHigherPriorityTasks(Task & task, set<Task*>& higher_tasks)
//{
//    for (auto t : priorities[instance.getTaskID(task)])
//    {
//        auto ret = higher_tasks.insert(t);
//        if (ret.second) // insert successfully
//        {
//            getHigherPriorityTasks(*t, higher_tasks);
//        }
//    }
//}
//
//bool PBS::findConflicts(PBSNode& node, Task& task)
//{
//    bool found = false;
//    auto path = paths[instance.getTaskID((task))];
//    for (auto& p : paths)
//    {
//        if (p == nullptr or //empty path
//            min(p->back().duration.second, path->back().duration.second) -
//            max(p->front().duration.first, path->front().duration.first) < EPSILON) // No overlapped time interval
//            continue;
//        if (findConflicts(node, *path, *p, instance.agents[task.agent_idx], instance.agents[p->front().task->agent_idx]))
//            found = true;
//    }
//    return found;
//}
//
//bool PBS::findConflicts(PBSNode& node, Path& p1, Path& p2, Agent& a1, Agent& a2)
//{
//    auto prev1 = p1.begin();
//    auto prev2 = p2.begin();
//    auto curr1 = p2.begin();
//    auto curr2 = p2.begin();
//    while (curr1 != p1.end() and curr2 != p2.end())
//    {
//        // vertex-vertex conflict
//        if (min(curr1->duration.second, curr2->duration.second) -
//            max(curr1->duration.first, curr2->duration.first) > EPSILON) // non-zero overlapped time interval
//        {
//            for (auto & conflict : a1.roadmap[curr1->vertex].VertexConflicts)
//            {
//                if (conflict.first == a2.idx && conflict.second == curr2->vertex)
//                {
//                    node.conflicts.emplace_back(curr1->task, curr2->task);
//                    return true;
//                }
//            }
//        }
//
//        // vertex-edge conflict
//        if (prev2->vertex != curr2->vertex and // agent 2 traverses an edge
//            min(curr1->duration.second, curr2->duration.first) -
//            max(curr1->duration.first, prev2->duration.second) > EPSILON) // non-zero overlapped time interval
//        {
//            for (auto & conflict : a1.roadmap[curr1->vertex].EdgeConflicts)
//            {
//                if (conflict.first == a2.idx && conflict.second == curr2->edge)
//                {
//                    node.conflicts.emplace_back(curr1->task, curr2->task);
//                    return true;
//                }
//            }
//        }
//
//        // edge-vertex conflict
//        if (prev1->vertex != curr1->vertex and // agent 1 traverses an edge
//            min(curr1->duration.first, curr2->duration.second) -
//            max(prev1->duration.second, curr2->duration.first) > EPSILON) // non-zero overlapped time interval
//        {
//            for (auto & conflict : a1.roadmap[curr1->edge].VertexConflicts)
//            {
//                if (conflict.first == a2.idx && conflict.second == curr2->vertex)
//                {
//                    node.conflicts.emplace_back(curr1->task, curr2->task);
//                    return true;
//                }
//            }
//        }
//
//        // edge-edge conflict
//        if (prev1->vertex != curr1->vertex and // agent 1 traverses an edge
//            prev2->vertex != curr2->vertex and // agent 2 traverses an edge
//            min(curr1->duration.first, curr2->duration.first) -
//            max(prev1->duration.second, prev2->duration.second) > EPSILON) // non-zero overlapped time interval
//        {
//            for (auto & conflict : a1.roadmap[curr1->edge].EdgeConflicts)
//            {
//                if (conflict.first == a2.idx && conflict.second == curr2->edge)
//                {
//                    node.conflicts.emplace_back(curr1->task, curr2->task);
//                    return true;
//                }
//            }
//        }
//
//        // advance the smaller pointers
//        if (curr1->duration.first < curr2->duration.first)
//        {
//            prev1 = curr1;
//            ++curr1;
//        }
//        else
//        {
//            prev2 = curr2;
//            ++curr2;
//        }
//    }
//    return false;
//}
//
//void PBS::update(PBSNode& node)
//{
//    priorities.resize(instance.getNumOfTasks(), set<Task*>());
//    paths.resize(instance.getNumOfTasks(), nullptr);
//
//    for (auto curr = &node; curr != nullptr; curr = curr->parent)
//    {
//        if (curr->parent != nullptr) // non-root node
//            priorities[instance.getTaskID(*curr->priority.first)].insert(curr->priority.second);
//        for (auto & path_pair : curr->new_paths)
//        {
//            auto task_idx = instance.getTaskID(*path_pair.first);
//            if (paths[task_idx] == nullptr)
//                paths[task_idx] = &path_pair.second;
//        }
//    }
//}
//
//bool PBS::addConstraint(PBSNode& node, Task& low, Task& high)
//{
//    node.priority = make_pair(&low, &high);
//    priorities[instance.getTaskID(low)].insert(&high);
//    topologicalSort(ordered_tasks);
//    auto pt = std::find(ordered_tasks.begin(), ordered_tasks.end(), &low);
//    assert(pt != ordered_tasks.end());
//    set<Task*> to_replan;
//    to_replan.insert(&low);
//    while(!to_replan.empty())
//    {
//        int rst = to_replan.erase(*pt);
//        if (rst > 0) // erase successfully
//        {
//            auto old_completion_time = paths[instance.getTaskID(**pt)]->back().duration.second;
//            // Re-plan path
//            if(!planPath(**pt, node))
//                return false;
//
//            // Delete old conflicts
//            auto c = node.conflicts.begin();
//            while (c != node.conflicts.end())
//            {
//                if (c->first == *pt or c->second == *pt)
//                    c = node.conflicts.erase(c);
//                else
//                    ++c;
//            }
//
//            // Find new conflicts
//            auto p1 = &node.new_paths.back().second;
//            for (auto t2 : ordered_tasks)
//            {
//                auto p2 = paths[instance.getTaskID(*t2)];
//                if (p2 == nullptr or //empty path
//                    min(p2->back().duration.second, p1->back().duration.second) -
//                    max(p2->front().duration.first, p1->front().duration.first) < EPSILON) // No overlapped time interval
//                    continue;
//                if (findConflicts(node, *p1, *p2, instance.agents[node.new_paths.back().first->agent_idx],
//                                  instance.agents[p2->front().task->agent_idx]))
//                {
//                    //??????????
//                    to_replan.insert(t2);
//                }
//            }
//
//            // Check preconditions ???????????
//            auto new_completion_time = paths[instance.getTaskID(**pt)]->back().duration.second;
//            if (new_completion_time - old_completion_time > EPSILON)
//            {
//
//                for (auto p = (*pt)->next; p != nullptr and paths[instance.getTaskID(*p)] != nullptr; p = p->next)
//                {
//                    to_replan.insert(p);
//                }
//            }
//        }
//        ++pt;
//    }
//    return true;
//}