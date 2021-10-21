#include <iostream>

#include <surf/software_surface_factory.hpp>
#include <surf/software_surface.hpp>

#include "archive/archive_manager.hpp"
#include "util/url.hpp"
#include "plugins/jpeg.hpp"

using namespace surf;

int main(int argc, char** argv)
{
  ArchiveManager archive_manager;
  SoftwareSurfaceFactory software_surface_factory;

  while(true)
  {
    for(int i = 1; i < argc; ++i)
    {
      //SoftwareSurfacePtr surface = SoftwareSurfaceFactory::current().from_url(URL::from_string(argv[i]));
      SoftwareSurface surface = jpeg::load_from_file(argv[i], 2);
    }
  }
}

/* EOF */
