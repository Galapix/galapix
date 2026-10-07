{
  description = "Software Surface Library";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    logmich.url = "github:logmich/logmich";
    logmich.inputs.nixpkgs.follows = "nixpkgs";

    geomcpp.url = "github:grumbel/geomcpp";
    geomcpp.inputs.nixpkgs.follows = "nixpkgs";

    SDL2-win32.url = "github:grumnix/SDL2-win32";
    SDL2-win32.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { self, nixpkgs, flake-utils, geomcpp, logmich, SDL2-win32 }:
    let
      versionBase = nixpkgs.lib.strings.removeSuffix "\n" (builtins.readFile ./VERSION);
      gitRev = "${self.shortRev or self.dirtyShortRev or "dirty"}";
      isDev = nixpkgs.lib.strings.hasInfix "-dev" versionBase;
      version =
        if isDev then
          "${versionBase}.${toString (self.revCount or 0)}+g${gitRev}"
        else
          versionBase;

      eachSystem = flake-utils.lib.eachSystem (flake-utils.lib.defaultSystems ++ [ "x86_64-windows" "i686-windows" ]);
      pkgsFromSystem = system:
        if system == "x86_64-windows" then nixpkgs.legacyPackages.x86_64-linux.pkgsCross.mingwW64
        else if system == "i686-windows" then nixpkgs.legacyPackages.x86_64-linux.pkgsCross.mingw32
        else nixpkgs.legacyPackages.${system};
    in
    eachSystem (system:
      let
        pkgs = pkgsFromSystem system;
      in
      {
        packages = rec {
          default = surfcpp;

          surfcpp = pkgs.callPackage ./surfcpp.nix {
            stdenv = pkgs.stdenv;
            SDL2 = if pkgs.stdenv.hostPlatform.isWindows then SDL2-win32.packages.${pkgs.stdenv.hostPlatform.system}.default else pkgs.SDL2;
            libjpeg = if pkgs.stdenv.hostPlatform.isWindows
                      then (pkgs.libjpeg_original.overrideAttrs (oldAttrs: { meta = {}; }))
                      else pkgs.libjpeg;
            geomcpp = geomcpp.packages.${pkgs.stdenv.hostPlatform.system}.default;
            logmich = logmich.packages.${pkgs.stdenv.hostPlatform.system}.default;
            inherit version;
          };

          surfcpp-stb = surfcpp.override {
            withStb = true;
          };
        };
      }
    );
}
