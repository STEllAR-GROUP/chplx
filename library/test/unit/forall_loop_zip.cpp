//  Copyright (c) 2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <chplx.hpp>

#include <hpx/hpx_main.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <cstddef>
#include <mutex>
#include <set>


bool chplx_fork_join_executor = true;
hpx::execution::experimental::fork_join_executor *exec = nullptr;

template <typename... Rs> void testForallLoopZip(Rs &&...rs) {

  std::size_t count = 0;
  auto zip = chplx::zip(std::forward<Rs>(rs)...);

  std::set<typename decltype(zip)::indexType> values;
  hpx::mutex mtx;

  chplx::forall(zip, [&](auto &&value) {
    std::lock_guard l(mtx);
    ++count;
    auto p = values.insert(std::forward<decltype(value)>(value));
    HPX_TEST(p.second);
  });

  HPX_TEST_EQ(count, static_cast<std::size_t>(zip.size()));
  count = 0;

  for (auto val : zip.these()) {
    ++count;
    HPX_TEST(values.contains(val));
  }

  HPX_TEST_EQ(count, values.size());
}

void testStructuredBindings() {
  chplx::Tuple t(2, 4L, std::string("33"));
  auto [a, b, c] = t;
  HPX_TEST_EQ(a, 2);
  HPX_TEST_EQ(b, 4L);
  HPX_TEST_EQ(c, std::string("33"));
  chplx::Array<std::int64_t, chplx::Domain<1>> B(chplx::Range(0, 5));
  chplx::Array<std::int64_t, chplx::Domain<1>> C(chplx::Range(0, 5));
  chplx::Array<std::int64_t, chplx::Domain<1>> D(chplx::Range(0, 5));
  
  chplx::forall(chplx::Range(0, 5), [&](auto&& i) { B(i) = i; });
  chplx::forall(chplx::Range(0, 5), [&](auto&& i) { C(i) = i; });

  chplx::forall(chplx::zip(B, C, D), [&](auto&& b, auto&& c, auto&& d) {
      d = b + c;
  });
  
  for (std::int64_t i = 0; i < 5; ++i) {
    HPX_TEST_EQ(B(i), i);
    HPX_TEST_EQ(C(i), i);
    HPX_TEST_EQ(D(i), i + i);
  }
}

int main() {

  chplx_fork_join_executor = true;
  exec = new hpx::execution::experimental::fork_join_executor();

  {
    testForallLoopZip(chplx::Range(0, 10));
    testForallLoopZip(chplx::Range(0, 10), chplx::Range(0, 10));
    testForallLoopZip(chplx::Range(0, 10), chplx::Range(0, 10),
                      chplx::Range(0, 10));

    testForallLoopZip(chplx::BoundedRange<int, true>(0, 10, 2),
                      chplx::Range(0, 10));
    testForallLoopZip(chplx::Range(0, 10, chplx::BoundsCategoryType::Open),
                      chplx::Range(0, 10));
    testForallLoopZip(chplx::BoundedRange<int, true>(
                          0, 10, 2, chplx::BoundsCategoryType::Open),
                      chplx::Range(0, 10));

    testForallLoopZip(chplx::Range(1, 0), chplx::Range(0, 10));

    testForallLoopZip(chplx::BoundedRange<int, true>(1, 9, 2),
                      chplx::Range(0, 10));
    testForallLoopZip(chplx::BoundedRange<int, true>(
                          1, 9, 2, chplx::BoundsCategoryType::Open),
                      chplx::Range(0, 10));

    testForallLoopZip(by(chplx::BoundedRange<int, true>(1, 10), -1),
                      chplx::Range(0, 10));
    testForallLoopZip(by(chplx::BoundedRange<int, true>(1, 10), -2),
                      chplx::Range(0, 10));
  }

  delete exec;

  chplx_fork_join_executor = false;

  {
    testForallLoopZip(chplx::Range(0, 10));
    testForallLoopZip(chplx::Range(0, 10), chplx::Range(0, 10));
    testForallLoopZip(chplx::Range(0, 10), chplx::Range(0, 10),
                      chplx::Range(0, 10));

    testForallLoopZip(chplx::BoundedRange<int, true>(0, 10, 2),
                      chplx::Range(0, 10));
    testForallLoopZip(chplx::Range(0, 10, chplx::BoundsCategoryType::Open),
                      chplx::Range(0, 10));
    testForallLoopZip(chplx::BoundedRange<int, true>(
                          0, 10, 2, chplx::BoundsCategoryType::Open),
                      chplx::Range(0, 10));

    testForallLoopZip(chplx::Range(1, 0), chplx::Range(0, 10));

    testForallLoopZip(chplx::BoundedRange<int, true>(1, 9, 2),
                      chplx::Range(0, 10));
    testForallLoopZip(chplx::BoundedRange<int, true>(
                          1, 9, 2, chplx::BoundsCategoryType::Open),
                      chplx::Range(0, 10));

    testForallLoopZip(by(chplx::BoundedRange<int, true>(1, 10), -1),
                      chplx::Range(0, 10));
    testForallLoopZip(by(chplx::BoundedRange<int, true>(1, 10), -2),
                      chplx::Range(0, 10));
  }

  testStructuredBindings();

  return hpx::util::report_errors();
}
