// Fast Epeg based Thumbnail generator
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include "epeg/Epeg.h"
#include <mhash.h>
#include "filesystem.hpp"

int num_sizes = 7;
int sizes[] = { 16, 32, 64, 128, 256, 512, 1024 };

inline std::string md5(const std::string& str)
{
  unsigned char hash[16]; /* enough size for MD5 */
  MHASH td = mhash_init(MHASH_MD5);
  if (td == MHASH_FAILED) 
    throw std::runtime_error("Failed to init MHash");
  
  mhash(td, str.c_str(), str.length());
  
  mhash_deinit(td, hash);

  std::ostringstream out;
  for (int i = 0; i < 16; i++) 
    out << std::setfill('0') << std::setw(2) << std::hex << int(hash[i]);

  return out.str();
}

void generate_thumbnails(const std::string& filename)
{
  std::string m = md5("file://" + filename);
      
  for(int i = 0; i < num_sizes; ++i)
    {
      std::ostringstream str;
      str << Filesystem::get_home() << "/.griv/cache/by_url/"
          << sizes[i] << "/" << m.substr(0,2) << "/" << m.substr(2) << ".jpg";

      if (!Filesystem::exist(str.str()))
        {
          // FIXME: This should happen outside the loop, but that doesn't
          // seem to generate a file, due to the way Epeg behaves
          Epeg_Image* img = epeg_file_open(filename.c_str());
          if (!img)
            {
              throw std::runtime_error("Cannot open " + filename);
            }
  
          int w, h;
          epeg_size_get(img, &w, &h);

          if (w > sizes[i] || h > sizes[i])
            {
              epeg_quality_set(img, 80);
              epeg_thumbnail_comments_enable(img, 1);

              if (w > h)
                epeg_decode_size_set(img, sizes[i], sizes[i] * h / w);
              else
                epeg_decode_size_set(img, sizes[i] * w / h, sizes[i]);
  
              epeg_decode_colorspace_set(img, EPEG_RGB8);

              std::cout << sizes[i] << " - " << filename << " => " << str.str() << std::endl;

              epeg_file_output_set(img, str.str().c_str());
              epeg_encode(img);
            }
      
          epeg_close(img);
        }
    }
}

void process_path(const std::string& pathname)
{
  if (Filesystem::is_directory(pathname))
    {
      std::vector<std::string> dir_list = Filesystem::open_directory(pathname);
      for(std::vector<std::string>::iterator i = dir_list.begin(); i != dir_list.end(); ++i)
        {
          process_path(*i);
        }
    }
  else
    {
      if (Filesystem::has_extension(pathname, ".jpg") ||
          Filesystem::has_extension(pathname, ".jpg"))
        {
          try {
            generate_thumbnails(Filesystem::realpath(pathname));
          } catch(std::exception& err) {
            std::cout << "Error: " << err.what() << std::endl;
          }          
        }
    }
}

int main(int argc, char** argv)
{
  Filesystem::init();

  if (argc == 1)
    {
      std::cout << "Usage: " << argv[0] << " [FILE]..." << std::endl;
    }
  else
    {
      for(int i = 1; i < argc; ++i)
        {
          process_path(argv[i]);
        }
    }

  Filesystem::deinit();

  return 0;
}

/* EOF */
