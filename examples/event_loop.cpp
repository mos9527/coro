#include <iostream>
#include <utils/event_loop.hpp>

int main() {
  coro::event_loop loop;
  auto task1 = []() -> coro::task<void> {
    std::cout << "Entering task 1" << std::endl;
    co_return;
  };
  auto task_exec = []() -> coro::task<void> {
    throw std::runtime_error("Quitting.");
    co_return;
  };
  auto task2 = [&]() -> coro::task<void> {
    co_await task1();
    // Reschedules the coroutine to the event loop's ready queue
    co_await loop.yield_to_event_loop();
    std::cout << "Entering task 2" << std::endl;
    co_await loop.yield_to_event_loop();
    co_await task_exec();
    co_return;
  };
  loop.push(task2());
  for (int frame = 0;;) {
    std::cout << "* Frame=" << frame++ << std::endl;
    try {
      if (!loop.run_frame()) {
        std::cout << "No more tasks to run." << std::endl;
        break;
      }
    } catch (std::exception &exc) {
      std::cout << "! Exception: " << exc.what() << std::endl;
    }
  };
}