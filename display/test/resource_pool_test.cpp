// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "resource_pool.hpp"

using namespace wstdisplay;

namespace {

struct Thing
{
  int value;
};

} // namespace

TEST(ResourcePoolTest, insert_and_find)
{
  ResourcePool<Thing> pool;
  Handle<Thing> const a = pool.insert(std::make_unique<Thing>(Thing{1}));
  Handle<Thing> const b = pool.insert(std::make_unique<Thing>(Thing{2}));

  EXPECT_TRUE(a);
  EXPECT_NE(a, b);
  EXPECT_EQ(pool.find(a)->value, 1);
  EXPECT_EQ(pool.find(b)->value, 2);
  EXPECT_EQ(pool.find(Handle<Thing>()), nullptr);
  EXPECT_EQ(pool.count(), 2u);
}

TEST(ResourcePoolTest, destroy_is_deferred)
{
  ResourcePool<Thing> pool;
  Handle<Thing> const a = pool.insert(std::make_unique<Thing>(Thing{1}));

  pool.destroy(a);
  pool.destroy(a);
  EXPECT_NE(pool.find(a), nullptr);

  EXPECT_TRUE(pool.collect());
  EXPECT_EQ(pool.find(a), nullptr);
  EXPECT_EQ(pool.count(), 0u);
  EXPECT_FALSE(pool.collect());
}

TEST(ResourcePoolTest, stale_handle_doesnt_reach_reused_slot)
{
  ResourcePool<Thing> pool;
  Handle<Thing> const a = pool.insert(std::make_unique<Thing>(Thing{1}));
  pool.destroy(a);
  pool.collect();

  Handle<Thing> const b = pool.insert(std::make_unique<Thing>(Thing{2}));
  EXPECT_EQ(b.index, a.index);
  EXPECT_NE(b.generation, a.generation);
  EXPECT_EQ(pool.find(a), nullptr);
  EXPECT_EQ(pool.find(b)->value, 2);

  // destroying through the stale handle must not hit the new object
  pool.destroy(a);
  pool.collect();
  EXPECT_EQ(pool.find(b)->value, 2);
}

TEST(ResourcePoolTest, destroy_all_makes_handles_stale)
{
  ResourcePool<Thing> pool;
  Handle<Thing> const a = pool.insert(std::make_unique<Thing>(Thing{1}));
  Handle<Thing> const b = pool.insert(std::make_unique<Thing>(Thing{2}));

  pool.destroy_all();
  EXPECT_TRUE(pool.collect());
  EXPECT_EQ(pool.count(), 0u);

  Handle<Thing> const c = pool.insert(std::make_unique<Thing>(Thing{3}));
  EXPECT_EQ(pool.find(a), nullptr);
  EXPECT_EQ(pool.find(b), nullptr);
  EXPECT_EQ(pool.find(c)->value, 3);
}

TEST(ResourcePoolTest, handles_are_small_values)
{
  static_assert(sizeof(Handle<Thing>) == 8);
  static_assert(std::is_trivially_copyable_v<Handle<Thing>>);
  EXPECT_FALSE(Handle<Thing>());
}

/* EOF */
