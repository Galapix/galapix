{
  description = "A C++ parser for a minimal XML-like dialect";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    let
      versionBase = nixpkgs.lib.strings.removeSuffix "\n" (builtins.readFile ./VERSION);
      gitRev = "${self.shortRev or self.dirtyShortRev or "dirty"}";
      isDev = nixpkgs.lib.strings.hasInfix "-dev" versionBase;
      version =
        if isDev then
          "${versionBase}.${toString (self.revCount or 0)}+g${gitRev}"
        else
          versionBase;
    in
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages = rec {
          default = babyxml;

          babyxml = pkgs.stdenv.mkDerivation {
            pname = "babyxml";
            inherit version;

            src = ./.;

            cmakeFlags = [
              "-DBUILD_EXTRA=ON"
              "-DPROJECT_VERSION_FULL=${version}"
            ];

            nativeBuildInputs = with pkgs; [
              buildPackages.cmake
            ];
          };
        };
      }
    );
}
