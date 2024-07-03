#pragma once
#include <bits/stdc++.h>
#include "../raft_node/raft_node.cpp"

std::vector<std::string> split(const std::string& s, char del) {
    std::vector<std::string> res;
    std::string cur;
    for (char c : s) {
        if (c == del) {
            if (!cur.empty()) {
                res.push_back(cur);
                cur.clear();
            }
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) {
        res.push_back(cur);
    }
    return res;
}

std::string format(const std::string& s, const std::vector<std::string>& val) {
    std::string t;
    int i = 0;
    for (char c : s) {
        if (c == '*') {
            t += val[i];
            i++;
        } else {
            t += c;
        }
    }
    return t; 
}

std::string stringify(std::vector<RaftNode::LogEntry>& log) {
    std::string t;
    for (auto& entry : log) {
        t += format("[* * *]", {std::to_string(entry.idx), std::to_string(entry.term), entry.data});
        t += ", ";
    }
    if (!t.empty()) {
        t.pop_back();
        t.pop_back(); 
    }
    return t; 
}

std::string stringify(std::unordered_map<std::string, std::string>& store) {
    std::string t;
    for (auto& [k, v] : store) {
        t += format("[* *]", {k, v});
        t += ", ";
    }
    if (!t.empty()) {
        t.pop_back();
        t.pop_back(); 
    }
    return t; 
}

std::unordered_map<std::string, std::string> readConfig(const std::string& filename) {
    std::ifstream configFile;
    configFile.open(filename);
    std::string line; 
    std::unordered_map<std::string, std::string> config;
    while (getline(configFile, line)) { 
        auto entry = split(line, ' ');
        config[entry[0]] = entry[1];
    } 
    configFile.close();
    return config;
}