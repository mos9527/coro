#include <generator.hpp>

#include <iostream>

int main() {
  auto gen = []() -> coro::generator<int> {
    for (int i = 0; i < 10; ++i) {
      co_yield i;
    }
  };
  for (auto value : gen()) {
    std::cout << value << " ";
  }
}