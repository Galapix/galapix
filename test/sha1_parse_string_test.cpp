#include <assert.h>
#include <iostream>

#include "util/sha1.hpp"

int main(int argc, char** argv)
{
  for(int i = 1; i < argc; ++i)
  {
    std::string in  = argv[i];
    std::string out = SHA1::parse_string(in).str();
    std::cout <<  in << "  " << out << std::endl;
    assert(in == out);
  }

  return 0;
}
