#pragma once
#include <queue>
#include "../task.hpp"

namespace coro {
using coro::task;

struct event_loop {
  std::queue<std::coroutine_handle<>> ready_queue;
  std::deque<task<void>> active_tasks; // Owning
  struct yield_awaitable {
    event_loop &loop;

    explicit yield_awaitable(event_loop &loop) : loop(loop) {
    }

    yield_awaitable(yield_awaitable const &) = delete;

    static bool await_ready() noexcept { return false; }

    static void await_resume() noexcept {
    }

    void await_suspend(const std::coroutine_handle<> handle) const {
      if (handle)
        loop.schedule(handle);
    }
  };

  yield_awaitable yield_to_event_loop() { return yield_awaitable{*this}; }

  void push(task<void> task) {
    active_tasks.emplace_back(std::move(task));
    if (active_tasks.back().coroutine)
      schedule(active_tasks.back().coroutine);
  }

  bool run_frame() {
    if (ready_queue.empty())
      return false;
    auto handle = ready_queue.front();
    ready_queue.pop();
    handle.resume();
    // Exception Handling
    // Exceptions are always stored into the Promise of the coroutine
    // and is not thrown in the state machine itself.
    // Checking it can be done at the synchronous points - such as this one.
    if (handle.done()) {
      auto [exception, continuation] = std::coroutine_handle<
            coro::details::task_promise_t>::from_address(handle.address()).
          promise();
      if (exception)
        std::rethrow_exception(exception);
    }
    return true;
  }

  void schedule(const std::coroutine_handle<> handle) {
    if (!handle)
      throw std::runtime_error("attempt to schedule a null handle.");
    if (handle && !handle.done())
      ready_queue.push(handle);
  }
};
}