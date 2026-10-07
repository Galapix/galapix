{
  description = "XDG Base Directory Specification implementation in C++";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        xdgcpp = pkgs.stdenv.mkDerivation (finalAttrs: {
          pname = "xdgcpp";
          version = "0.1.0";

          src = pkgs.lib.cleanSource ./.;

          cmakeFlags = [
            "-DXDG_BUILD_TESTS=ON"
            "-DXDG_BUILD_INFO=ON"
          ];

          # Build + run the unit-test suite via ctest.
          doCheck = true;

          nativeBuildInputs = with pkgs; [
            cmake
          ];

          meta = with pkgs.lib; {
            description = "XDG Base Directory Specification implementation in C++";
            homepage = "https://github.com/grumbel/xdgcpp";
            license = licenses.lgpl3Plus;
            platforms = platforms.unix;
          };
        });
      in
      rec {
        packages = {
          default = xdgcpp;
          inherit xdgcpp;
        };

        # `nix flake check` builds every attribute under checks.*.
        checks = {
          inherit xdgcpp;
        };

        apps = rec {
          xdgcpp-info = {
            type = "app";
            program = "${packages.xdgcpp}/bin/xdgcpp-info";
            meta = {
              description = "Print resolved XDG base directories";
            };
          };
          default = xdgcpp-info;
        };
      }
    );
}
