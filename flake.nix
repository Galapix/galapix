{
  description = "An image viewer for large image collections";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.05";
    flake-utils.url = "github:numtide/flake-utils";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";
    tinycmmc.inputs.flake-utils.follows = "flake-utils";

    geomcpp.url = "github:grumbel/geomcpp";
    geomcpp.inputs.nixpkgs.follows = "nixpkgs";
    geomcpp.inputs.flake-utils.follows = "flake-utils";
    geomcpp.inputs.tinycmmc.follows = "tinycmmc";

    exspcpp.url = "github:grumbel/exspcpp";
    exspcpp.inputs.nixpkgs.follows = "nixpkgs";
    exspcpp.inputs.flake-utils.follows = "flake-utils";
    exspcpp.inputs.tinycmmc.follows = "tinycmmc";
    exspcpp.inputs.logmich.follows = "logmich";
    exspcpp.inputs.uitest.follows = "uitest";

    arxpcpp.url = "github:grumbel/arxpcpp";
    arxpcpp.inputs.nixpkgs.follows = "nixpkgs";
    arxpcpp.inputs.flake-utils.follows = "flake-utils";
    arxpcpp.inputs.tinycmmc.follows = "tinycmmc";
    arxpcpp.inputs.logmich.follows = "logmich";
    arxpcpp.inputs.exspcpp.follows = "exspcpp";
    arxpcpp.inputs.uitest.follows = "uitest";

    logmich.url = "github:logmich/logmich";
    logmich.inputs.nixpkgs.follows = "nixpkgs";
    logmich.inputs.flake-utils.follows = "flake-utils";
    logmich.inputs.tinycmmc.follows = "tinycmmc";

    priocpp.url = "github:grumbel/priocpp";
    priocpp.inputs.nixpkgs.follows = "nixpkgs";
    priocpp.inputs.flake-utils.follows = "flake-utils";
    priocpp.inputs.tinycmmc.follows = "tinycmmc";
    priocpp.inputs.logmich.follows = "logmich";
    priocpp.inputs.sexpcpp.follows = "sexpcpp";

    sexpcpp.url = "github:lispparser/sexp-cpp";
    sexpcpp.inputs.nixpkgs.follows = "nixpkgs";
    sexpcpp.inputs.flake-utils.follows = "flake-utils";
    sexpcpp.inputs.tinycmmc.follows = "tinycmmc";

    strutcpp.url = "github:grumbel/strutcpp";
    strutcpp.inputs.nixpkgs.follows = "nixpkgs";
    strutcpp.inputs.flake-utils.follows = "flake-utils";
    strutcpp.inputs.tinycmmc.follows = "tinycmmc";
    strutcpp.inputs.geomcpp.follows = "geomcpp";
    strutcpp.inputs.logmich.follows = "logmich";

    surfcpp.url = "github:grumbel/surfcpp";
    surfcpp.inputs.nixpkgs.follows = "nixpkgs";
    surfcpp.inputs.flake-utils.follows = "flake-utils";
    surfcpp.inputs.tinycmmc.follows = "tinycmmc";
    surfcpp.inputs.geomcpp.follows = "geomcpp";
    surfcpp.inputs.logmich.follows = "logmich";

    uitest.url = "github:grumbel/uitest";
    uitest.inputs.nixpkgs.follows = "nixpkgs";
    uitest.inputs.flake-utils.follows = "flake-utils";
    uitest.inputs.tinycmmc.follows = "tinycmmc";

    babyxml.url = "github:grumbel/babyxml";
    babyxml.inputs.nixpkgs.follows = "nixpkgs";
    babyxml.inputs.flake-utils.follows = "flake-utils";
    babyxml.inputs.tinycmmc.follows = "tinycmmc";

    wstdisplay.url = "github:WindstilleTeam/wstdisplay";
    wstdisplay.inputs.nixpkgs.follows = "nixpkgs";
    wstdisplay.inputs.flake-utils.follows = "flake-utils";
    wstdisplay.inputs.tinycmmc.follows = "tinycmmc";
    wstdisplay.inputs.geomcpp.follows = "geomcpp";
    wstdisplay.inputs.babyxml.follows = "babyxml";
    wstdisplay.inputs.surfcpp.follows = "surfcpp";
    wstdisplay.inputs.logmich.follows = "logmich";
  };

  outputs = { self, nixpkgs, flake-utils,
              tinycmmc, exspcpp, arxpcpp, geomcpp, logmich, priocpp, sexpcpp, strutcpp, surfcpp, uitest, babyxml, wstdisplay }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in rec {
        packages = flake-utils.lib.flattenTree rec {
          galapix = pkgs.stdenv.mkDerivation {
            pname = "galapix";
            version = "0.3.0";
            meta = {
              mainProgram = "galapix-0.3.sdl";
            };
            src = nixpkgs.lib.cleanSource ./.;
            enableParallelBuilding = true;
            doCheck = false;
            cmakeFlags = [
              "-DBUILD_GALAPIX_SDL=ON"
              "-DBUILD_GALAPIX_GTK=ON"
              # "-DBUILD_TESTS=ON"
              "-DBUILD_BENCHMARKS=ON"
              "-DBUILD_EXTRAS=ON"
            ];
            postFixup = ''
                wrapProgram $out/bin/galapix-0.3.sdl \
                  --prefix LIBGL_DRIVERS_PATH ":" "${pkgs.mesa.drivers}/lib/dri" \
                  --prefix LD_LIBRARY_PATH ":" "${pkgs.mesa.drivers}/lib"
                wrapProgram $out/bin/galapix-0.3.gtk \
                  --prefix LIBGL_DRIVERS_PATH ":" "${pkgs.mesa.drivers}/lib/dri" \
                  --prefix LD_LIBRARY_PATH ":" "${pkgs.mesa.drivers}/lib"
            '';
            nativeBuildInputs = [
              pkgs.cmake
              pkgs.pkgconfig
              pkgs.makeWrapper
            ];
            buildInputs = [
              tinycmmc.defaultPackage.${system}
              logmich.defaultPackage.${system}
              arxpcpp.defaultPackage.${system}
              geomcpp.defaultPackage.${system}
              priocpp.defaultPackage.${system}
              surfcpp.defaultPackage.${system}
              babyxml.defaultPackage.${system}
              sexpcpp.defaultPackage.${system}
              wstdisplay.defaultPackage.${system}
              uitest.defaultPackage.${system}
              strutcpp.defaultPackage.${system}

              pkgs.entt
              pkgs.sqlitecpp
              pkgs.gbenchmark
              pkgs.fmt
              pkgs.glm
              pkgs.gtest
              pkgs.gtkmm3
              pkgs.jsoncpp
              pkgs.libexif
              pkgs.libjpeg
              pkgs.libpng
              pkgs.libsigcxx
              pkgs.mesa
              pkgs.openssl
              pkgs.python3

              pkgs.SDL2
              pkgs.SDL2_image
              pkgs.boost
              pkgs.curl
              pkgs.glew
              pkgs.imagemagick6
              pkgs.libGL
              pkgs.libGLU
              pkgs.libexif
              pkgs.libmhash
              pkgs.libspnav
              pkgs.sqlite
              pkgs.

              # Silence pkg-config warnings
              pkgs.pcre
              pkgs.util-linux
              pkgs.libselinux
              pkgs.libsepol
              pkgs.libunwind
              pkgs.elfutils
              pkgs.zstd
              pkgs.orc
              pkgs.libthai
              pkgs.libdatrie
              pkgs.libxkbcommon
              pkgs.xorg.libXdmcp
              pkgs.epoxy
              pkgs.dbus-glib
              pkgs.at-spi2-core
              pkgs.xorg.libXtst
            ];
           };
        };
        defaultPackage = packages.galapix;
      });
}
