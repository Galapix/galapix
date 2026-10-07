// babyxml - tiny parser for XML-like dialect
// Copyright (C) 2009-2020 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <assert.h>
#include <iostream>

#include "babyxml.hpp"

int main(int argc, char** argv)
{
  if (argc == 1) {
    std::cout << "Usage: " << argv[0] << " STRING" << std::endl;
    return 0;
  }

  for(int i = 1; i < argc; ++i)
  {
    // "<strong size='10'>Hello &lt; World&gt; <b>&quot;blabla&quot;</b> Blablub</strong>"
    for(auto const& node : babyxml::parse(argv[i]))
    {
      if (node.type == babyxml::NodeType::START_TAG) {
        std::cout << "STARTTAG: ";
      } else if (node.type == babyxml::NodeType::END_TAG) {
        std::cout << "ENDTAG: ";
      } else if (node.type == babyxml::NodeType::TEXT) {
        std::cout << "TEXT: ";
      }

      std::cout << "'" << node.content << "'";

      for(auto const& attrib : node.attributes)
      {
        std::cout << " " << attrib.name << "='" << attrib.value << "'";
      }

      std::cout << std::endl;
    }
  }
}

/* EOF */
