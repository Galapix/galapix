# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

{
  description = "wst - Windstille game framework (display, input, gui, sprite)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    babyxml.url = "git+https://github.com/grumbel/babyxml.git";
    babyxml.inputs.nixpkgs.follows = "nixpkgs";

    logmich.url = "git+https://github.com/logmich/logmich.git";
    logmich.inputs.nixpkgs.follows = "nixpkgs";

    geomcpp.url = "git+https://github.com/grumbel/geomcpp.git";
    geomcpp.inputs.nixpkgs.follows = "nixpkgs";

    priocpp.url = "git+https://github.com/grumbel/priocpp.git";
    priocpp.inputs.nixpkgs.follows = "nixpkgs";
    priocpp.inputs.logmich.follows = "logmich";

    sexpcpp.url = "git+https://github.com/lispparser/sexp-cpp.git";
    sexpcpp.inputs.nixpkgs.follows = "nixpkgs";

    SDL2-win32.url = "git+https://github.com/grumnix/SDL2-win32.git";
    SDL2-win32.inputs.nixpkgs.follows = "nixpkgs";

    freetype-win32.url = "git+https://github.com/grumnix/freetype-win32.git";
    freetype-win32.inputs.nixpkgs.follows = "nixpkgs";

    surfcpp.url = "git+https://github.com/grumbel/surfcpp.git";
    surfcpp.inputs.nixpkgs.follows = "nixpkgs";
    surfcpp.inputs.geomcpp.follows = "geomcpp";
    surfcpp.inputs.logmich.follows = "logmich";
    surfcpp.inputs.SDL2-win32.follows = "SDL2-win32";
  };

  outputs = { self, nixpkgs, flake-utils,
              babyxml, geomcpp, logmich, priocpp, sexpcpp, surfcpp,
              freetype-win32, SDL2-win32 }:
    let
      # Read the base from VERSION.
      versionBase = nixpkgs.lib.strings.removeSuffix "\n" (builtins.readFile ./VERSION);
      gitRev = "${self.shortRev or self.dirtyShortRev or "dirty"}";
      isDev = nixpkgs.lib.strings.hasInfix "-dev" versionBase;
      version =
        if isDev then
          "${versionBase}.${toString (self.revCount or 0)}+g${gitRev}"
        else
          versionBase;

      # Support Windows cross as tinycmmc did
      eachSystem = flake-utils.lib.eachSystem (flake-utils.lib.defaultSystems ++ [ "x86_64-windows" "i686-windows" ]);
      pkgsFromSystem = system:
        if system == "x86_64-windows" then nixpkgs.legacyPackages.x86_64-linux.pkgsCross.mingwW64
        else if system == "i686-windows" then nixpkgs.legacyPackages.x86_64-linux.pkgsCross.mingw32
        else nixpkgs.legacyPackages.${system};
    in
    eachSystem (system:
      let
        pkgs = pkgsFromSystem system;
        hostSystem = pkgs.stdenv.hostPlatform.system;
      in
      rec {
        packages = rec {
          default = wst;

          wst = pkgs.stdenv.mkDerivation {
            pname = "wst";
            inherit version;

            src = nixpkgs.lib.cleanSource ./.;

            cmakeFlags = [
              "-DBUILD_EXTRA=ON"
              "-DBUILD_TESTS=ON"
              "-DWARNINGS=ON"
              "-DWERROR=ON"
              "-DPROJECT_VERSION_FULL=${version}"
            ];

            # the GL tests need a GPU or EGL, run them with
            # 'ctest -L gl' outside of the sandbox
            doCheck = !pkgs.stdenv.hostPlatform.isWindows;
            checkPhase = ''
              runHook preCheck
              ctest --output-on-failure -LE gl
              runHook postCheck
            '';

            postFixup =
              (nixpkgs.lib.optionalString pkgs.stdenv.hostPlatform.isWindows ''
                mkdir -p $out/bin/
                find ${pkgs.windows.mcfgthreads} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
                find ${pkgs.stdenv.cc.cc} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
                ln -sfv ${SDL2-win32.packages.${hostSystem}.default}/bin/*.dll $out/bin/
                ln -sfv ${pkgs.gtest}/bin/*.dll $out/bin/
                ln -sfv ${freetype-win32.packages.${hostSystem}.default}/bin/*.dll $out/bin/

                # FIXME: These should be handled in surfcpp or statically linked
                ln -sfv ${pkgs.libjpeg_original.overrideAttrs (oldAttrs: { meta = {}; })}/bin/*.dll $out/bin/
                ln -sfv ${pkgs.libpng.overrideAttrs (oldAttrs: { meta = {}; })}/bin/*.dll $out/bin/
                ln -sfv ${pkgs.zlib.overrideAttrs (oldAttrs: { meta = {}; })}/bin/*.dll $out/bin/

                # FIXME: should be handled by sexpcpp itself
                ln -sfv ${pkgs.jsoncpp}/bin/*.dll $out/bin/
            '');

            nativeBuildInputs = [
              pkgs.buildPackages.cmake
              pkgs.buildPackages.pkg-config
            ];

            buildInputs = [
              pkgs.gtest
            ];

            propagatedBuildInputs = [
              babyxml.packages.${hostSystem}.default
              geomcpp.packages.${hostSystem}.default
              logmich.packages.${hostSystem}.default
              priocpp.packages.${hostSystem}.default
              sexpcpp.packages.${hostSystem}.default
              surfcpp.packages.${hostSystem}.default

              (if pkgs.stdenv.hostPlatform.isWindows
               then freetype-win32.packages.${hostSystem}.default
               else pkgs.freetype)

              (if pkgs.stdenv.hostPlatform.isWindows
               then SDL2-win32.packages.${hostSystem}.default
               else pkgs.SDL2)

              # OpenGL via libglvnd (GLAD is vendored)
              pkgs.libGL
            ];
          };
        };

        apps =
          let
            app = exe: description: {
              type = "app";
              program = "${packages.wst}/bin/${exe}";
              meta.description = description;
            };
            demos = {
              demo-destructible = "Destructible terrain with partial texture updates";
              demo-gui = "wstgui widgets and menus";
              demo-lighting = "Compositor lighting passes";
              demo-mesh = "Meshes, custom shaders and framebuffers";
              demo-shapes = "Canvas shapes, transforms, clipping and text";
              demo-sprite-viewer = "SuperTux, Windstille and Pingus .sprite files";
              demo-sprites = "Sprite batching benchmark";
              demo-text = "TTF and bitmap fonts, TextArea";
              demo-tilemap = "Tilemap with static batches and parallax layers";
            };
          in
            nixpkgs.lib.mapAttrs (name: description: app "wst-${name}" description) demos // {
              default = app "wst-demo-shapes" demos.demo-shapes;
              wstgui = app "wstgui" "wstgui example";
            };

        # Build every package, check that every app exists and that the
        # tree is REUSE compliant
        checks = packages // (nixpkgs.lib.mapAttrs' (name: app:
          nixpkgs.lib.nameValuePair "app-${name}" (
            pkgs.runCommand "check-app-${name}" {} ''
              if [ ! -e "${app.program}" ]; then
                echo "apps.${name}: program missing: ${app.program}" >&2
                exit 1
              fi
              touch "$out"
            ''
          )) apps) // {
          reuse = pkgs.runCommand "check-reuse" {
            nativeBuildInputs = [ pkgs.buildPackages.reuse ];
          } ''
            cd ${self}
            reuse lint
            touch "$out"
          '';
        };
      }
    );
}
