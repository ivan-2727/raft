#pragma once
#include <bits/stdc++.h>
#include "sim_server.cpp"
#include "sim_connection.cpp"
#include "utils.cpp"

class Cluster {
private:
    std::unordered_map<int64_t, std::unique_ptr<SimServer>> servers_;
public:
    bool addServer(const std::unordered_map<std::string, std::string>&);
    bool addManyServers(const std::unordered_map<std::string, std::string>&);
    void removeAllServers();
    bool toggle(int64_t);
    bool createNewEntry(int64_t id, const std::string& data);
    bool execCommand(const std::string&);
    int size();
    std::vector<SimServer::ExportState> getExportStates();
};

bool Cluster::addServer(const std::unordered_map<std::string, std::string>& simServerConfig) {
    auto id = std::stoll(simServerConfig.find("id")->second);
    auto x = servers_.find(id);
    if (x != servers_.end()) {
        return false;
    }
    servers_[id] = std::make_unique<SimServer>(simServerConfig);
    auto y = servers_.find(id);
    for (auto& [otherId, server] : servers_) {
        if (otherId != id) {
            auto side1 = std::make_shared<SimConnection<RaftNode::Request,RaftNode::Response>>();
            auto side2 = std::make_shared<SimConnection<RaftNode::Request,RaftNode::Response>>();
            server->addConnection(id, {side1, side2});
            y->second->addConnection(otherId, {side2, side1});
        }
    }

    return true;
}

bool Cluster::addManyServers(const std::unordered_map<std::string, std::string>& simServerConfigs) {
    if (!servers_.empty()) {
        return false;
    } 
    auto n = std::stoi(simServerConfigs.find("numOfServers")->second); 
    for (auto id = 0; id < n; id++) {
        auto simServerConfig = simServerConfigs;
        simServerConfig.erase("numOfServers");
        simServerConfig["id"] = std::to_string(id);
        addServer(simServerConfig);
    }
    return true;
}

void Cluster::removeAllServers() {
    for (auto& [_, server] : servers_) {
        server->stop();
    }
    servers_.clear();
}

std::vector<SimServer::ExportState> Cluster::getExportStates() {
    std::vector<SimServer::ExportState> states;
    for (auto& [_, server] : servers_) {
        states.push_back(server->getExportState());
    }
    sort(states.begin(), states.end(), [](const auto& s1, const auto& s2) -> bool {
        return s1.id < s2.id;
    });
    return states; 
}

int Cluster::size() {
    return servers_.size();
}

bool Cluster::toggle(int64_t id) {
    auto x = servers_.find(id);
    if (x == servers_.end()) {
        return false;
    }
    x->second->toggle();
    return true;
}

bool Cluster::createNewEntry(int64_t id, const std::string& data) {
    auto x = servers_.find(id);
    if (x == servers_.end()) {
        return false; 
    }
    x->second->createNewEntry(data);
    return true;
}

bool Cluster::execCommand(const std::string& cmdStr) {
    try {
        auto cmd = split(cmdStr, ' '); 
        auto x = servers_.find(std::stoll(cmd.at(1)));
        if (x == servers_.end()) {
            return false;
        }
        if (cmd.at(0) == "toggle") {
            if (cmd.size() != 2) {
                return false;
            }
            x->second->toggle();
            return true;
        }
        if (cmd.at(0) == "do") {
            if (cmd.size() != 3) {
                return false;
            }
            if (!x->second->getActiveStatus()) {
                return false;
            }
            x->second->createNewEntry(cmd.at(2));
            return true;
        }
    } catch (...) {}
    return false;
}