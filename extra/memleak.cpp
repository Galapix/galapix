#include <iostream>

#include <surf/software_surface_factory.hpp>
#include <surf/software_surface.hpp>

#include <arxp/archive_manager.hpp>
#include <surf/plugins/jpeg.hpp>

#include "util/url.hpp"

int main(int argc, char** argv)
{
  arxp::ArchiveManager archive_manager;
  surf::SoftwareSurfaceFactory software_surface_factory;

  while(true)
  {
    for(int i = 1; i < argc; ++i)
    {
      //SoftwareSurfacePtr surface = SoftwareSurfaceFactory::current().from_url(URL::from_string(argv[i]));
      surf::SoftwareSurface surface = surf::jpeg::load_from_file(argv[i], 2);
    }
  }
}

/* EOF */
