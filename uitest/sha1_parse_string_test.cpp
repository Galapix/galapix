#include <assert.h>
#include <iostream>
#include <uitest/uitest.hpp>

#include "util/sha1.hpp"

UITEST(SHA1, parse_string, "TEXT...")
{
  for(const auto& arg : args)
  {
    std::string in  = arg;
    std::string out = SHA1::from_string(in).str();
    std::cout <<  in << "  " << out << std::endl;
    assert(in == out);
  }
}

/* EOF */
