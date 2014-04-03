/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HEADER_GALAPIX_UTIL_CURRENTON_HPP
#define HEADER_GALAPIX_UTIL_CURRENTON_HPP

#include <assert.h>

/** A class derived from 'Currenton' is allowed to have only one
    instance of itself at a time and a reference to that instance is
    provided via the current() function. The difference to a Singleton
    is that the object creation and destruction isn't done by the
    Currenton, the Currenton only keeps a reference. So normal RAII
    practices can be used for the object construction. */
template<class C>
class Currenton
{
private:
  static C* s_current;

protected:
  Currenton()  { assert(!s_current); s_current = static_cast<C*>(this); }
  virtual ~Currenton() { s_current = 0; }

public:
  static C& current() { assert(s_current); return *s_current; }
};

template<class C> C* Currenton<C>::s_current = 0;

#endif

/* EOF */
