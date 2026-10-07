// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "signal.hpp"

using namespace wstsystem;

TEST(SignalTest, calls_slots_in_order)
{
  Signal<void (int)> sig;
  std::vector<int> calls;
  sig.connect([&](int v) { calls.push_back(v); });
  sig.connect([&](int v) { calls.push_back(v * 10); });

  sig(3);
  EXPECT_EQ(calls, (std::vector<int>{3, 30}));
}

TEST(SignalTest, disconnect)
{
  Signal<void ()> sig;
  int count = 0;
  Connection connection = sig.connect([&] { count += 1; });
  sig();
  connection.disconnect();
  sig();
  EXPECT_EQ(count, 1);
  EXPECT_TRUE(sig.empty());
}

TEST(SignalTest, changes_during_emission_apply_to_the_next_one)
{
  Signal<void ()> sig;
  int first = 0;
  int second = 0;
  Connection self;
  self = sig.connect([&] {
    first += 1;
    self.disconnect();
    sig.connect([&] { second += 1; });
  });

  sig();
  EXPECT_EQ(first, 1);
  EXPECT_EQ(second, 0);

  sig();
  EXPECT_EQ(first, 1);
  EXPECT_EQ(second, 1);
}

TEST(SignalTest, connection_outlives_signal)
{
  Connection connection;
  {
    Signal<void ()> sig;
    connection = sig.connect([] {});
  }
  connection.disconnect();
}

/* EOF */
