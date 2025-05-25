#include <task.hpp>
#include <iostream>
#include <thread>

coro::task<int> task_2() {
  std::cout << "Entering task 2" << std::endl;
  co_return 39;
}

coro::task<void> task_1() {
  std::cout << "Entering task 1" << std::endl;
  int result = co_await task_2();
  std::cout << "Task 1 received result: " << result << std::endl;
  co_return;
}

int main() {
  // Start the coroutine and wait for it to complete
  coro::wait_one(task_1());
}
