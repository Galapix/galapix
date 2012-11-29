/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_GALAPIX_RESOURCE_NAME_HPP
#define HEADER_GALAPIX_GALAPIX_RESOURCE_NAME_HPP

/** 
    The ResourceName uniquely identifies a resource. Resources stored
    in different places will have the same ResourceName when they
    contain the same content.

    The {handler} part gives the type of resource and {args} allows to
    address subelements of the resource, such as individual pages in
    an PDF.
    
    The ResourceName is represented as string in the form:
    
    "{checksum}//{handler}:{args}" 

    Example:
    
    "sha1:95e309e639e67fa2bb0c0a40532832971309dee9//pdf:5"
*/
class ResourceName
{
private:
public:
  ResourceName();
};

#endif

/* EOF */
