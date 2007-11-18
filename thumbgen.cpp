// Fast Epeg based Thumbnail generator
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include "epeg/Epeg.h"
#include "md5.hpp"
#include "command_line.hpp"
#include "filesystem.hpp"

bool verbose = false;

void
generate_thumbnails(const std::string& filename, int quality, const std::vector<int>& sizes)
{
  std::string m = MD5::md5_string("file://" + filename);
      
  for(int i = 0; i < int(sizes.size()); ++i)
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
              epeg_quality_set(img, quality);
              epeg_thumbnail_comments_enable(img, 1);

              if (w > h)
                epeg_decode_size_set(img, sizes[i], sizes[i] * h / w);
              else
                epeg_decode_size_set(img, sizes[i] * w / h, sizes[i]);
  
              epeg_decode_colorspace_set(img, EPEG_RGB8);

              if (verbose)
                std::cout << sizes[i] << " - " << filename << " => " << str.str() << std::endl;

              epeg_file_output_set(img, str.str().c_str());
              epeg_encode(img);
            }
      
          epeg_close(img);
        }
    }
}

void
generate_file_list(const std::string& pathname, std::vector<std::string>& file_list)
{
  if (Filesystem::is_directory(pathname))
    {
      std::vector<std::string> dir_list = Filesystem::open_directory(pathname);
      for(std::vector<std::string>::iterator i = dir_list.begin(); i != dir_list.end(); ++i)
        {
          generate_file_list(*i, file_list);
        }
    }
  else
    {
      if (Filesystem::has_extension(pathname, ".jpg")  ||
          Filesystem::has_extension(pathname, ".JPG")  ||
          Filesystem::has_extension(pathname, ".jpe")  ||
          Filesystem::has_extension(pathname, ".JPE")  ||
          Filesystem::has_extension(pathname, ".JPEG") ||
          Filesystem::has_extension(pathname, ".jpeg"))
        {
          file_list.push_back(Filesystem::realpath(pathname));
        }
    }
}

void parse_number_list(const std::string& str, std::vector<int>& sizes)
{
  // FIXME: implement me
  // FIXME: add handling for "16,32,64"

  int begin = 0;
  for(int i = 1; i <= int(str.size()); ++i)
    {
      if (str[i] == ',' || i == int(str.size()))
        {
          if (i - begin > 0)
            {
              int j;
              if (sscanf(str.substr(begin, i - begin).c_str(), "%d", &j) == 1)
                {
                  if (j < 16)
                    throw std::runtime_error("thumb size below 16 not supported");
                  else if (j > 1024)
                    throw std::runtime_error("thumb size larger then 1024 not supported");
                  else if ((j & (j - 1)) != 0)
                    throw std::runtime_error("thumb size must be power of two");
                  else
                    sizes.push_back(j);
                }
              else
                throw std::runtime_error("argument not a number");
            }
          begin = i + 1;
        }
    }
}

int main(int argc, char** argv)
{
  try {
    CommandLine argp;
      
    argp.add_usage("[OPTION]... [FILE]...");
    argp.add_doc("A offline thumbnail generator for griv\n");
    argp.add_option('s', "size", "NUM,...", "Generate thumbnails for the given sizes (default: 16)");
    argp.add_option('a', "all", "", "Generate thumbnails for all sizes");
    argp.add_option('v', "verbose", "", "Print more detailed output on generation");
    argp.add_option('q', "quality", "NUM", "JPEG quality used for thumbnailing, range 1-100 (default: 80)");
    argp.add_option('h', "help", "", "Print this help");

    try {
      argp.parse_args(argc, argv);
    } catch(std::exception& err) {
      std::cout << "Error: CommandLine: " << err.what() << std::endl;
      exit(EXIT_FAILURE);
    }

    int quality = 80;
    std::vector<int> sizes;
    std::vector<std::string> pathnames;
    while(argp.next())
      {
        switch(argp.get_key())
          {
            case 's':
              parse_number_list(argp.get_argument(), sizes);
              break;

            case 'q':
              quality = atoi(argp.get_argument().c_str());
              break;

            case 'v':
              verbose = true;
              break;

            case 'a':
              sizes.push_back(16);
              sizes.push_back(32);
              sizes.push_back(64);
              sizes.push_back(128);
              sizes.push_back(256);
              sizes.push_back(512);
              sizes.push_back(1024);
              break;

            case 'h':
              argp.print_help();
              exit(EXIT_SUCCESS);
              break;

            case CommandLine::REST_ARG:
              pathnames.push_back(argp.get_argument());
              break;

            default:
              std::cout << "Unhandled argument: " << argp.get_key() << std::endl;
              exit(EXIT_FAILURE);
              break;
          }
      };
  
    if (sizes.empty())
      sizes.push_back(16);

    if (pathnames.empty())
      {
        argp.print_help();
      }
    else
      {
        Filesystem::init();

        std::cout << "Generating file list... " << std::flush;
        std::vector<std::string> file_list;
        for(std::vector<std::string>::iterator i = pathnames.begin(); i != pathnames.end(); ++i)
          generate_file_list(*i, file_list);
        std::cout << file_list.size() << " files found" << std::endl;;

        int progress_scale = 70;
        int progress = 0;
        int last_progress = 0;
        std::cout << "Generating thumbnails for " << std::flush;

        for(std::vector<int>::iterator i = sizes.begin(); i != sizes.end(); ++i)
          std::cout << *i << " " << std::flush;

        std::cout << std::endl;

        for(std::vector<std::string>::iterator i = file_list.begin(); i != file_list.end(); ++i)
          {
            try {
              generate_thumbnails(*i, quality, sizes);
            } catch(std::exception& err) {
              std::cout << "Error: " << err.what() << std::endl;
            }

            progress = progress_scale * (i - file_list.begin() + 1) / file_list.size();

            if (progress != last_progress)
              {
                last_progress = progress;

                std::cout << "[";
                for(int i = 0; i < progress_scale; ++i)
                  {
                    if (i <= progress)
                      std::cout.put('#');
                    else
                      std::cout.put(' ');
                  }
                std::cout << "]\r" << std::flush;
              }
          }
        std::cout << "\ndone" << std::endl;;
      }

    Filesystem::deinit();
  } catch(std::exception& err) {
    std::cout << "Error: " << err.what() << std::endl;
  }
  return 0;
}

/* EOF */
