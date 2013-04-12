#include <iostream>
#include <gtest/gtest.h>

#include "database/database.hpp"

TEST(DatabaseTest, check_cleanup) {
  Database db("/tmp/database_test");
  db.cleanup();
}

TEST(DatabaseTest, check_get) {
  Database db("/tmp/database_test");

  db.get_resources();
  db.get_tiles();

  db.cleanup();
}

/* EOF */
