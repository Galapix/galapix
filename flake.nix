{
  description = "An image viewer for large image collections";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-21.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in rec {
        packages = flake-utils.lib.flattenTree rec {
          galapix = pkgs.stdenv.mkDerivation {
            pname = "galapix";
            version = "0.3.0";
            src = nixpkgs.lib.cleanSource ./.;
            enableParallelBuilding = true;
            cmakeFlags = [
              "-DBUILD_GALAPIX_SDL=ON"
              "-DBUILD_GALAPIX_GTK=ON"
              "-DBUILD_TESTS=ON"
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
            ];
           };
        };
        defaultPackage = packages.galapix;
      });
}
