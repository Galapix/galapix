{
  description = "An image viewer for large image collections";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.11";
    flake-utils.url = "github:numtide/flake-utils";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";
    tinycmmc.inputs.flake-utils.follows = "flake-utils";

    geomcpp.url = "github:grumbel/geomcpp";
    geomcpp.inputs.nixpkgs.follows = "nixpkgs";
    geomcpp.inputs.tinycmmc.follows = "tinycmmc";

    exspcpp.url = "github:grumbel/exspcpp";
    exspcpp.inputs.nixpkgs.follows = "nixpkgs";
    exspcpp.inputs.tinycmmc.follows = "tinycmmc";
    exspcpp.inputs.flake-utils.follows = "flake-utils";

    arxpcpp.url = "github:grumbel/arxpcpp";
    arxpcpp.inputs.nixpkgs.follows = "nixpkgs";
    arxpcpp.inputs.flake-utils.follows = "flake-utils";
    arxpcpp.inputs.tinycmmc.follows = "tinycmmc";
    arxpcpp.inputs.logmich.follows = "logmich";
    arxpcpp.inputs.exspcpp.follows = "exspcpp";
    arxpcpp.inputs.uitest.follows = "uitest";

    logmich.url = "github:logmich/logmich";
    logmich.inputs.nixpkgs.follows = "nixpkgs";
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
    strutcpp.inputs.tinycmmc.follows = "tinycmmc";

    surfcpp.url = "github:grumbel/surfcpp";
    surfcpp.inputs.nixpkgs.follows = "nixpkgs";
    surfcpp.inputs.tinycmmc.follows = "tinycmmc";
    surfcpp.inputs.geomcpp.follows = "geomcpp";
    surfcpp.inputs.logmich.follows = "logmich";

    uitest.url = "github:grumbel/uitest";
    uitest.inputs.nixpkgs.follows = "nixpkgs";
    uitest.inputs.flake-utils.follows = "flake-utils";
    uitest.inputs.tinycmmc.follows = "tinycmmc";

    babyxml.url = "github:grumbel/babyxml";
    babyxml.inputs.nixpkgs.follows = "nixpkgs";
    babyxml.inputs.tinycmmc.follows = "tinycmmc";

    wstdisplay.url = "github:WindstilleTeam/wstdisplay";
    wstdisplay.inputs.nixpkgs.follows = "nixpkgs";
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
        packages = rec {
          default = galapix;

          galapix = pkgs.gcc12Stdenv.mkDerivation {
            pname = "galapix";
            version = tinycmmc.lib.versionFromFile self;

            src = nixpkgs.lib.cleanSource ./.;

            postPatch = ''
              echo "v${tinycmmc.lib.versionFromFile self}" > VERSION
            '';

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

            nativeBuildInputs = with pkgs; [
              cmake
              pkgconfig
              makeWrapper
            ];

            buildInputs = with pkgs; [
              entt
              sqlitecpp
              gbenchmark
              fmt_8
              glm
              gtest
              gtkmm3
              jsoncpp
              libexif
              libjpeg
              libpng
              libsigcxx
              mesa
              openssl
              python3

              SDL2
              SDL2_image
              curl
              glew
              imagemagick6
              libGL
              libGLU
              libexif
              libmhash
              libspnav
              sqlite

              # Silence pkg-config warnings
              pcre
              util-linux
              libselinux
              libsepol
              libunwind
              elfutils
              zstd
              orc
              libthai
              libdatrie
              libxkbcommon
              xorg.libXdmcp
              epoxy
              dbus-glib
              at-spi2-core
              xorg.libXtst
            ] ++ [
              tinycmmc.packages.${system}.default
              logmich.packages.${system}.default
              arxpcpp.packages.${system}.default
              geomcpp.packages.${system}.default
              priocpp.packages.${system}.default
              surfcpp.packages.${system}.default
              babyxml.packages.${system}.default
              sexpcpp.packages.${system}.default
              wstdisplay.packages.${system}.default
              uitest.packages.${system}.default
              strutcpp.packages.${system}.default
            ];
          };
        };

        apps = rec {
          default = galapix_sdl;

          galapix_sdl = flake-utils.lib.mkApp {
            drv = packages.galapix;
            exePath = "/bin/galapix-0.3.sdl";
          };

          galapix_gtk = flake-utils.lib.mkApp {
            drv = packages.galapix;
            exePath = "/bin/galapix-0.3.gtk";
          };
        };
      }
    );
}
