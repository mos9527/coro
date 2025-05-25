#pragma once
#include <algorithm>
#include <coroutine>
#include <exception>
#include <optional>
#include <stdexcept>

namespace coro::details {
template <typename T> struct task;

struct task_promise_t {
  std::exception_ptr exception;
  std::coroutine_handle<> continuation = nullptr;

  void unhandled_exception() noexcept { exception = std::current_exception(); }
  static std::suspend_always initial_suspend() noexcept { return {}; }

  struct final_awaiter {
    static bool await_ready() noexcept { return false; }

    template <typename Ty>
    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<Ty> h) noexcept {
      // Allows continuation into another coroutine i.e. allowing `co_await task_2()` in task_1
      // This will be immediately resumed by the coroutine state machine
      if (auto &promise = h.promise(); promise.continuation)
        return promise.continuation;
      // Last coroutine in the chain
      return std::noop_coroutine();
    }

    static void await_resume() noexcept {
    }
  };

  static final_awaiter final_suspend() noexcept { return final_awaiter{}; }
};

template <typename T>
struct task_promise : task_promise_t {
  using value_type = T;
  using task_type = task<T>;
  task_type get_return_object() noexcept;
  std::optional<value_type> result;
  void return_value(T &&v) { result.emplace(std::forward<T>(v)); }
};

template <> struct task_promise<void> : task_promise_t {
  using value_type = void;
  using task_type = task<void>;
  task_type get_return_object() noexcept;

  static void return_void() noexcept {
  }
};

template <typename T>
struct task {
  using promise_type = task_promise<T>;
  using handle_type = std::coroutine_handle<promise_type>;
  handle_type coroutine = nullptr;

  explicit task(handle_type h) : coroutine(h) {
  }

  // Move only ctor
  // "Lifetime hell awaits otherwise"
  task(task const &) = delete;
  task(task &&t) noexcept { std::swap(coroutine, t.coroutine); }

  ~task() {
    if (coroutine)
      coroutine.destroy();
  }

  // Task itself is also awaitable
  [[nodiscard]] bool await_ready() const noexcept {
    return !coroutine || coroutine.done();
  }

  // Chain to the innermost coroutine (if there's any)
  std::coroutine_handle<> await_suspend(
      std::coroutine_handle<> awaiting_coroutine) noexcept {
    coroutine.promise().continuation = awaiting_coroutine;
    return coroutine;
  }

  T await_resume() {
    auto &promise = coroutine.promise();
    if (promise.exception)
      std::rethrow_exception(promise.exception);
    if constexpr (std::is_void_v<T>)
      return;
    else {
      if (promise.result.has_value())
        return promise.result.value();
      throw std::runtime_error(
          "task finished without return value or exception");
    }
  }
};

template <typename T> typename task_promise<T>::task_type task_promise<
  T>::get_return_object() noexcept {
  return task_type{std::coroutine_handle<task_promise>::from_promise(*this)};
}

inline task_promise<void>::task_type task_promise<
  void>::get_return_object() noexcept {
  return task_type{std::coroutine_handle<task_promise>::from_promise(*this)};
}
}


namespace coro {
template <typename T>
using task = details::task<T>;

template <typename T>
static T wait_one(task<T> task) {
  auto handle = task.coroutine;
  handle.resume();
  if (handle.done()) {
    auto [exception, continuation] = std::coroutine_handle<
          coro::details::task_promise_t>::from_address(handle.address()).
        promise();
    if (exception)
      std::rethrow_exception(exception);
  }
  if constexpr (std::is_void_v<T>)
    return;
  else {
    if (auto promise = handle.promise(); promise.result.has_value())
      return promise.result.value();
    throw std::runtime_error(
        "task finished without return value or exception");
  }
}
}
