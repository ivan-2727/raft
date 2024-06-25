#pragma once 
#include <bits/stdc++.h>
#include "../../raft_node/raft_node.cpp"
#include "sim_connection.cpp"
#include "utils.cpp"

class SimServer {
private:
    std::mutex mtx_;
    std::thread loop_;
    std::unique_ptr<RaftNode> node_;
    std::unordered_map<int64_t, TwoSideSimConnection<RaftNode::Request,RaftNode::Response>> conns_;
    std::unordered_map<std::string, std::string> store_; 
    bool active_;
    bool alive_;
    void commit(const std::string& data);
public:
    SimServer(const std::unordered_map<std::string, std::string>&);
    ~SimServer();
    class ExportState : public RaftNode::ExportState {
    public:
        bool active;
        std::unordered_map<std::string, std::string> store; 
    };
    ExportState getExportState();
    void stop();
    void toggle();
    void createNewEntry(const std::string& data);
    void addConnection(int64_t, const TwoSideSimConnection<RaftNode::Request,RaftNode::Response>&);
    void removeConnection(int64_t);
    bool getActiveStatus();
};

SimServer::SimServer(const std::unordered_map<std::string, std::string>& simServerConfig) {
    active_ = true;
    alive_ = true; 
    auto refreshPeriod = std::stoll(simServerConfig.find("refreshPeriod")->second);
    node_ = std::make_unique<RaftNode>(simServerConfig); 
    loop_ = std::thread([this, refreshPeriod]() -> void {
        while (true) {
            {
                std::unique_lock<decltype(mtx_)> lock(mtx_);
                if (!alive_) {
                    break; 
                }
                if (!active_) {
                    continue;
                }
                for (const std::string& data : node_->getCommittedDatas()) {
                    commit(data);
                }
                auto requests = node_->createRequests();
                std::vector<std::shared_ptr<std::promise<RaftNode::Response>>> promisedResS;
                for (auto& [otherId, reqs] : requests) {
                    auto x = conns_.find(otherId);
                    if (x == conns_.end()) {
                        throw std::runtime_error("Connection " + std::to_string(otherId) + " not found");
                    }
                    for (auto& req : reqs) {
                        auto promisedRes = std::make_shared<std::promise<RaftNode::Response>>();
                        x->second.sender->send({req, promisedRes});
                        promisedResS.push_back(promisedRes);
                    }
                }
                for (auto promisedRes : promisedResS) {
                    auto futureRes = promisedRes->get_future();
                    if (futureRes.wait_for(std::chrono::milliseconds(2*(1+conns_.size())*refreshPeriod)) == std::future_status::ready) {
                        node_->handleResponse(futureRes.get());
                    }
                }
                for (auto& [otherId, connPair] : conns_) {
                    while (true) {
                        auto maybeReqRes = connPair.receiver->receive();
                        if (maybeReqRes.has_value()) {
                            auto reqRes = maybeReqRes.value();
                            reqRes.promisedRes->set_value(node_->handleRequest(reqRes.req));
                        } else {
                            break;
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(refreshPeriod));
        }
    });
}

SimServer::~SimServer() {
    if (loop_.joinable()) {
        loop_.join();
    }
}

void SimServer::stop() {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    alive_ = false;
}

void SimServer::toggle() {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    active_ = !active_;
}

SimServer::ExportState SimServer::getExportState() {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    return {node_->getExportState(), active_, store_};
}

void SimServer::addConnection(int64_t otherId, const TwoSideSimConnection<RaftNode::Request,RaftNode::Response>& conn) {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    auto x = conns_.find(otherId);
    if (x == conns_.end()) {
        conns_[otherId] = conn; 
        node_->addOther(otherId);
    }
}

void SimServer::removeConnection(int64_t otherId) {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    conns_.erase(otherId);
    node_->removeOther(otherId);
}

void SimServer::createNewEntry(const std::string& data) {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    node_->createNewEntry(data);
}

void SimServer::commit(const std::string& data) {
    try {
        auto got = split(data, ';');
        if (got.at(0) == "put") {
            store_[got.at(1)] = got.at(2);
        } else if (got.at(0) == "delete") {
            store_.erase(got.at(1));
        }
    } catch(...) {}
}

bool SimServer::getActiveStatus() {
    std::unique_lock<decltype(mtx_)> lock(mtx_);
    return active_;
}