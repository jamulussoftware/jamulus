/******************************************************************************\
 * Copyright (c) 2021-2025
 *
 * Author(s):
 *  Stefan Menzel
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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

class CThreadPool
{
public:
    CThreadPool() = default;
    CThreadPool ( size_t );
    template<class F, class... Args>
    auto enqueue ( F&& f, Args&&... args ) -> std::future<typename std::result_of<F ( Args... )>::type>;
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
auto CThreadPool::enqueue ( F&& f, Args&&... args ) -> std::future<typename std::result_of<F ( Args... )>::type>
{
    using return_type = typename std::result_of<F ( Args... )>::type;

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
