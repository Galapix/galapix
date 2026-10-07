# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
{
  description = "thumtoo — media index and display-pixel ladder library";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  # Optional corpus for checks.bench-smoke-lite (github:Grumbel/benchtoo).
  inputs.benchtoo = {
    url = "github:Grumbel/benchtoo";
    inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { self, nixpkgs, benchtoo }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f {
        inherit system;
        pkgs = import nixpkgs { inherit system; };
      });

      # Pin MuPDF ≥ 1.28 for native Markdown (and future .txt) document support.
      # nixpkgs is still on 1.27.2; biltoo docs/TXT_MD_SUPPORT.md waits on this.
      # Consumers (biltoo) get the same pin via lib.mkBuildInputs.
      pinMupdf = pkgs: pkgs.mupdf.overrideAttrs (old: rec {
        version = "1.28.5";
        src = pkgs.fetchurl {
          url = "https://mupdf.com/downloads/archive/mupdf-${version}-source.tar.gz";
          hash = "sha256-mKXBDNogw5ks33b/ayoRScMr15zHltP3AyMLEYW36TQ=";
        };
        # nixpkgs 1.27.x patches may not apply on 1.28; start without them.
        patches = [ ];
        # Markdown needs cmark-gfm. Upstream source tarball vendors
        # thirdparty/cmark-gfm; nixpkgs USE_SYSTEM_LIBS only removes listed
        # thirdparty dirs (curl, freetype, …) and does not list cmark-gfm, so
        # the vendored copy is kept. No separate nixpkgs cmark-gfm input required
        # for a first pin. If Markdown probes fail later, switch to system
        # cmark-gfm + USE_SYSTEM_CMARK_GFM.
        # nixpkgs mupdf uses `rec { version = …; postInstall = "… ${version} …" }`,
        # so overrideAttrs on version alone leaves Version: 1.27.2 in mupdf.pc
        # while the store path is already mupdf-1.28.5.
        postFixup = (old.postFixup or "") + ''
          for pc in "$dev/lib/pkgconfig"/mupdf*.pc "$out/lib/pkgconfig"/mupdf*.pc; do
            if [ -f "$pc" ]; then
              sed -i "s/^Version:.*/Version: ${version}/" "$pc"
            fi
          done
        '';
      });

      # libvips + JPEG-XL, and the Requires.private packages whose .pc files
      # pkg-config looks for when probing vips (same set biltoo uses to silence
      # "Package '…' was not found" spam). We do not necessarily link all of
      # these into thumtoo; they only need to be on PKG_CONFIG_PATH.
      # Also exported as lib.mkBuildInputs for consumers (biltoo) that
      # add_subdirectory thumtoo and must have the same pkg-config deps.
      vipsInputs = pkgs:
        let
          pkgs' = pkgs // { mupdf = pinMupdf pkgs; };
        in
        with pkgs'; [
        sqlite
        vips
        libjxl

        # glib Requires.private
        glib
        libsysprof-capture
        pcre2          # libpcre2-8.pc

        # gio-2.0 Requires.private (pulled in via glib/vips)
        util-linux     # mount.pc
        libselinux     # libselinux.pc
        libsepol       # libsepol.pc (Requires.private of libselinux)

        # libarchive
        libunarr 
        libarchive
        mupdf          # pinned ≥ 1.28.5 (see pinMupdf)
        tesseract
        # tesseract.pc Requires: lept — without leptonica on PKG_CONFIG_PATH,
        # pkg_check_modules(tesseract) spams "Package 'lept' was not found"
        # (even when tesseract itself is found). We do not link lept directly.
        leptonica
        djvulibre
        djvulibre.dev
        curl
        openssl        # libcrypto.pc
        dbus           # dbus-1.pc (XDG Thumbnailer1)
        # dbus-1.pc Requires.private: libsystemd — silence pkg-config spam
        systemd

        # vips Requires.private (and common transitive .pc names)
        fftw
        cfitsio
        libimagequant
        cgif
        libexif
        libultrahdr
        libwebp
        pango
        fribidi
        libthai        # libthai.pc (pango)
        libdatrie      # libdatrie.pc (libthai)
        libtiff
        librsvg
        libxml2        # libxml-2.0.pc (librsvg / others)
        dav1d
        matio
        hdf5
        lcms2
        openexr
        libraw
        openjpeg
        libhwy
        libxdmcp       # xdmcp.pc (X11 transitive via pango/cairo)
      ];

      versionBase = nixpkgs.lib.strings.removeSuffix "\n" (builtins.readFile ./VERSION);
      revCount = toString (self.revCount or 0);
      gitRev =
        if self ? shortRev then self.shortRev
        else if self ? dirtyShortRev then self.dirtyShortRev
        else "dirty";
      # SemVer-ish: 0.1.0-dev.N+gHASH when VERSION ends with -dev
      version =
        if nixpkgs.lib.strings.hasInfix "-dev" versionBase
        then "${versionBase}.${revCount}+g${gitRev}"
        else versionBase;

      mkPackage = pkgs: pkgs.stdenv.mkDerivation {
        pname = "thumtoo";
        inherit version;
        src = self;
        # python3: lets CMake register the bench_tools_cli ctest (tests/test_bench_tools_cli.py).
        nativeBuildInputs = [ pkgs.cmake pkgs.ninja pkgs.pkg-config pkgs.python3 ];
        buildInputs = vipsInputs pkgs;
        cmakeFlags = [
          "-GNinja"
          "-DTHUMTOO_BUILD_TESTS=ON"
          "-DTHUMTOO_BUILD_TOOLS=ON"
          "-DPROJECT_VERSION_FULL=${version}"
        ];
        doCheck = true;
        # Bin presence is checked by `checks.tools-bin` (`nix flake check`),
        # not postInstall. Install list: CMakeLists.txt THUMTOO_BUILD_TOOLS.
        meta = with pkgs.lib; {
          description = "Persistent media index and display-pixel ladder";
          license = licenses.gpl3Plus;
          platforms = platforms.unix;
        };
      };

      # Galapix-style out-of-tree helpers for `nix develop`.
      mkDevScripts = pkgs:
        let
          preamble = ''
            set -euo pipefail
            if [ -z "''${THUMTOO_SOURCE:-}" ]; then
              echo "$0: THUMTOO_SOURCE is not set (enter the shell with: nix develop)" >&2
              exit 1
            fi
            THUMTOO_BUILD_DIR="''${THUMTOO_BUILD_DIR:-/tmp/thumtoo-build}"
          '';
          configure = pkgs.writeShellScriptBin "thumtoo-configure" (
            preamble
            + ''
              cmake -S "$THUMTOO_SOURCE" -B "$THUMTOO_BUILD_DIR" -G Ninja \
                -DCMAKE_BUILD_TYPE="''${CMAKE_BUILD_TYPE:-Debug}" \
                -DTHUMTOO_BUILD_TESTS=ON \
                -DTHUMTOO_BUILD_TOOLS=ON
            ''
          );
          build = pkgs.writeShellScriptBin "thumtoo-build" (
            preamble
            + ''
              if [ ! -f "$THUMTOO_BUILD_DIR/build.ninja" ] && [ ! -f "$THUMTOO_BUILD_DIR/Makefile" ]; then
                thumtoo-configure || exit 1
              fi
              cmake --build "$THUMTOO_BUILD_DIR" "$@"
            ''
          );
          test = pkgs.writeShellScriptBin "thumtoo-test" (
            preamble
            + ''
              thumtoo-build || exit 1
              ctest --test-dir "$THUMTOO_BUILD_DIR" --output-on-failure "$@"
            ''
          );
          runTool = name: pkgs.writeShellScriptBin "thumtoo-run-${name}" (
            preamble
            + ''
              thumtoo-build || exit 1
              bin="$THUMTOO_BUILD_DIR/thumtoo-${name}"
              if [ ! -x "$bin" ]; then
                echo "thumtoo-run-${name}: $bin missing after build" >&2
                exit 1
              fi
              # No exec: return to interactive shell when typed by hand.
              "$bin" "$@"
            ''
          );
          runStatus = runTool "status";
          runPrepare = runTool "prepare";
          runBench = runTool "bench";
          runArchive = runTool "archive";
          runGdb = pkgs.writeShellScriptBin "thumtoo-run-gdb" (
            preamble
            + ''
              thumtoo-build || exit 1
              tool="''${1:-prepare}"
              shift || true
              bin="$THUMTOO_BUILD_DIR/thumtoo-$tool"
              if [ ! -x "$bin" ]; then
                echo "thumtoo-run-gdb: $bin missing after build" >&2
                exit 1
              fi
              if ! command -v gdb >/dev/null 2>&1; then
                echo "thumtoo-run-gdb: gdb not found (should be in the nix develop shell)" >&2
                exit 1
              fi
              gdb --args "$bin" "$@"
            ''
          );
        in [
          configure
          build
          test
          runStatus
          runPrepare
          runBench
          runArchive
          runGdb
        ];
    in {
      # biltoo (and others) that add_subdirectory this source need these on
      # PKG_CONFIG_PATH / link path — otherwise optional backends (libunarr, …)
      # silently disable at configure time.
      lib = {
        mkBuildInputs = vipsInputs;
        # Overridden MuPDF (1.28.5); for overlays / explicit dependency pins.
        pinMupdf = pinMupdf;
      };

      packages = forAllSystems ({ pkgs, ... }: {
        default = mkPackage pkgs;
      });


      # Package integrity: run with `nix flake check` (not postInstall).
      checks = forAllSystems ({ pkgs, system, ... }:
        let
          pkg = self.packages.${system}.default;
          bins = [
            "thumtoo-status"
            "thumtoo-prepare"
            "thumtoo-bench"
            "thumtoo-tile"
            "thumtoo-export"
            "thumtoo-gc"
            "thumtoo-archive"
            "thumtoo-microbench-decode"
            "thumtoo-gp-tile"
            "thumtoo-gp-archive"
          ];
        in {
          tools-bin = pkgs.runCommand "thumtoo-tools-bin-check" { } ''
            set -euo pipefail
            for b in ${pkgs.lib.concatStringsSep " " bins}; do
              if [ ! -x "${pkg}/bin/$b" ]; then
                echo "missing or not executable: ${pkg}/bin/$b" >&2
                ls -la "${pkg}/bin" >&2 || true
                exit 1
              fi
            done
            echo "ok: all tool binaries present"
            touch "$out"
          '';
          # Script presence only — full smoke needs a built tree + benchtoo corpus.
          bench-smoke-script = pkgs.runCommand "thumtoo-bench-smoke-script-check" { } ''
            set -euo pipefail
            script=${./tools/bench_smoke.sh}
            test -f "$script"
            test -x "$script" || chmod +x "$script"
            # Basic syntax / usage surface
            grep -q "microbench-decode\|gp-tile\|gp-archive" "$script"
            grep -q "benchtoo" "$script"
            echo "ok: tools/bench_smoke.sh present and references golden tools + benchtoo"
            touch "$out"
          '';

          # Lite smoke: run golden tools against a few benchtoo fixtures (no full Client DB).
          # Requires inputs.benchtoo; builds corpus package then microbench-decode + gp-archive.
          bench-smoke-lite = pkgs.runCommand "thumtoo-bench-smoke-lite" {
            nativeBuildInputs = [ pkg ];
            # Prefer corpus-smoke (fast); fall back to full corpus on older benchtoo tips
            corpus = benchtoo.packages.${system}.corpus-smoke or benchtoo.packages.${system}.corpus;
          } ''
            set -euo pipefail
            echo "corpus=$corpus"
            jpeg=$(ls "$corpus"/synthetic/jpeg/*_q90.jpg 2>/dev/null | head -1 || true)
            if [ -z "$jpeg" ]; then
              echo "no synthetic jpeg in benchtoo corpus" >&2
              exit 1
            fi
            echo "jpeg=$jpeg"
            ${pkg}/bin/thumtoo-microbench-decode --repeat 1 "$jpeg"
            ${pkg}/bin/thumtoo-gp-tile --codec jpeg --quality 80 --repeat 1 --max-cells 4 "$jpeg"
            # Codec comparison on the smallest lossless source (unsupported
            # codecs, e.g. AVIF without an AV1 encoder, are reported, not fatal).
            png=$(ls -Sr "$corpus"/synthetic/png/*.png 2>/dev/null | head -1 || true)
            if [ -n "$png" ]; then
              ${pkg}/bin/thumtoo-gp-tile --codec all --quality 50,80 --repeat 1 --max-cells 2 "$png"
            fi
            arch=$(ls "$corpus"/archives/*.cbz "$corpus"/documents/sample_book.cbz 2>/dev/null | head -1 || true)
            if [ -n "$arch" ]; then
              ${pkg}/bin/thumtoo-gp-archive --repeat 1 --backend libarchive "$arch"
              ${pkg}/bin/thumtoo-gp-archive --repeat 1 --backend all "$arch"
            fi
            echo "ok: bench-smoke-lite"
            touch "$out"
          '';

          # Self-check compare_bench_json against example baseline (identity = pass).
          baseline-compare-tool = pkgs.runCommand "thumtoo-baseline-compare-tool" {
            nativeBuildInputs = [ pkgs.python3 ];
          } ''
            set -euo pipefail
            base=${./docs/bench/baselines/example/gp-archive-libarchive.json}
            python3 ${./tools/compare_bench_json.py} --baseline "$base" --current "$base"
            echo "ok: compare_bench_json identity"
            # Identity keying, tolerances, winner changes, exit codes. The
            # test finds the tool next to itself, so copy both together.
            mkdir tools
            cp ${./tools/compare_bench_json.py} tools/compare_bench_json.py
            cp ${./tools/test_compare_bench_json.py} tools/test_compare_bench_json.py
            python3 tools/test_compare_bench_json.py
            touch "$out"
          '';
        });

      apps = forAllSystems ({ pkgs, system, ... }:
        let
          pkg = self.packages.${system}.default;
          app = exe: description: {
            type = "app";
            program = "${pkg}/bin/${exe}";
            meta = {
              description = description;
            };
          };
        in {
          default = app "thumtoo-status" "Show thumtoo cache status";
          status = app "thumtoo-status" "Show thumtoo cache status";
          prepare = app "thumtoo-prepare" "Prewarm size probe and soft ladder for paths";
          bench = app "thumtoo-bench" "Benchmark ladder / tile paths";
          gc = app "thumtoo-gc" "Garbage-collect unreferenced cache blobs";
          archive = app "thumtoo-archive" "List/extract archive members (same backends as //archive:)";
          micro-decode = app "thumtoo-microbench-decode" "Golden-path vips JPEG decode timings (no Client)";
          gp-tile = app "thumtoo-gp-tile" "Golden-path tile encode/decode (jpeg/webp/avif/jxl); --codec all picks a winner";
          gp-archive = app "thumtoo-gp-archive" "Golden-path archive TOC/extract (libarchive/unarr); --backend all picks a winner";
        });

      devShells = forAllSystems ({ pkgs, ... }: {
        default = pkgs.mkShell {
          inputsFrom = [ (mkPackage pkgs) ];
          packages = with pkgs; [
            cmake
            ninja
            gcc
            clang-tools
            gdb
            pkg-config
          ] ++ (vipsInputs pkgs) ++ (mkDevScripts pkgs);
          CMAKE_BUILD_TYPE = "Debug";
          shellHook = ''
            export THUMTOO_SOURCE="$PWD"
            export THUMTOO_BUILD_DIR="''${THUMTOO_BUILD_DIR:-/tmp/thumtoo-build}"
            echo "thumtoo dev shell (CMAKE_BUILD_TYPE=''${CMAKE_BUILD_TYPE:-Debug})"
            echo "  source:    $THUMTOO_SOURCE"
            echo "  build dir: $THUMTOO_BUILD_DIR"
            echo "  thumtoo-configure          # cmake -S . -B \$THUMTOO_BUILD_DIR -G Ninja"
            echo "  thumtoo-build [args…]      # build (configure if needed)"
            echo "  thumtoo-test [ctest args…] # build + ctest"
            echo "  thumtoo-run-status …"
            echo "  thumtoo-run-prepare …"
            echo "  thumtoo-run-bench …"
            echo "  thumtoo-run-archive …"
            echo "  thumtoo-run-gdb [tool] …   # tool = prepare|bench|status|archive"
            echo "  flake apps: nix run .#status|prepare|bench|archive"
            echo "  nix flake check            # includes tools-bin check"
          '';
        };
      });
    };
}
