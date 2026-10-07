{
  description = "A tiny CMake module collection";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    let
      tinycmmc_lib = import ./. { inherit nixpkgs flake-utils; };
    in
      {
        lib = import ./. { inherit nixpkgs flake-utils; };
      } //
      tinycmmc_lib.eachSystemWithPkgs (pkgs:
        rec {
          packages = rec {
            default = tinycmmc;
            tinycmmc = pkgs.callPackage ./tinycmmc.nix {
              inherit self tinycmmc_lib;
            };
          };

          # `nix flake check` builds these. The package itself is the main
          # check (cmake configure + install). On native platforms also run a
          # find_package smoke test so the installed config file and module
          # path are verified end-to-end.
          checks = {
            tinycmmc = packages.tinycmmc;
          } // pkgs.lib.optionalAttrs (pkgs.stdenv.buildPlatform == pkgs.stdenv.hostPlatform) {
            cmake-find = pkgs.runCommand "tinycmmc-cmake-find" {
              nativeBuildInputs = [ pkgs.cmake packages.tinycmmc ];
            } ''
              cat > CMakeLists.txt <<'EOF_CMAKE'
              cmake_minimum_required(VERSION 3.10)
              project(tinycmmc-find-test LANGUAGES NONE)
              find_package(tinycmmc REQUIRED CONFIG)
              if(NOT TINYCMMC_MODULE_PATH)
                message(FATAL_ERROR "TINYCMMC_MODULE_PATH not set by tinycmmcConfig.cmake")
              endif()
              message(STATUS "Found tinycmmc modules at: ''${TINYCMMC_MODULE_PATH}")
              EOF_CMAKE
              cmake -S . -B build
              touch $out
            '';
          };
        }
      );
}
