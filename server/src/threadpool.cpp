#pragma once 
#include <bits/stdc++.h>

class ThreadPool {
private:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> q_;
    std::vector<std::thread> workers_;
    bool active_;
public:
    void init(int n) {
        active_ = true;
        while(n--) {
            workers_.push_back(std::thread([this]() -> void {
                while (true) {
                    std::function<void()> job;
                    {   
                        std::unique_lock<decltype(mtx_)> lock(mtx_);
                        cv_.wait(lock, [this] {
                            return (!q_.empty()) || (!active_);
                        });
                        if (q_.empty() && !active_) {
                            break;
                        }
                        job = q_[0];
                        q_.pop_front();
                    }
                    job();
                }
            }));
        }
    }
    void enque(std::function<void()> job) {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        q_.push_back(job);
        cv_.notify_one();
    }
    ~ThreadPool() {
        active_ = false;
        cv_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
};

// int main(void) {
//     auto tp = ThreadPool(5);
//     for (int i = 0; i < 100; i++) {
//         tp.enque([i]() -> void {
//             printf("%d\n", i);
//         });
//     }
//     return 0;
// }