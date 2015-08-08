#include <iostream>

#include "archive/archive_manager.hpp"
#include "util/software_surface_factory.hpp"
#include "util/url.hpp"
#include "plugins/jpeg.hpp"

int main(int argc, char** argv)
{
  ArchiveManager archive_manager;
  SoftwareSurfaceFactory software_surface_factory;

  while(true)
  {
    for(int i = 1; i < argc; ++i)
    {
      //SoftwareSurfacePtr surface = SoftwareSurfaceFactory::current().from_url(URL::from_string(argv[i]));
      SoftwareSurface surface = JPEG::load_from_file(argv[i], 2);

    }
  }

  return 0;
}

/* EOF */
