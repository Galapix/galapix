#include <iostream>
#include <uitest/uitest.hpp>

#include "plugins/jpeg.hpp"
#include "plugins/png.hpp"
#include "surface/software_surface.hpp"
#include "surface/software_surface_factory.hpp"
#include "util/url.hpp"

UITEST(SoftwareSurface, test, "FILENAME",
       "Load FILENAME and perform actions on it")
{
  SoftwareSurface surface = SoftwareSurfaceFactory::current().from_url(URL::from_filename(args[0]));

  PNG::save(surface,             "/tmp/software_surface_test_original.png");
  PNG::save(surface.rotate90(),  "/tmp/software_surface_test_rotate90.png");
  PNG::save(surface.rotate180(), "/tmp/software_surface_test_rotate180.png");
  PNG::save(surface.rotate270(), "/tmp/software_surface_test_rotate270.png");
  PNG::save(surface.hflip(),     "/tmp/software_surface_test_hflip.png");
  PNG::save(surface.vflip(),     "/tmp/software_surface_test_vflip.png");
}

/* EOF */
