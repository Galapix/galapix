/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2018 Ingo Ruhnke <grumbel@gmail.com>
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

#include "galapix/app.hpp"

#include <assert.h>

namespace galapix {

App g_app;

App::App() :
  m_surface_factory(nullptr),
  m_archive_manager(nullptr)
{
}

surf::SoftwareSurfaceFactory&
App::surface_factory()
{
  assert(m_surface_factory != nullptr);
  return *m_surface_factory;
}

arxp::ArchiveManager&
App::archive()
{
  assert(m_archive_manager != nullptr);
  return *m_archive_manager;
}

} // namespace galapix

/* EOF */