#include <iostream>

#include "resource/resource_url.hpp"

int main(int argc, char** argv)
{
  if (argc != 1)
  {
    for(int i = 1; i < argc; ++i)
    {
      ResourceURL url = ResourceURL::from_string(argv[i]);
      std::cout << "scheme    : '" << url.get_scheme() << "'\n";
      std::cout << "authority : '" << url.get_authority() << "'\n";
      std::cout << "path      : '" << url.get_path() << "'\n";
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
