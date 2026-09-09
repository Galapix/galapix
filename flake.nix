{
  description = "An image viewer for large image collections";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";
    tinycmmc.inputs.flake-utils.follows = "flake-utils";

    geomcpp.url = "github:grumbel/geomcpp";
    geomcpp.inputs.nixpkgs.follows = "nixpkgs";



    logmich.url = "github:logmich/logmich";
    logmich.inputs.nixpkgs.follows = "nixpkgs";

    priocpp.url = "github:grumbel/priocpp";
    priocpp.inputs.nixpkgs.follows = "nixpkgs";
    priocpp.inputs.flake-utils.follows = "flake-utils";
    priocpp.inputs.logmich.follows = "logmich";
    priocpp.inputs.sexpcpp.follows = "sexpcpp";

    sexpcpp.url = "github:lispparser/sexp-cpp";
    sexpcpp.inputs.nixpkgs.follows = "nixpkgs";
    sexpcpp.inputs.flake-utils.follows = "flake-utils";

    strutcpp.url = "github:grumbel/strutcpp";
    strutcpp.inputs.nixpkgs.follows = "nixpkgs";

    xdgcpp.url = "github:Grumbel/xdgcpp";
    xdgcpp.inputs.nixpkgs.follows = "nixpkgs";
    xdgcpp.inputs.flake-utils.follows = "flake-utils";

    surfcpp.url = "github:grumbel/surfcpp";
    surfcpp.inputs.nixpkgs.follows = "nixpkgs";
    surfcpp.inputs.geomcpp.follows = "geomcpp";
    surfcpp.inputs.logmich.follows = "logmich";

    uitest.url = "github:grumbel/uitest";
    uitest.inputs.nixpkgs.follows = "nixpkgs";
    uitest.inputs.flake-utils.follows = "flake-utils";

    babyxml.url = "github:grumbel/babyxml";
    babyxml.inputs.nixpkgs.follows = "nixpkgs";

    wstdisplay.url = "github:WindstilleTeam/wstdisplay";
    wstdisplay.inputs.nixpkgs.follows = "nixpkgs";
    wstdisplay.inputs.geomcpp.follows = "geomcpp";
    wstdisplay.inputs.babyxml.follows = "babyxml";
    wstdisplay.inputs.surfcpp.follows = "surfcpp";
    wstdisplay.inputs.logmich.follows = "logmich";

    # Tile backend source tree (CMake add_subdirectory via THUMTOO_DIR).
    # flake=false: input is the checkout path, not thumtoo's package outputs.
    # Locked in flake.lock like any other input.
    # If Galapix must diverge from upstream again, vendor via git subtree
    # (see AGENTS.md); do not reintroduce pkgs.applyPatches + patches/*.patch.
    thumtoo = {
      url = "github:Grumbel/thumtoo";
      flake = false;
    };

  };

  outputs = { self, nixpkgs, flake-utils,
              tinycmmc, geomcpp, logmich, priocpp, sexpcpp, strutcpp, surfcpp, uitest, babyxml, wstdisplay, xdgcpp, thumtoo }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        # thumtoo source tree for CMake add_subdirectory (THUMTOO_DIR).
        thumtooSrc = thumtoo;
        # Match biltoo: VERSION file + flake revCount/shortRev (not git-describe).
        versionBase = nixpkgs.lib.strings.removeSuffix "\n" (builtins.readFile ./VERSION);
        gitRev = "${self.shortRev or self.dirtyShortRev or "dirty"}";
        isDev = nixpkgs.lib.strings.hasInfix "-dev" versionBase;
        version =
          if isDev then
            "${versionBase}.${toString (self.revCount or 0)}+g${gitRev}"
          else
            versionBase;
      in rec {
        packages = rec {
          default = galapix;

          galapix = pkgs.stdenv.mkDerivation {
            pname = "galapix";
            inherit version;

            src = nixpkgs.lib.cleanSource ./.;

            enableParallelBuilding = true;

            doCheck = false;

            postInstall = ''
              wrapProgram "$out/bin/galapix" \
                --set GALAPIX_DATADIR "$out/share/galapix"
            '';

            cmakeFlags = [
              # "-DBUILD_TESTS=ON"
              "-DBUILD_BENCHMARKS=ON"
              "-DWITH_THUMTOO=ON"
              "-DTHUMTOO_DIR=${thumtooSrc}"
              "-DPROJECT_VERSION_FULL=${version}"
              # SQLiteCpp ships a deprecation note on SQLite::SQLite3 (upstream).
              "-Wno-dev"
            ];

            nativeBuildInputs = with pkgs; [
              cmake
              pkg-config
              makeWrapper
              librsvg # rsvg-convert for tool icons SVG→PNG
            ];

            buildInputs = with pkgs; [
              # Removed (unused by Galapix sources): entt, python3, libmhash, jsoncpp
              gbenchmark
              glm
              gtest
              libjpeg
              libpng
              libsigcxx
              mesa

              SDL2
              SDL2_image
              curl
              imagemagick
              libGL
              libGLU
              # libexif: no direct Galapix refs; drop unless surfcpp needs it at link time
              libspnav

              # thumtoo (via add_subdirectory): its own SQLite cache + ladder
              sqlite   # required by thumtoo CMake (not Galapix cache4)
              vips
              libjxl
              libarchive
              poppler
              mupdf
              djvulibre

              # Silence pkg-config warnings
              cgif
              libexif
              libultrahdr
              libwebp
              pango
              fribidi
              libtiff
              librsvg
              dav1d
              matio
              hdf5
              lcms2
              openexr
              libraw
              openjpeg
              libhwy
              libimagequant
              libxml2
              libsysprof-capture
              pcre2
              cfitsio
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
              libxdmcp
              libepoxy
              dbus-glib
              at-spi2-core
              libxtst
            ] ++ [
              tinycmmc.packages.${system}.default
              logmich.packages.${system}.default
              geomcpp.packages.${system}.default
              priocpp.packages.${system}.default
              surfcpp.packages.${system}.default
              babyxml.packages.${system}.default
              sexpcpp.packages.${system}.default
              wstdisplay.packages.${system}.default
              uitest.packages.${system}.default
              strutcpp.packages.${system}.default
              xdgcpp.packages.${system}.default
            ];
          };
        };

        apps = rec {
          default = galapix;

          galapix = flake-utils.lib.mkApp {
            drv = packages.galapix;
            exePath = "/bin/galapix";
          };

        };

        # Out-of-tree debug workflow (see biltoo-style helpers).
        devShells.default =
          let
            galapixDevPreamble = ''
              set -euo pipefail
              if [ -z "''${GALAPIX_SOURCE:-}" ]; then
                echo "$0: GALAPIX_SOURCE is not set (enter the shell with: nix develop)" >&2
                exit 1
              fi
              GALAPIX_BUILD_DIR="''${GALAPIX_BUILD_DIR:-/tmp/galapix-build}"
              # Resolve thumtoo source for -DTHUMTOO_DIR:
              # 1) explicit THUMTOO_DIR
              # 2) sibling checkout ../thumtoo (common when developing both)
              # 3) flake input (locked in flake.lock)
              if [ -z "''${THUMTOO_DIR:-}" ]; then
                if [ -f "$GALAPIX_SOURCE/../thumtoo/CMakeLists.txt" ]; then
                  THUMTOO_DIR="$(cd "$GALAPIX_SOURCE/../thumtoo" && pwd)"
                elif [ -f "$GALAPIX_SOURCE/third_party/thumtoo/CMakeLists.txt" ]; then
                  THUMTOO_DIR="$(cd "$GALAPIX_SOURCE/third_party/thumtoo" && pwd)"
                else
                  THUMTOO_DIR="${thumtooSrc}"
                fi
              fi
              export THUMTOO_DIR
            '';
            galapixConfigure = pkgs.writeShellScriptBin "galapix-configure" (
              galapixDevPreamble
              + ''
                cmake -S "$GALAPIX_SOURCE" -B "$GALAPIX_BUILD_DIR" -G Ninja \
                  -DCMAKE_BUILD_TYPE="''${CMAKE_BUILD_TYPE:-Debug}" \
                  -DWARNINGS=ON \
                  -DWERROR=ON \
                  -DBUILD_BENCHMARKS=OFF \
                  -DWITH_THUMTOO=ON \
                  -DTHUMTOO_DIR="''${THUMTOO_DIR:-${thumtooSrc}}" \
                  -Wno-dev
              ''
            );
            galapixBuild = pkgs.writeShellScriptBin "galapix-build" (
              galapixDevPreamble
              + ''
                need_configure=0
                if [ ! -f "$GALAPIX_BUILD_DIR/build.ninja" ] && [ ! -f "$GALAPIX_BUILD_DIR/Makefile" ]; then
                  need_configure=1
                else
                  # Pick up a different/local thumtoo tree without a manual reconfigure.
                  cached=""
                  if [ -f "$GALAPIX_BUILD_DIR/CMakeCache.txt" ]; then
                    cached="$(sed -n 's/^THUMTOO_DIR:PATH=//p' "$GALAPIX_BUILD_DIR/CMakeCache.txt" | head -1)"
                  fi
                  if [ "$cached" != "$THUMTOO_DIR" ]; then
                    echo "galapix-build: THUMTOO_DIR changed ($cached -> $THUMTOO_DIR); reconfiguring" >&2
                    need_configure=1
                  fi
                fi
                if [ "$need_configure" -eq 1 ]; then
                  galapix-configure || exit 1
                fi
                cmake --build "$GALAPIX_BUILD_DIR" "$@"
              ''
            );
            # Product binary is `galapix`
            galapixRun = pkgs.writeShellScriptBin "galapix-run" (
              galapixDevPreamble
              + ''
                galapix-build || exit 1
                bin="$GALAPIX_BUILD_DIR/galapix"
                if [ ! -x "$bin" ]; then
                  echo "galapix-run: $bin missing after build" >&2
                  exit 1
                fi
                echo "galapix-run: THUMTOO_DIR=$THUMTOO_DIR" >&2
                export GALAPIX_DATADIR="''${GALAPIX_DATADIR:-$GALAPIX_BUILD_DIR/share/galapix}"
                if [ ! -d "$GALAPIX_DATADIR/icons/hicolor/24x24/actions" ]; then
                  echo "galapix-run: icons missing under $GALAPIX_DATADIR (galapix_icons target?)" >&2
                  exit 1
                fi
                # No exec: return to interactive shell when typed by hand.
                "$bin" "$@"
              ''
            );
            galapixRunGdb = pkgs.writeShellScriptBin "galapix-run-gdb" (
              galapixDevPreamble
              + ''
                galapix-build || exit 1
                bin="$GALAPIX_BUILD_DIR/galapix"
                if [ ! -x "$bin" ]; then
                  echo "galapix-run-gdb: $bin missing after build" >&2
                  exit 1
                fi
                if ! command -v gdb >/dev/null 2>&1; then
                  echo "galapix-run-gdb: gdb not found (should be in the nix develop shell)" >&2
                  exit 1
                fi
                export GALAPIX_DATADIR="''${GALAPIX_DATADIR:-$GALAPIX_BUILD_DIR/share/galapix}"
                gdb --args "$bin" "$@"
              ''
            );
          in
          pkgs.mkShell {
            inputsFrom = [ packages.galapix ];
            packages = (with pkgs; [
              cmake
              ninja
              gdb
              pkg-config
              librsvg
            ]) ++ [
              galapixConfigure
              galapixBuild
              galapixRun
              galapixRunGdb
            ];
            CMAKE_BUILD_TYPE = "Debug";
            shellHook = ''
              export GALAPIX_SOURCE="$PWD"
              export GALAPIX_BUILD_DIR="''${GALAPIX_BUILD_DIR:-/tmp/galapix-build}"
              # PNGs are generated into the build tree (not source data/).
              export GALAPIX_DATADIR="''${GALAPIX_DATADIR:-$GALAPIX_BUILD_DIR/share/galapix}"
              export THUMTOO_DIR="''${THUMTOO_DIR:-${thumtooSrc}}"
              echo "galapix dev shell (CMAKE_BUILD_TYPE=''${CMAKE_BUILD_TYPE:-Debug})"
              echo "  build dir: $GALAPIX_BUILD_DIR"
              echo "  GALAPIX_DATADIR=$GALAPIX_DATADIR"
              echo "  THUMTOO_DIR=$THUMTOO_DIR"
              echo "  galapix-configure     # cmake -S . -B \$GALAPIX_BUILD_DIR -G Ninja (+ thumtoo, WARNINGS=ON WERROR=ON)"
echo "  version: cmake reads VERSION + .git (0.3.0-dev.N+gHASH)"
              echo "  galapix-build         # incremental cmake --build"
              echo "  galapix-run [args]    # build + run galapix"
              echo "  THUMTOO_DIR           # override thumtoo source (default: ../thumtoo or flake lock)"
              echo "  galapix-run-gdb [args]# build + gdb --args galapix"
              echo "  nix build             # packaged RelWithDebInfo-style derivation"
              echo "  also: nix develop -c galapix-run /tmp/*.jpg"
            '';
          };
      }
    );
}
