#include <gtest/gtest.h>

#include "database/resource_database.hpp"

class ResourceDatabaseTest : public testing::Test
{
protected:
  ResourceDatabaseTest() :
    m_db(""),
    m_resource(m_db)
  {}

  SQLiteConnection m_db;
  ResourceDatabase m_resource;
};

TEST_F(ResourceDatabaseTest, get_file_info)
{
  FileInfo file_info("/tmp/test.txt", 12345, SHA1::from_string("0a4d55a8d778e5022fab701977c5d840bbc486d0"), 11);
  auto row_id = m_resource.store_file_info(file_info);
  ASSERT_TRUE(static_cast<bool>(row_id));

  auto result = m_resource.get_file_info(file_info.get_path());
  ASSERT_TRUE(result);
  EXPECT_EQ(file_info.get_path(), result->get_path());
  EXPECT_EQ(file_info.get_mtime(), result->get_mtime());
}

TEST_F(ResourceDatabaseTest, get_resource_info)
{
  //m_res.get_resource_info();
}

/* EOF */
