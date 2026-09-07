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
        pkgs = import nixpkgs {
          inherit system;
          config = { allowUnfree = true; };
        };
        version =
          nixpkgs.lib.strings.removeSuffix "\n" (builtins.readFile ./VERSION);
      in rec {
        packages = rec {
          default = galapix;

          galapix = pkgs.stdenv.mkDerivation {
            pname = "galapix";
            inherit version;
            src = nixpkgs.lib.cleanSource ./.;

            meta = with pkgs.lib; {
              description = "An image viewer for large image collections";
              homepage = "https://github.com/Galapix/galapix";
              license = licenses.gpl3Plus;
              mainProgram = "galapix.sdl";
              platforms = platforms.linux;
            };

            # Disabled due to insecure qtwebkit dependency
            # --set GALAPIX_KOCONVERTER "${pkgs.calligra}/bin/koconverter"
            postFixup = ''
              if [ ! -x "$out/bin/galapix.sdl" ]; then
                echo "error: galapix.sdl was not installed" >&2
                exit 1
              fi
              wrapProgram $out/bin/galapix.sdl \
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
              cmake
              pkg-config
              makeWrapper
            ];

            buildInputs = with pkgs; [
              SDL2
              SDL2_image
              boost187
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

            # SDL only under Nix. Gtk2 needs libglademm/gtkglextmm, which are
            # no longer in nixpkgs; the incomplete Gtk UI is optional via CMake
            # on systems that still provide those packages.
            cmakeFlags = [
              "-DBUILD_GALAPIX_SDL=ON"
              "-DBUILD_GALAPIX_GTK=OFF"
            ];
          };
        };

        apps = rec {
          galapix_sdl = flake-utils.lib.mkApp {
            drv = packages.galapix;
            exePath = "/bin/galapix.sdl";
          };
          default = galapix_sdl;
        };
      }
    );
}
