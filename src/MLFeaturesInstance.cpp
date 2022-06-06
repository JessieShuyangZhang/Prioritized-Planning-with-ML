#include "MLFeaturesInstance.h"

MLFeaturesInstance::MLFeaturesInstance(Instance& instance, PP& pp)
	:num_agents(instance.num_of_agents), instance(instance), pp(pp)
{
	int obstacle_count = 0;
	for (int i = 0; i < instance.map_size; i++) {
		if (instance.isObstacle(i))
			obstacle_count++;
	}
	obstacle_density = (double)obstacle_count / instance.map_size;
	agent_sparcity = (double)num_agents / (instance.map_size - obstacle_count);

	feature_vec = { (double)num_agents,obstacle_density,agent_sparcity };
}

void MLFeaturesInstance::hashMDDnodeOccurances()
{
	//loop through all agents
		//go through all nodes in their mdd, hash loc to hashmap, [hash value]++
	for (int i = 0; i < num_agents; i++) {
		Agent& agent = pp.agents[i];
		//cout << "agent id: " << agent.id << endl;
		for (auto level: agent.mdd.levels){
			for (auto node : level) {
				mddnode_occurances[node->location] += 1;
				vertexTimestep_agentids[make_pair(node->location, node->level)].emplace_back(agent.id);
				for (auto child : node->children) {
					edgeTimestep_agentids[make_tuple(node->location, child->location, node->level)].emplace_back(agent.id);
				}
			}
		}
	}

	//debug
	/*for (auto it = vertexTimestep_agentids.begin(); it != vertexTimestep_agentids.end(); ++it) {
		cout << it->first.first <<" -- "<<it->first.second << ": [";
		for (auto ita = it->second.begin(); ita != it->second.end(); ++ita) {
			cout << *ita << ",";
		}
		cout << "]"<<endl;
	}*/

	for (auto loc : instance.start_locations) {
		start_locations_hash[loc] = 1; //assuming no 2 agents have same start
	}

	for (auto loc : instance.goal_locations) {
		goal_locations_hash[loc] = 1;//assuming no 2 agents have same goal
	}

	computeVertexConflicts();
	computeEdgeConflicts();
}

void MLFeaturesInstance::computeVertexConflicts() {
	for (auto i = vertexTimestep_agentids.begin(); i != vertexTimestep_agentids.end(); i++) {
		if (i->second.size() <= 1) 
			continue;
		
		for (int ai = 0; ai < i->second.size(); ai++) {
			//cout << i->second[ai] << ": ";
			for (int aj = ai + 1; aj < i->second.size(); aj++) {
				//cout << i->second[aj] <<",";
				agent_vertexConf_bypair[i->second[ai]].insert(i->second[aj]);
				agent_vertexConf_byconf[i->second[ai]] += 1;
				agent_vertexConf_bypair[i->second[aj]].insert(i->second[ai]);
				agent_vertexConf_byconf[i->second[aj]] += 1;

				//TODO: check if cardinal
				if (pp.agents[i->second[ai]].mdd.levels[i->first.second].size() == 1
					&& pp.agents[i->second[aj]].mdd.levels[i->first.second].size() == 1) {
					agent_cardinalConf_bypair[i->second[ai]].insert(i->second[aj]);
					agent_cardinalConf_byconf[i->second[ai]] += 1;
					agent_cardinalConf_bypair[i->second[aj]].insert(i->second[ai]);
					agent_cardinalConf_byconf[i->second[aj]] += 1;
				}
			}
			//cout << endl;
		}
	}
	
	//debug
	/*for (auto it = agent_vertexConf_bypair.begin(); it != agent_vertexConf_bypair.end(); ++it) {
		cout << it->first << ": [";
		for (auto ita = it->second.begin(); ita != it->second.end(); ++ita) {
			cout << *ita <<",";
		}
		cout << "]" << endl;
	}*/
}

void MLFeaturesInstance::computeEdgeConflicts() {
	for (auto i = edgeTimestep_agentids.begin(); i != edgeTimestep_agentids.end(); i++) {
		//only start with edges <u,v> such that u < v in location
		if (std::get<0>(i->first) >= std::get<1>(i->first))
			continue;
		auto foundOpposite = edgeTimestep_agentids.find(make_tuple(std::get<1>(i->first), std::get<0>(i->first), std::get<2>(i->first)));
		if (foundOpposite != edgeTimestep_agentids.end()) { //found opposite edge at same timestep
			/*vector<int>& agentsComing = i->second; 
			vector<int>& agentsGoing = foundOpposite->second;*/
			for (auto agentComing : i->second) {
				for (auto agentGoing : foundOpposite->second) {
					agent_edgeConf_bypair[agentComing].insert(agentGoing);
					agent_edgeConf_byconf[agentComing] += 1;
					agent_edgeConf_bypair[agentGoing].insert(agentComing);
					agent_edgeConf_byconf[agentGoing] += 1;

					//TODO: check if cardinal
					if (pp.agents[agentComing].mdd.levels[std::get<2>(i->first)].size() == 1
						&& pp.agents[agentComing].mdd.levels[(size_t)std::get<2>(i->first) + 1].size() == 1
						&& pp.agents[agentGoing].mdd.levels[std::get<2>(i->first)].size() == 1
						&& pp.agents[agentGoing].mdd.levels[(size_t)(std::get<2>(i->first) + 1)].size() == 1) {
						agent_cardinalConf_bypair[agentComing].insert(agentGoing);
						agent_cardinalConf_byconf[agentComing] += 1;
						agent_cardinalConf_bypair[agentGoing].insert(agentComing);
						agent_cardinalConf_byconf[agentGoing] += 1;
					}
				}
			}
		}
	}
	//debug
	/*for (auto it = agent_edgeConf_bypair.begin(); it != agent_edgeConf_bypair.end(); ++it) {
		cout << it->first << ": [";
		for (auto ita = it->second.begin(); ita != it->second.end(); ++ita) {
			cout << *ita << "," ;
		}
	}*/
}