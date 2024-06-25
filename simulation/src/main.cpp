#include <bits/stdc++.h>
#include "cluster.cpp"

int main(void) {
    Cluster cluster;
    auto simServerConfigs = readConfig("config/config.txt");
    auto refreshPeriod = std::stoi(simServerConfigs.find("refreshPeriod")->second);
    cluster.addManyServers(simServerConfigs);
    auto cmdLoop = std::thread([&cluster]() -> void {
        while(true) {
            std::string cmdStr;
            getline(std::cin, cmdStr);
            if (cluster.execCommand(cmdStr)) {
                std::cout << "Ok\n";
            } else {
                std::cout << "Bad command or the server is deactivated\n";
            }
        }
    });
    std::cout << "Ready\n";
    while (true) {
        auto exportStates = cluster.getExportStates();
        std::ofstream stateFile;
        stateFile.open("result/state.txt", std::ios::trunc);
        for (auto& exportState : exportStates) {
            std::string s = 
                "Id: *\n"
                "Current term: *\n"
                "Role: *\n"
                "Voted for id: *\n"
                "Leader id: *\n"
                "Timeout, ms: *\n"
                "Since start, ms: *\n"
                "Since last update, ms: *\n"
                "Active: *\n"
                "CommitIdx: *, *\n"
                "Log: *\n"
                "Store: *\n"
                "-------------------------\n";
            std::string role = "None";
            if (exportState.role == RaftNode::Role::FOLL) {
                role = "FOLL";
            } else if (exportState.role == RaftNode::Role::CAND) {
                role = "CAND";
            } else if (exportState.role == RaftNode::Role::LEAD) {
                role = "LEAD";
            }
            stateFile << format(s, {
                std::to_string(exportState.id),
                std::to_string(exportState.curTerm),
                role,
                exportState.votedForId.has_value() ? std::to_string(exportState.votedForId.value()) : "None",
                exportState.leaderId.has_value() ? std::to_string(exportState.leaderId.value()) : "None",
                std::to_string(exportState.timeout),
                std::to_string(exportState.sinceStart),
                std::to_string(exportState.sinceLastUpd),
                exportState.active ? "True" : "False",
                std::to_string(exportState.commitIdx[0]), std::to_string(exportState.commitIdx[1]),
                stringify(exportState.log),
                stringify(exportState.store)
            }) << "\n\n";
        }
        stateFile.close();
        std::this_thread::sleep_for(std::chrono::milliseconds(refreshPeriod));
    }
    cmdLoop.join();
    return 0;
}