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

#ifndef NIFI_MINIFI_CPP_CALLBACKTIMER_H
#define NIFI_MINIFI_CPP_CALLBACKTIMER_H

#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <functional>

namespace org {
namespace apache {
namespace nifi {
namespace minifi {
namespace utils {

class CallBackTimer
{
 public:
  CallBackTimer(std::chrono::milliseconds interval);

  ~CallBackTimer();

  void stop();

  void start(const std::function<void(void)>& func);

  bool is_running() const;

private:
  void stop_inner(std::unique_lock<std::mutex>& lk);

  bool execute_;
  std::thread thd_;
  mutable std::mutex mtx_;
  std::condition_variable cv_;

  const std::chrono::milliseconds interval_;
};

} /* namespace utils */
} /* namespace minifi */
} /* namespace nifi */
} /* namespace apache */
} /* namespace org */

#endif //NIFI_MINIFI_CPP_CALLBACKTIMER_H

