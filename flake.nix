{
  description = "capstone project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };

        dependencies = with pkgs; [
          ns-3
          yaml-cpp
        ];

        buildTools = with pkgs; [
          gcc

          cmake
          ninja
          pkg-config
        ];

        devTools = with pkgs; [
          clang-tools
          neocmakelsp
          wireshark
        ];
      in
      {
        devShells.default = pkgs.mkShell {
          nativeBuildInputs = buildTools;
          buildInputs = dependencies;
          packages = devTools;
        };
      });
}
