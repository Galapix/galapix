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

TEST_F(ResourceDatabaseTest, file_info)
{
  FileInfo file_info("/tmp/test.txt", 12345, SHA1::from_string("0a4d55a8d778e5022fab701977c5d840bbc486d0"), 11);
  auto row_id = m_resource.store_file_info(file_info);
  ASSERT_TRUE(static_cast<bool>(row_id));

  auto result = m_resource.get_file_info(file_info.get_path());
  ASSERT_TRUE(result);
  EXPECT_EQ(file_info.get_path(), result->get_path());
  EXPECT_EQ(file_info.get_mtime(), result->get_mtime());
  EXPECT_EQ(file_info.get_blob_info().get_sha1(), result->get_blob_info().get_sha1());
  EXPECT_EQ(file_info.get_blob_info().get_size(), result->get_blob_info().get_size());
}

TEST_F(ResourceDatabaseTest, url_info)
{
  auto blob_info = BlobInfo(SHA1::from_string("0a4d55a8d778e5022fab701977c5d840bbc486d0"), 11);
  auto url = std::string("http://www.example.com");
  auto url_info = URLInfo(url, 34234234, "image/png", blob_info);

  RowId row_id = m_resource.store_url_info(url_info);
  ASSERT_TRUE(static_cast<bool>(row_id));

  auto result = m_resource.get_url_info(url);
  ASSERT_TRUE(result);
  EXPECT_EQ(url_info.get_url(), result->get_url());
  EXPECT_EQ(url_info.get_mtime(), result->get_mtime());
  EXPECT_EQ(url_info.get_content_type(), result->get_content_type());
  EXPECT_EQ(url_info.get_blob_info().get_sha1(), result->get_blob_info().get_sha1());
  EXPECT_EQ(url_info.get_blob_info().get_size(), result->get_blob_info().get_size());
}

TEST_F(ResourceDatabaseTest, resource_info)
{
  auto blob_info = BlobInfo(RowId(1), SHA1::from_string("0a4d55a8d778e5022fab701977c5d840bbc486d0"), 11);
  auto handler = ResourceHandler("image", "png");
  auto name = ResourceName(blob_info, handler);
  auto res_info = ResourceInfo(name, ResourceStatus::Unknown);

  RowId row_id = m_resource.store_resource_info(res_info);
  ASSERT_TRUE(static_cast<bool>(row_id));

  auto result = m_resource.get_resource_info(blob_info);
  ASSERT_TRUE(result);
  EXPECT_EQ(res_info.get_type(), result->get_type());
  EXPECT_EQ(res_info.get_source_type(), result->get_source_type());
  //ASSERT_EQ(res_info.get_name(), result->get_name());
  //ASSERT_EQ(res_info.get_locator(), result->get_locator());
}

TEST_F(ResourceDatabaseTest, image_info)
{
  auto resource_id = RowId(1);

  ImageInfo image_info(RowId(), resource_id, 1024, 768);

  RowId row_id = m_resource.store_image_info(image_info);
  ASSERT_TRUE(static_cast<bool>(row_id));

  auto resource_info = ResourceInfo(row_id, ResourceInfo());

  auto result = m_resource.get_image_info(resource_info);
  ASSERT_TRUE(result);
  EXPECT_EQ(image_info.get_width(), result->get_width());
  EXPECT_EQ(image_info.get_height(), result->get_height());
}

/* EOF */
