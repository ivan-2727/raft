#pragma once
#include <bits/stdc++.h>
#include "../../raft_node/raft_node.cpp"
#include "../../utils/utils.cpp"

std::string serializeRequest(const RaftNode::Request& req) {
    std::string s = format(
        "*\n"
        "*\n"
        "*\n",
        {std::to_string(req.type),
        std::to_string(req.id),
        std::to_string(req.term)}
    );
    for (int i = 0; i < req.entries.size(); i++) {
        s += format("* * *|", {std::to_string(req.entries[i].idx), std::to_string(req.entries[i].term), req.entries[i].data});
    }
    s += "\n";
    s += std::to_string(req.commitIdx1);
    return s; 
}

std::optional<RaftNode::Request> deserializeRequest(const std::string& s) {
    try {
        RaftNode::Request req;
        auto got = split(s, '\n');
        req.type = static_cast<RaftNode::MsgType>(std::stoi(got.at(0)));
        req.id = std::stoll(got.at(1));
        req.term = std::stoll(got.at(2));
        for (const auto& entryStr : split(got.at(3), '|')) {
            auto entry = split(entryStr, ' ');
            req.entries.push_back({std::stoi(entry.at(0)), std::stoll(entry.at(1)), entry.at(2)});
        }
        req.commitIdx1 = std::stoi(got.at(4));
        return req;
    } catch(...) {}
    return std::nullopt;     
}

std::string serializeResponse(const RaftNode::Response& res) {
    return format("*\n*\n*\n*", {
        std::to_string(res.type),
        std::to_string(res.id),
        std::to_string(res.term),
        std::to_string(res.result)
    });
}

std::optional<RaftNode::Response> deserializeResponse(const std::string& s) {
    try {
        auto got = split(s, '\n');
        RaftNode::Response res;
        res.type = static_cast<RaftNode::MsgType>(std::stoi(got.at(0)));
        res.id = std::stoll(got.at(1));
        res.term = std::stoll(got.at(2));
        res.result = std::stoi(got.at(3));
        return res;
    } catch (...) {}
    return std::nullopt; 
}