#include <iostream>

#include "resource/resource_locator.hpp"

int main(int argc, char** argv)
{
  if (argc != 1)
  {
    for(int i = 1; i < argc; ++i)
    {
      try
      {
        ResourceLocator loc = ResourceLocator::from_string(argv[i]);
        std::cout << "locator       : " << loc.str() << '\n';
        std::cout << "url.scheme    : '" << loc.get_url().get_scheme() << "'\n";
        std::cout << "url.authority : '" << loc.get_url().get_authority() << "'\n";
        std::cout << "url.path      : '" << loc.get_url().get_path() << "'\n";
        for(const auto& handler : loc.get_handler())
        {
          std::cout << "handler: " << handler.get_type() << " - " << handler.get_name();
          if (!handler.get_args().empty())
          {
            std::cout << " : " << handler.get_args();
          }
          std::cout << std::endl;
        }
      }
      catch(const std::exception& err)
      {
        std::cout << "error: " << err.what() << std::endl;
      }
    }
  }
  else
  {
    ResourceURL::from_string("http://www.example.com");
    ResourceURL::from_string("http://www.example.com:8080");
    ResourceURL::from_string("http://www.example.com:8080/index.html");
    ResourceURL::from_string("file:///home/juser/test.txt");
    ResourceURL::from_string("file://home/juser/test.txt");
    ResourceURL::from_string("file:/home/juser/test.txt");
  }
}

/* EOF */
