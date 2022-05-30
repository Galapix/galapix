#include <iostream>
#include <uitest/uitest.hpp>

#include "resource/resource_locator.hpp"

using namespace galapix;

UITEST_S(ResourceLocator, test, "TEXT")
{
  try
  {
    ResourceLocator loc = ResourceLocator::from_string(arg);
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

  //ResourceURL::from_string("http://www.example.com");
  //ResourceURL::from_string("http://www.example.com:8080");
  //ResourceURL::from_string("http://www.example.com:8080/index.html");
  //ResourceURL::from_string("file:///home/juser/test.txt");
  //ResourceURL::from_string("file://home/juser/test.txt");
  //ResourceURL::from_string("file:/home/juser/test.txt");
}

/* EOF */
