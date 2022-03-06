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
        packages = flake-utils.lib.flattenTree {
          galapix = pkgs.stdenv.mkDerivation {
            pname = "galapix";
            version = "0.2.2";
            meta = {
              mainProgram = "galapix.sdl";
            };
            src = nixpkgs.lib.cleanSource ./.;
            enableParallelBuilding = true;
            sconsFlags = [
              "GALAPIX_SDL=True"
              "GALAPIX_GTK=False"
            ];
            installPhase = ''
              mkdir -p $out/bin
              cp build/galapix.sdl $out/bin/
            '';
            postFixup = ''
                wrapProgram $out/bin/galapix.sdl \
                  --prefix LIBGL_DRIVERS_PATH ":" "${pkgs.mesa.drivers}/lib/dri" \
                  --prefix LD_LIBRARY_PATH ":" "${pkgs.mesa.drivers}/lib"
            '';
            nativeBuildInputs = [
              pkgs.scons
              pkgs.pkgconfig
              pkgs.makeWrapper
            ];
            buildInputs = [
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
