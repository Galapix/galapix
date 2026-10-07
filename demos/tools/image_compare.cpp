// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Compare two images, fails if they differ in size or if the mean
// absolute difference per channel exceeds the threshold (0-255).
//
//   wst-image-compare A.png B.png [THRESHOLD]

#include <cmath>
#include <cstdlib>
#include <format>
#include <iostream>
#include <string>

#include <surf/software_surface.hpp>

int main(int argc, char** argv)
{
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " A.png B.png [THRESHOLD]\n";
    return EXIT_FAILURE;
  }

  try {
    surf::SoftwareSurface const a = surf::SoftwareSurface::from_file(argv[1]);
    surf::SoftwareSurface const b = surf::SoftwareSurface::from_file(argv[2]);
    double const threshold = argc > 3 ? std::stod(argv[3]) : 1.0;

    if (a.get_size() != b.get_size()) {
      std::cerr << "size mismatch\n";
      return EXIT_FAILURE;
    }

    double total = 0.0;
    double max_diff = 0.0;
    for (int y = 0; y < a.get_height(); ++y) {
      for (int x = 0; x < a.get_width(); ++x) {
        surf::Color const ca = a.get_pixel(geom::ipoint(x, y));
        surf::Color const cb = b.get_pixel(geom::ipoint(x, y));
        double const d = (std::abs(ca.r - cb.r) + std::abs(ca.g - cb.g) + std::abs(ca.b - cb.b)) / 3.0 * 255.0;
        total += d;
        max_diff = std::max(max_diff, d);
      }
    }

    double const mean = total / (static_cast<double>(a.get_width()) * a.get_height());
    std::cout << std::format("mean difference {:.3f}, max difference {:.1f}\n", mean, max_diff);
    return mean <= threshold ? EXIT_SUCCESS : EXIT_FAILURE;
  } catch (std::exception const& err) {
    std::cerr << "error: " << err.what() << "\n";
    return EXIT_FAILURE;
  }
}

/* EOF */
