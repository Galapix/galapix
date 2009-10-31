#include <iostream>
#include "util/software_surface.hpp"
#include "util/url.hpp"
#include "util/software_surface_factory.hpp"
#include "plugins/png.hpp"
#include "plugins/jpeg.hpp"

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    std::cout << "Usage: " << argv[0] << " FILENAME" << std::endl;
  }
  else
  {
    SoftwareSurfacePtr surface = SoftwareSurfaceFactory::current().from_url(URL::from_filename(argv[1]));
    
    PNG::save(surface,              "/tmp/software_surface_test_original.png");
    PNG::save(surface->clone(),     "/tmp/software_surface_test_clone.png");
    PNG::save(surface->rotate90(),  "/tmp/software_surface_test_rotate90.png");
    PNG::save(surface->rotate180(), "/tmp/software_surface_test_rotate180.png");
    PNG::save(surface->rotate270(), "/tmp/software_surface_test_rotate270.png");
    PNG::save(surface->hflip(),     "/tmp/software_surface_test_hflip.png");
    PNG::save(surface->vflip(),     "/tmp/software_surface_test_vflip.png");
  }

  return 0;
}

/* EOF */
