{
  description = "capstone project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
  };

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      pkgsFor = system: import nixpkgs {
        inherit system;
      };

    in
    {
      devShells = forAllSystems (system:
        let
          pkgs = pkgsFor system;
        in
        {
          default = pkgs.mkShell {
            buildInputs = with pkgs; [
              gcc
              clang-tools

              ninja
              cmake
              pkg-config
              neocmakelsp

              ns-3
              python3

              wireshark
            ];

            shellHook =''
              echo "ns-3 development environment activated!"  
            '';
          };
        });
    };
}
