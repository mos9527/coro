#include <iostream>
#include <utils/event_loop.hpp>

int main() {
  coro::event_loop loop;
  auto run_frames = [&]() {
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
  };
  ///
  auto task1 = []() -> coro::task<void> {
    std::cout << "Entering task 1" << std::endl;
    co_return;
  };
  auto task_exec = []() -> coro::task<void> {
    throw std::runtime_error("Quitting.");
    co_return;
  };
  auto task2 = [&](bool raise_exeception) -> coro::task<void> {
    co_await task1();
    // Reschedules the coroutine to the event loop's ready queue
    co_await loop.yield_to_event_loop();
    std::cout << "Entering task 2" << std::endl;
    co_await loop.yield_to_event_loop();
    if (raise_exeception)
      task_exec();
    co_return;
  };
  loop.push(task2(true));
  run_frames();
  std::cout << "Remaining tasks: " << loop.active_tasks.size() << std::endl;
  loop.reset();
  std::cout << "* Running without exceptions" << std::endl;
  loop.push(task2(false));
  run_frames();
  std::cout << "Remaining tasks: " << loop.active_tasks.size() << std::endl;
}