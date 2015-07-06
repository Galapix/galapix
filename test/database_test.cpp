#include <iostream>
#include <gtest/gtest.h>

#include "database/database.hpp"

class DatabaseTest : public testing::Test
{
protected:
  DatabaseTest() :
    m_db(Database::create("/tmp/database_test"))
  {}

  Database m_db;
};

TEST_F(DatabaseTest, check_cleanup)
{
  m_db.cleanup();
}

TEST_F(DatabaseTest, check_get)
{
  m_db.get_resources();
  m_db.get_tiles();

  m_db.cleanup();
}

/* EOF */
