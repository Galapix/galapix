#include "filesystem.hpp"
#include "md5.hpp"
#include "file_entry_cache.hpp"
#include <iostream>

int main(int argc, char** argv)
{
  FileEntryCache cache("/tmp/test.cache");

  std::cout << "Generating file list... " << std::flush;
  std::vector<std::string> file_list;
  for(int i = 1; i < argc; ++i)
    {
      Filesystem::generate_jpeg_file_list(argv[i], file_list);
    }
  std::cout << file_list.size() << " done"  << std::endl;

  for(std::vector<std::string>::iterator i = file_list.begin(); i != file_list.end(); ++i)
    {
      const FileEntry* entry = cache.get_entry(*i);
      if (!entry)
        {
          std::cout << "Couldn't get entry for: " << *i << std::endl;
        }
      else
        {
          std::cout << *i << std::endl;
          std::cout << "  " << entry->url_md5 << std::endl;
          std::cout << "  " << entry->mtime << std::endl;
          std::cout << "  " << entry->width << "x" << entry->height << std::endl;
          std::cout << std::endl;
        }
    }

  std::cout << "Saving cache" << std::endl;
  cache.save("/tmp/test.cache");

  return 0;
}

/* EOF */
