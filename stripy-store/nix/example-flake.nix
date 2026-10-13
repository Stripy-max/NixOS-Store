# Example flake.nix for a StripyOS configuration folder (copy, then edit the placeholders).
# Folder layout:  flake.nix  configuration.nix  stripy-apps.nix  apps.json  (contents of apps.json: [])
{
  description = "StripyOS configuration";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs, ... }: {
    # <hostname> must match the "Configuration name" in Stripy Store > Settings
    # (by default it is the machine's hostname).
    nixosConfigurations."CHANGE-ME-HOSTNAME" = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";
      modules = [
        ./configuration.nix
        ./stripy-apps.nix
        ({ ... }: {
          # Let `nix search nixpkgs` use the SAME nixpkgs as the system (no network lookup).
          nix.registry.nixpkgs.flake = nixpkgs;
          nix.settings.experimental-features = [ "nix-command" "flakes" ];
        })
      ];
    };
  };
}
