{
  description = "An image viewer for large image collections";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
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
        packages = rec {
          default = galapix;

          galapix = pkgs.ccacheStdenv.mkDerivation {
            pname = "galapix";
            version = "0.2.2";
            src = nixpkgs.lib.cleanSource ./.;
            enableParallelBuilding = true;
            sconsFlags = [
              "GALAPIX_SDL=True"
              "GALAPIX_GTK=False"
            ];
            preConfigure = ''
              export CCACHE_DIR=/nix/var/cache/ccache
              export CCACHE_UMASK=007
              if [ ! -d "''${CCACHE_DIR}" ]; then
                export CCACHE_DISABLE=1
              fi
            '';
            installPhase = ''
              mkdir -p $out/bin
              cp build/galapix.sdl $out/bin/
            '';

            # Disabled due to insecure qtwebkit dependency
            # --set GALAPIX_KOCONVERTER "${pkgs.calligra}/bin/koconverter"
            postFixup = ''
                wrapProgram $out/bin/galapix.sdl \
                  --prefix LIBGL_DRIVERS_PATH ":" "${pkgs.mesa.drivers}/lib/dri" \
                  --prefix LD_LIBRARY_PATH ":" "${pkgs.mesa.drivers}/lib" \
                  --set LIBGL_DRIVERS_PATH "${pkgs.mesa.drivers}/lib/dri" \
                  --set GALAPIX_RAR "${pkgs.rar}/bin/rar" \
                  --set GALAPIX_RSVG "${pkgs.librsvg}/bin/rsvg" \
                  --set GALAPIX_7ZR "${pkgs.p7zip}/bin/7zr" \
                  --set GALAPIX_TAR "${pkgs.gnutar}/bin/tar" \
                  --set GALAPIX_UFRAW_BATCH "${pkgs.nufraw}/bin/nufraw-batch" \
                  --set GALAPIX_VIDTHUMB "${vidthumb.packages.${system}.default}/bin/vidthumb" \
                  --set GALAPIX_XCFINFO "${pkgs.xcftools}/bin/xcfinfo" \
                  --set GALAPIX_XCF2PNG "${pkgs.xcftools}/bin/xcf2png" \
                  --set GALAPIX_UNZIP "${pkgs.unzip}/bin/unzip"
            '';
            nativeBuildInputs = with pkgs; [
              scons
              pkg-config
              makeWrapper
            ];
            buildInputs = with pkgs; [
              SDL2
              SDL2_image
              boost
              curl
              glew
              imagemagick
              libGL
              libGLU
              libexif
              libmhash
              libspnav
              sqlite
            ];
           };
        };
        apps = rec {
          galapix_sdl = flake-utils.lib.mkApp {
            drv = packages.galapix;
            exePath = "/bin/galapix.sdl";
          };
          galapix_gtk = flake-utils.lib.mkApp {
            drv = packages.galapix;
            exePath = "/bin/galapix.gtk";
          };
          default = galapix_sdl;
        };
      }
    );
}
