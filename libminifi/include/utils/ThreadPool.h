/**
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef LIBMINIFI_INCLUDE_THREAD_POOL_H
#define LIBMINIFI_INCLUDE_THREAD_POOL_H

#include <chrono>
#include <sstream>
#include <iostream>
#include <atomic>
#include <mutex>
#include <map>
#include <vector>
#include <queue>
#include <future>
#include <thread>
#include <functional>

#include "BackTrace.h"
#include "Monitors.h"
#include "core/expect.h"
#include "controllers/ThreadManagementService.h"
#include "concurrentqueue.h"
#include "core/controller/ControllerService.h"
#include "core/controller/ControllerServiceProvider.h"
namespace org {
namespace apache {
namespace nifi {
namespace minifi {
namespace utils {

/**
 * Worker task
 * purpose: Provides a wrapper for the functor
 * and returns a future based on the template argument.
 */
template<typename T>
class Worker {
 public:
  explicit Worker(std::function<T()> &task, const std::string &identifier, std::unique_ptr<AfterExecute<T>> run_determinant)
      : identifier_(identifier),
        time_slice_(std::chrono::steady_clock::now()),
        task(task),
        run_determinant_(std::move(run_determinant)) {
    promise = std::make_shared<std::promise<T>>();
  }

  explicit Worker(std::function<T()> &task, const std::string &identifier)
      : identifier_(identifier),
        time_slice_(std::chrono::steady_clock::now()),
        task(task),
        run_determinant_(nullptr) {
    promise = std::make_shared<std::promise<T>>();
  }

  explicit Worker(const std::string identifier = "")
      : identifier_(identifier),
        time_slice_(std::chrono::steady_clock::now()) {
  }

  virtual ~Worker() {

  }

  /**
   * Move constructor for worker tasks
   */
  Worker(Worker &&other)
      : identifier_(std::move(other.identifier_)),
        time_slice_(std::move(other.time_slice_)),
        task(std::move(other.task)),
        run_determinant_(std::move(other.run_determinant_)),
        promise(other.promise) {
  }

  /**
   * Runs the task and takes the output from the functor
   * setting the result into the promise
   * @return whether or not to continue running
   *   false == finished || error
   *   true == run again
   */
  virtual bool run() {
    T result = task();
    if (run_determinant_ == nullptr || (run_determinant_->isFinished(result) || run_determinant_->isCancelled(result))) {
      promise->set_value(result);
      return false;
    }
    time_slice_ = increment_time(run_determinant_->wait_time());
    return true;
  }

  virtual void setIdentifier(const std::string identifier) {
    identifier_ = identifier;
  }

  virtual std::chrono::time_point<std::chrono::steady_clock> getTimeSlice() const {
    return time_slice_;
  }

  virtual std::chrono::milliseconds getWaitTime() {
    return run_determinant_->wait_time();
  }

  Worker<T>(const Worker<T>&) = delete;
  Worker<T>& operator =(const Worker<T>&) = delete;

  Worker<T>& operator =(Worker<T> &&);

  std::shared_ptr<std::promise<T>> getPromise();

  const std::string &getIdentifier() {
    return identifier_;
  }
 protected:

  inline std::chrono::time_point<std::chrono::steady_clock> increment_time(const std::chrono::milliseconds &time) {
    return std::chrono::steady_clock::now() + time;
  }

  std::string identifier_;
  std::chrono::time_point<std::chrono::steady_clock> time_slice_;
  std::function<T()> task;
  std::unique_ptr<AfterExecute<T>> run_determinant_;
  std::shared_ptr<std::promise<T>> promise;
};

template<typename T>
class WorkerComparator {
 public:
  bool operator()(Worker<T> &a, Worker<T> &b) {
    return a.getTimeSlice() < b.getTimeSlice();
  }
};

template<typename T>
Worker<T>& Worker<T>::operator =(Worker<T> && other) {
  task = std::move(other.task);
  promise = other.promise;
  time_slice_ = std::move(other.time_slice_);
  identifier_ = std::move(other.identifier_);
  run_determinant_ = std::move(other.run_determinant_);
  return *this;
}

template<typename T>
std::shared_ptr<std::promise<T>> Worker<T>::getPromise() {
  return promise;
}

class WorkerThread {
 public:
  explicit WorkerThread(std::thread thread, const std::string &name = "NamelessWorker")
      : is_running_(false),
        thread_(std::move(thread)),
        name_(name) {

  }
  WorkerThread(const std::string &name = "NamelessWorker")
      : is_running_(false),
        name_(name) {

  }
  std::atomic<bool> is_running_;
  std::thread thread_;
  std::string name_;
};

/**
 * Thread pool
 * Purpose: Provides a thread pool with basic functionality similar to
 * ThreadPoolExecutor
 * Design: Locked control over a manager thread that controls the worker threads
 */
template<typename T>
class ThreadPool {
 public:

  ThreadPool(int max_worker_threads = 2, bool daemon_threads = false, const std::shared_ptr<core::controller::ControllerServiceProvider> &controller_service_provider = nullptr,
             const std::string &name = "NamelessPool")
      : daemon_threads_(daemon_threads),
        thread_reduction_count_(0),
        max_worker_threads_(max_worker_threads),
        adjust_threads_(false),
        running_(false),
        controller_service_provider_(controller_service_provider),
        name_(name) {
    current_workers_ = 0;
    task_count_ = 0;
    thread_manager_ = nullptr;
  }

  ThreadPool(const ThreadPool<T> &&other)
      : daemon_threads_(std::move(other.daemon_threads_)),
        thread_reduction_count_(0),
        max_worker_threads_(std::move(other.max_worker_threads_)),
        adjust_threads_(false),
        running_(false),
        controller_service_provider_(std::move(other.controller_service_provider_)),
        thread_manager_(std::move(other.thread_manager_)),
        name_(std::move(other.name_)) {
    current_workers_ = 0;
    task_count_ = 0;
  }

  ~ThreadPool() {
    shutdown();
  }

  /**
   * Execute accepts a worker task and returns
   * a future
   * @param task this thread pool will subsume ownership of
   * the worker task
   * @param future future to move new promise to
   * @return true if future can be created and thread pool is in a running state.
   */
  bool execute(Worker<T> &&task, std::future<T> &future);

  /**
   * attempts to stop tasks with the provided identifier.
   * @param identifier for worker tasks. Note that these tasks won't
   * immediately stop.
   */
  void stopTasks(const std::string &identifier);

  /**
   * Returns true if a task is running.
   */
  bool isRunning(const std::string &identifier) {
    return task_status_[identifier] == true;
  }

  std::vector<BackTrace> getTraces() {
    std::vector<BackTrace> traces;
    std::lock_guard<std::recursive_mutex> lock(manager_mutex_);
    std::unique_lock<std::mutex> wlock(worker_queue_mutex_);
    // while we may be checking if running, we don't want to
    // use the threads outside of the manager mutex's lock -- therefore we will
    // obtain a lock so we can keep the threads in memory
    if (running_) {
      for (const auto &worker : thread_queue_) {
        if (worker->is_running_)
          traces.emplace_back(TraceResolver::getResolver().getBackTrace(worker->name_, worker->thread_.native_handle()));
      }
    }
    return traces;
  }

  /**
   * Starts the Thread Pool
   */
  void start();
  /**
   * Shutdown the thread pool and clear any
   * currently running activities
   */
  void shutdown();
  /**
   * Set the max concurrent tasks. When this is done
   * we must start and restart the thread pool if
   * the number of tasks is less than the currently configured number
   */
  void setMaxConcurrentTasks(uint16_t max) {
    std::lock_guard<std::recursive_mutex> lock(manager_mutex_);
    if (running_) {
      shutdown();
    }
    max_worker_threads_ = max;
    if (!running_)
      start();
  }

  ThreadPool<T> operator=(const ThreadPool<T> &other) = delete;
  ThreadPool(const ThreadPool<T> &other) = delete;

  ThreadPool<T> &operator=(ThreadPool<T> &&other) {
    std::lock_guard<std::recursive_mutex> lock(manager_mutex_);
    if (other.running_) {
      other.shutdown();
    }
    if (running_) {
      shutdown();
    }
    max_worker_threads_ = std::move(other.max_worker_threads_);
    daemon_threads_ = std::move(other.daemon_threads_);
    current_workers_ = 0;
    thread_reduction_count_ = 0;

    thread_queue_ = std::move(other.thread_queue_);
    worker_queue_ = std::move(other.worker_queue_);

    controller_service_provider_ = std::move(other.controller_service_provider_);
    thread_manager_ = std::move(other.thread_manager_);

    adjust_threads_ = false;

    if (!running_) {
      start();
    }

    name_ = other.name_;
    return *this;
  }

 protected:

  std::thread createThread(std::function<void()> &&functor) {
    return std::thread([ functor ]() mutable {
      functor();
    });
  }

  /**
   * Drain will notify tasks to stop following notification
   */
  void drain() {
    while (current_workers_ > 0) {
      tasks_available_.notify_one();
    }
  }
// determines if threads are detached
  bool daemon_threads_;
  std::atomic<int> thread_reduction_count_;
// max worker threads
  int max_worker_threads_;
// current worker tasks.
  std::atomic<int> current_workers_;
  std::atomic<int> task_count_;
// thread queue
  std::vector<std::shared_ptr<WorkerThread>> thread_queue_;
// manager thread
  std::thread manager_thread_;
// the thread responsible for putting delayed tasks to the worker queue when they had to be put
  std::thread delayed_scheduler_thread_;
// conditional that's used to adjust the threads
  std::atomic<bool> adjust_threads_;
// atomic running boolean
  std::atomic<bool> running_;
// controller service provider
  std::shared_ptr<core::controller::ControllerServiceProvider> controller_service_provider_;
// integrated power manager
  std::shared_ptr<controllers::ThreadManagementService> thread_manager_;
  // thread queue for the recently deceased threads.
  moodycamel::ConcurrentQueue<std::shared_ptr<WorkerThread>> deceased_thread_queue_;
// worker queue of worker objects
  moodycamel::ConcurrentQueue<Worker<T>> worker_queue_;
  std::priority_queue<Worker<T>, std::vector<Worker<T>>, WorkerComparator<T>> delayed_worker_queue_;
// notification for available work
  std::condition_variable tasks_available_;
// notification for new delayed tasks that's before the current ones
  std::condition_variable delayed_task_available_;
// map to identify if a task should be
  std::map<std::string, bool> task_status_;
// manager mutex
  std::recursive_mutex manager_mutex_;
// work queue mutex
  std::mutex worker_queue_mutex_;
  // thread pool name
  std::string name_;

  /**
   * Call for the manager to start worker threads
   */
  void manageWorkers();

  /**
   * Function to adjust the workers up and down.
   */
  void adjustWorkers(int count);

  /**
   * Runs worker tasks
   */
  void run_tasks(std::shared_ptr<WorkerThread> thread);

  void manage_delayed_queue();
};

} /* namespace utils */
} /* namespace minifi */
} /* namespace nifi */
} /* namespace apache */
} /* namespace org */

#endif
