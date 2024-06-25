#pragma once
#include <bits/stdc++.h>

class RaftNode {
public:
    struct LogEntry {
        int idx; 
        int64_t term;
        std::string data;
    };
    enum Role {
        FOLL,
        CAND,
        LEAD
    };
    class State {
    public:
        int64_t id; 
        int64_t timeout;
        int64_t curTerm;
        Role role;
        std::optional<int64_t> votedForId;
        std::optional<int64_t> leaderId;
        std::vector<int> commitIdx;
    };
    class ExportState : public State {
    public:
        int64_t sinceStart;
        int64_t sinceLastUpd;
        std::vector<LogEntry> log; 
    };
    enum MsgType {
        AskVote,
        AppendEntries,
        CreateNewEntries
    };
    struct Request {
        MsgType type;
        int64_t id;
        int64_t term;
        std::vector<LogEntry> entries;
        int commitIdx1;
    };
    struct Response {
        MsgType type;
        int64_t id;
        int64_t term;
        int result = 0;
    };

private:
    std::mutex mtx_;
    State state_;
    int aeSize_; 
    std::unordered_map<int64_t, int> nextIdx_;
    int votes_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_, lastUpd_;
    std::vector<LogEntry> log_;
    std::unordered_map<int, int> replCnt_;
    std::vector<LogEntry> pending_;
     
    // without own lock
    Response handleAskVoteRequest_(const Request&);
    Response handleAppendEntriesRequest_(const Request&); 
    Response handleCreateNewEntriesRequest_(const Request&); 
    void handleAskVoteResponse_(const Response&);
    void handleAppendEntriesResponse_(const Response&);
    void handleCreateNewEntriesResponse_(const Response&);
    int64_t sinceLastUpd_();
    int64_t sinceStart_();
    int64_t majority_();
    void updateCommitIdx_(int);

public:
    RaftNode(const std::unordered_map<std::string, std::string>&);

    // with lock 
    Response handleRequest(const Request&);
    std::unordered_map<int64_t, std::vector<RaftNode::Request>> createRequests();
    void handleResponse(const Response&);
    ExportState getExportState();
    ExportState getExportState(int); 
    void addOther(int64_t);
    void removeOther(int64_t);
    void createNewEntry(const std::string& data);
    std::vector<std::string> getCommittedDatas();
     
};

void RaftNode::updateCommitIdx_(int commitIdx1) {
    state_.commitIdx[1] = std::max(state_.commitIdx[1], std::min(commitIdx1, int(log_.size())-1));
}

int64_t RaftNode::majority_() {
    auto n = 1 + nextIdx_.size();
    return n/2 + n%2; 
}

int64_t RaftNode::sinceLastUpd_() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastUpd_).count();
}

int64_t RaftNode::sinceStart_() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_).count();
}

RaftNode::RaftNode(const std::unordered_map<std::string, std::string>& config) {
    state_.id = std::stoll(config.find("id")->second);
    state_.timeout = std::stoll(config.find("timeout")->second); 
    aeSize_ = std::stoll(config.find("aeSize")->second);
    start_ = std::chrono::high_resolution_clock::now();
    lastUpd_ = start_;
    votes_ = 0; 
    state_.role = Role::FOLL;
    state_.curTerm = 0; 
    state_.commitIdx = {-1,-1};
}

RaftNode::Response RaftNode::handleAskVoteRequest_(const Request& req) {
    Response res = {RaftNode::MsgType::AskVote, state_.id, state_.curTerm};
    if (state_.votedForId.has_value() && state_.votedForId.value() != req.id) {
        res.result = 0;
        return res;
    }
    state_.curTerm = req.term;
    state_.votedForId = state_.id;
    state_.role = Role::FOLL;
    lastUpd_ = std::chrono::high_resolution_clock::now();
    res.result = 1;
    return res;
}

RaftNode::Response RaftNode::handleAppendEntriesRequest_(const Request& req) {
    Response res = {RaftNode::MsgType::AppendEntries, state_.id, state_.curTerm};
    state_.curTerm = req.term;
    state_.votedForId = std::nullopt;
    state_.leaderId = req.id;
    state_.role = Role::FOLL;
    lastUpd_ = std::chrono::high_resolution_clock::now();
    if (req.entries.empty()) {
        updateCommitIdx_(req.commitIdx1);
        res.result = 0;
        return res;
    }
    if (!log_.empty() && (req.entries[0].idx >= log_.size() || log_[req.entries[0].idx].term != req.entries[0].term)) {
        res.result = -1;
        return res; 
    }
    if (log_.empty()) {
        for (int i = 0; i < req.entries.size(); i++) {
            log_.push_back(req.entries[i]);
        }
        updateCommitIdx_(req.commitIdx1);
        res.result = 1;
        return res; 
    }
    int keep = log_.size(); 
    int l = 1; 
    for (int i = req.entries[0].idx; i < log_.size(); i++) {
        auto j = i - req.entries[0].idx;
        if (log_[i].term != req.entries[j].term) {
            keep = i;
            l = j;
            break; 
        }
    }
    while (log_.size() > keep) {
        log_.pop_back(); 
    }
    for (int i = l; i < req.entries.size(); i++) {
        log_.push_back(req.entries[i]);
    }
    updateCommitIdx_(req.commitIdx1);
    res.result = 1;
    return res;
}

RaftNode::Response RaftNode::handleCreateNewEntriesRequest_(const Request& req) {
    Response res = {RaftNode::MsgType::CreateNewEntries, state_.id, state_.curTerm, 0};
    if (state_.role == Role::LEAD) {
        for (auto& entry : req.entries) {
            log_.push_back({log_.empty() ? 0 : (log_.back().idx+1), state_.curTerm, entry.data});
        }
        res.result = 1;
    } else {
        for (auto& entry : req.entries) {
            pending_.push_back(entry);
        }
    }
    return res; 
}

RaftNode::Response RaftNode::handleRequest(const Request& req) {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    if (state_.curTerm > req.term) {
        return {req.type, state_.id, state_.curTerm, 0};
    }
    if (req.type == MsgType::AskVote) {
        return handleAskVoteRequest_(req);
    }
    if (req.type == MsgType::AppendEntries) {
        return handleAppendEntriesRequest_(req);
    }
    return handleCreateNewEntriesRequest_(req);
}

std::unordered_map<int64_t, std::vector<RaftNode::Request>> RaftNode::createRequests() {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    if (state_.role == Role::FOLL) {
        std::unordered_map<int64_t, std::vector<RaftNode::Request>> requests;
        if (sinceLastUpd_() > 2*state_.timeout + rand()%state_.timeout) {
            state_.role = Role::CAND;
            state_.curTerm += 1;
            state_.votedForId = state_.id;
            votes_ = 1; 
            state_.leaderId = std::nullopt;
            lastUpd_ = std::chrono::high_resolution_clock::now();
            for (auto [otherId, _] : nextIdx_) {
                requests[otherId].push_back({RaftNode::MsgType::AskVote, state_.id, state_.curTerm, {}}); 
            }
        }
        if (state_.leaderId.has_value() && !pending_.empty()) {
            requests[state_.leaderId.value()].push_back({RaftNode::MsgType::CreateNewEntries, state_.id, state_.curTerm, pending_});
            pending_.clear();
        }
        return requests;
    } else if (state_.role == Role::CAND) {
        if (sinceLastUpd_() > 2*state_.timeout) {
            state_.role = Role::FOLL;
            state_.votedForId = std::nullopt;
            lastUpd_ = std::chrono::high_resolution_clock::now();
        }
        return {}; 
    } else if (state_.role == Role::LEAD) {
        for (auto& entry : pending_) {
            log_.push_back({log_.empty() ? 0 : (log_.back().idx+1), state_.curTerm, entry.data});
        }
        pending_.clear(); 
        if (sinceLastUpd_() > state_.timeout) {
            lastUpd_ = std::chrono::high_resolution_clock::now();
            std::unordered_map<int64_t, std::vector<RaftNode::Request>> requests;
            for (auto [otherId, idx] : nextIdx_) {
                std::vector<LogEntry> entries;
                for (auto i = idx; i < idx+aeSize_ && i < log_.size(); i++) {
                    entries.push_back(log_[i]);
                }
                requests[otherId].push_back({RaftNode::MsgType::AppendEntries, state_.id, state_.curTerm, entries, state_.commitIdx[1]}); 
            }
            return requests; 
        }
    }
    return {};
}

void RaftNode::handleAskVoteResponse_(const RaftNode::Response& res) {
    if (state_.role == Role::CAND) {
        votes_ += res.result;
        if (votes_ >= majority_()) {
            state_.role = Role::LEAD;
            lastUpd_ = std::chrono::high_resolution_clock::now();
        }
    }
}

void RaftNode::handleAppendEntriesResponse_(const RaftNode::Response& res) {
    if (state_.role == Role::LEAD) {
        nextIdx_.find(res.id)->second += res.result;
        if (res.result == 1) {
            std::optional<int> commitIdxUpd;
            for (int i = state_.commitIdx[1]+1; i < log_.size(); i++) {
                replCnt_[i]++;
                if (replCnt_.find(i)->second >= majority_()) {
                    commitIdxUpd = i; 
                } else {
                    break;
                }
            }
            if (commitIdxUpd.has_value()) {
                state_.commitIdx[1] = commitIdxUpd.value();
            }
        }
    }
}

void RaftNode::handleCreateNewEntriesResponse_(const RaftNode::Response& res) {
}

void RaftNode::handleResponse(const RaftNode::Response& res) {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    if (res.term > state_.curTerm) {
        state_.role = Role::FOLL;
        state_.curTerm = res.term;
        state_.votedForId = std::nullopt;
        lastUpd_ = std::chrono::high_resolution_clock::now();
    }
    if (res.type == MsgType::AskVote) {
        handleAskVoteResponse_(res);
    } else if (res.type == MsgType::AppendEntries) {
        handleAppendEntriesResponse_(res);
    } else {
        handleCreateNewEntriesResponse_(res);
    }
}

void RaftNode::addOther(int64_t otherId) {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    if (nextIdx_.find(otherId) != nextIdx_.end()) {
        return;
    }
    nextIdx_[otherId] = 0; 
}

void RaftNode::createNewEntry(const std::string& data) {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    pending_.push_back({0, 0, data}); 
}

void RaftNode::removeOther(int64_t otherId) {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    nextIdx_.erase(otherId);
}

RaftNode::ExportState RaftNode::getExportState() {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    return {state_, sinceStart_(), sinceLastUpd_(), log_};
}

RaftNode::ExportState RaftNode::getExportState(int cnt) {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    std::vector<LogEntry> log;
    for (int i = std::max(0, int(log_.size()) - cnt); i < log_.size(); i++) {
        log.push_back(log_[i]);
    }
    return {state_, sinceStart_(), sinceLastUpd_(), log};
}

std::vector<std::string> RaftNode::getCommittedDatas() {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    std::vector<std::string> datas;
    for (int i = state_.commitIdx[0]+1; i <= state_.commitIdx[1]; i++) {
        datas.push_back(log_[i].data);
    }
    state_.commitIdx[0] = state_.commitIdx[1];
    return datas;
}
 