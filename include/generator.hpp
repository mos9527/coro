#pragma once
#include <exception>
#include <coroutine>
#include <iterator>

namespace coro::details {
template <typename T> struct generator;

struct generator_promise_t {
  std::exception_ptr exception;

  static std::suspend_always initial_suspend() noexcept { return {}; }
  static std::suspend_always final_suspend() noexcept { return {}; }

  static void return_void() noexcept {
  }

  void unhandled_exception() noexcept {
    exception = std::current_exception();
  }
};

template <typename T>
struct generator_promise : generator_promise_t {
  using value_type = T;
  using generator_type = generator<T>;

  generator_type get_return_object() noexcept;

  T current_value;

  std::suspend_always yield_value(T value) {
    current_value = std::move(value);
    return {};
  }
};

template <typename T>
struct generator {
  using promise_type = generator_promise<T>;
  using handle_type = std::coroutine_handle<promise_type>;
  handle_type coroutine = nullptr;

  struct iterator {
    // Forward iteration only
    using iterator_category = std::input_iterator_tag;
    using value_type = std::remove_reference_t<T>;
    using difference_type = void;

    struct iterator_end {
    };

    // Support comparison operators for (auto &&item : generator) semantics
    friend bool operator==(const iterator &it, const iterator_end &) {
      return it.coroutine == nullptr || it.coroutine.done();
    }

    friend bool operator==(const iterator_end &, const iterator &it) {
      return it == iterator_end{};
    }

    friend bool operator!=(const iterator &it, const iterator_end &) {
      return !(it == iterator_end{});
    }

    friend bool operator!=(const iterator_end &, const iterator &it) {
      return !(it == iterator_end{});
    }

    handle_type coroutine;

    explicit iterator(const handle_type coroutine) : coroutine(coroutine) {
    }

    iterator &operator++() {
      coroutine.resume();
      // See also `event_loop::run_frame()`
      if (coroutine.done()) {
        // This in turns makes *this == iterator_end{}
        if (auto exception = coroutine.promise().exception)
          std::rethrow_exception(exception);
      }
      return *this;
    }

    value_type &operator*() {
      return coroutine.promise().current_value;
    }

    value_type *operator->() {
      return &coroutine.promise().current_value;
    }
  };

  explicit generator(handle_type h) : coroutine(h) {
  }

  generator(generator const &) = delete;
  generator(generator &&t) noexcept { std::swap(coroutine, t.coroutine); }

  iterator begin() {
    iterator it = iterator(coroutine);
    ++it; // Move to the first value
    return it;
  }

  static typename iterator::iterator_end end() {
    return {};
  }

  ~generator() {
    if (coroutine)
      coroutine.destroy();
  }
};

template <typename T> typename generator_promise<T>::generator_type
generator_promise<T>::get_return_object() noexcept {
  return generator<T>{
      generator_type::handle_type::from_promise(*this)
  };
}
};

namespace coro {
template <typename T>
using generator = details::generator<T>;
}
