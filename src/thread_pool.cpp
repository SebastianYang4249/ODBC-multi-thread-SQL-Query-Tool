///////////////////////////////////////////////////////////////////////////////
// Copyright 2022, Oushu Inc.
// All rights reserved.
//
// Author:
///////////////////////////////////////////////////////////////////////////////

#include "../include/thread_pool.h"

#define MAX_CMD_LINES 100

class thread_pool {
public:
    explicit thread_pool(int n) : isRunning{false}, startTime{clock()}, Que(n) {}

    ~thread_pool() {
        endTime = clock();
        std::cout << startTime << " end_time:" << (double) endTime / CLOCKS_PER_SEC * 1000 << "ms " << "execute_time:"
                  << (double) (endTime - startTime) / CLOCKS_PER_SEC * 1000 << "ms" << std::endl;
        stop();
    }

    //start a thread pool
    void start(size_t n) {

        if (isRunning) return;

        isRunning = true;
        threads.reserve(n);
        while (n--) {
            threads.emplace_back(&thread_pool::worker, this);
        }
    }

    //stop thread pool and stop all threads
    void stop() {

        if (!isRunning) return;

        {
            std::lock_guard<std::mutex> lk{muteX};
            isRunning = false;
            notFull.notify_all();
            notEmpty.notify_all();
        }

        for (auto &t : threads) {
            if (t.joinable()) t.join();
        }
    }

    //submit task function
    void submit(std::function<void(std::vector<std::string>, int, std::vector<int>)> f) {

        std::unique_lock<std::mutex> lk{muteX};
        notFull.wait(lk, [this] { return !isRunning || !Que.full(); });

        assert(!isRunning || !Que.full());

        if (!isRunning) return;

        Que.push_back(std::move(f));
        notEmpty.notify_one();

    }

    //copy sql commands and expected results
    void setCmd(std::vector<std::string> _cmds, std::vector<int> res, int _line, int _cmd) {
        this->line = _line;
        this->cmd = _cmd;
        for (int i = 0; i < _cmd; ++i) {
            this->sqlres.push_back(res[i]);
            this->cmds.push_back(_cmds[i]);
        }
    }

private:
    void worker() {
        while (true) {
            task t;
            {
                std::unique_lock<std::mutex> lk{muteX};
                notEmpty.wait(lk, [this] { return !isRunning || !Que.empty(); });

                assert(!isRunning || !Que.empty());

                if (!isRunning) return;

                t = std::move(Que.front());
                Que.pop_front();
                notFull.notify_one();
            }
            t(cmds, cmd, sqlres);
        }
    }

    int option;

    using task = std::function<void(std::vector<std::string>, int, std::vector<int>)>;

    std::vector<std::thread> threads;
    boost::circular_buffer<task> Que;
    std::mutex muteX;
    std::condition_variable notFull;
    std::condition_variable notEmpty;
    bool isRunning;

    std::vector<std::string> cmds;
    int line;
    int cmd;
    char bashBuf[512];
    std::vector<int> sqlres;

    clock_t startTime, endTime;
};
