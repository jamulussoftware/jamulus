/******************************************************************************\
 * Copyright (c) 2021-2026
 *
 * Author(s):
 *  Stefan Menzel
 *
 * As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
 * under AGPL 3.0 or any later version.
 *
 * Existing code: Code contributed before 3.12.1dev (commit eb172d47) was licensed under GPL 2.0+.
 * This code will be licensed under GPL 3.0 (or any later version) from
 * 3.12.1dev (commit eb172d47).  When distributed as part of Jamulus, the AGPL 3.0 terms govern
 * the combined work, including network use provisions.
 *
 ******************************************************************************
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
\******************************************************************************/

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

// compatibility with C++17 deprecation of result_of
#if __cplusplus >= 201703L
// std::invoke_result_t should be used from C++17 onwards, but is not available in C++14 and earlier
template<class F, class... Args>
using threadpool_result_t = std::invoke_result_t<F, Args...>;
#else
// std::result_of should be used on C++14 and earlier, but is deprecated in C++17
template<class F, class... Args>
using threadpool_result_t = typename std::result_of<F ( Args... )>::type;
#endif

class CThreadPool
{
public:
    CThreadPool ( size_t );
    template<class F, class... Args>
    auto enqueue ( F&& f, Args&&... args ) -> std::future<threadpool_result_t<F, Args...>>;
    ~CThreadPool();

private:
    // need to keep track of threads so we can join them
    std::vector<std::thread> workers;
    // the task queue
    std::queue<std::function<void()>> tasks;

    // synchronization
    std::mutex              queue_mutex;
    std::condition_variable condition;
    bool                    stop;
};

// the constructor just launches some amount of workers
inline CThreadPool::CThreadPool ( size_t threads ) : stop ( false )
{
    for ( size_t i = 0; i < threads; ++i )
    {
        workers.emplace_back ( [this] {
            for ( ;; )
            {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock ( this->queue_mutex );
                    this->condition.wait ( lock, [this] { return this->stop || !this->tasks.empty(); } );

                    if ( this->stop && this->tasks.empty() )
                    {
                        return;
                    }

                    task = std::move ( this->tasks.front() );
                    this->tasks.pop();
                }

                task();
            }
        } );
    }
}

// add new work item to the pool
template<class F, class... Args>
auto CThreadPool::enqueue ( F&& f, Args&&... args ) -> std::future<threadpool_result_t<F, Args...>>
{
    using return_type = threadpool_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<return_type()>> ( std::bind ( std::forward<F> ( f ), std::forward<Args> ( args )... ) );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock ( queue_mutex );

        // don't allow enqueueing after stopping the pool
        if ( stop )
            throw std::runtime_error ( "enqueue on stopped CThreadPool" );

        tasks.emplace ( [task]() { ( *task )(); } );
    }
    condition.notify_one();
    return res;
}

// the destructor joins all threads
inline CThreadPool::~CThreadPool()
{
    {
        std::unique_lock<std::mutex> lock ( queue_mutex );
        stop = true;
    }
    condition.notify_all();
    for ( std::thread& worker : workers )
        worker.join();
}

#endif
