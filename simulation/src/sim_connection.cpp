#pragma once 
#include <bits/stdc++.h>
#include "../../raft_node/raft_node.cpp"

template<typename X, typename Y>
struct ReqRes {
    X req; 
    std::shared_ptr<std::promise<Y>> promisedRes;
};

template<typename X, typename Y>
class SimConnection {
private:
    std::mutex mtx_; 
    std::deque<ReqRes<X,Y>> q_;
public:
    void send(const ReqRes<X,Y>& reqRes) {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        q_.push_back(reqRes);
    }
    std::optional<ReqRes<X,Y>> receive() {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        if (q_.empty()) {
            return std::nullopt;
        }
        auto reqRes = q_[0];
        q_.pop_front();
        return reqRes;
    }
};

template<typename X, typename Y>
struct TwoSideSimConnection {
    std::shared_ptr<SimConnection<X,Y>> sender;
    std::shared_ptr<SimConnection<X,Y>> receiver;
};