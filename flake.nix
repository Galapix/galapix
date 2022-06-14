{
  description = "An image viewer for large image collections";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.05";
    flake-utils.url = "github:numtide/flake-utils";

    vidthumb.url = "github:grumbel/vidthumb";
    vidthumb.inputs.nixpkgs.follows = "nixpkgs";
    vidthumb.inputs.flake-utils.follows = "flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, vidthumb }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        # pkgs = nixpkgs.legacyPackages.${system};
        pkgs = import nixpkgs {
          inherit system;
          config = { allowUnfree = true; };
        };
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
                  --prefix LD_LIBRARY_PATH ":" "${pkgs.mesa.drivers}/lib" \
                  --set LIBGL_DRIVERS_PATH "${pkgs.mesa.drivers}/lib/dri" \
                  --set GALAPIX_KOCONVERTER "${pkgs.calligra}/bin/koconverter" \
                  --set GALAPIX_RAR "${pkgs.rar}/bin/rar" \
                  --set GALAPIX_RSVG "${pkgs.librsvg}/bin/rsvg" \
                  --set GALAPIX_7ZR "${pkgs.p7zip}/bin/7zr" \
                  --set GALAPIX_TAR "${pkgs.gnutar}/bin/tar" \
                  --set GALAPIX_UFRAW_BATCH "${pkgs.nufraw}/bin/nufraw-batch" \
                  --set GALAPIX_VIDTHUMB "${vidthumb.defaultPackage.${system}}/bin/vidthumb" \
                  --set GALAPIX_XCFINFO "${pkgs.xcftools}/bin/xcfinfo" \
                  --set GALAPIX_XCF2PNG "${pkgs.xcftools}/bin/xcf2png" \
                  --set GALAPIX_UNZIP "${pkgs.unzip}/bin/unzip"
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
