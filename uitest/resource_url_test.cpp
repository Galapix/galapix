#include <iostream>
#include <uitest/uitest.hpp>

#include "resource/resource_url.hpp"

UITEST(ResourceURL, test, "URL...")
{
  for(const auto& arg : rest)
  {
    ResourceURL url = ResourceURL::from_string(arg);
    std::cout << "scheme    : '" << url.get_scheme() << "'\n";
    std::cout << "authority : '" << url.get_authority() << "'\n";
    std::cout << "path      : '" << url.get_path() << "'\n";
  }
}

// ResourceURL::from_string("http://www.example.com");
// ResourceURL::from_string("http://www.example.com:8080");
// ResourceURL::from_string("http://www.example.com:8080/index.html");
// ResourceURL::from_string("file:///home/juser/test.txt");
// ResourceURL::from_string("file://home/juser/test.txt");
// ResourceURL::from_string("file:/home/juser/test.txt");

/* EOF */
