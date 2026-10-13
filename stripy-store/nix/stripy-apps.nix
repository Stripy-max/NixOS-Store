# StripyOS apps module. Import it from your flake/configuration (see example-flake.nix).
# Stripy Store edits apps.json (a JSON list of nixpkgs attribute names) and rebuilds the system.
{ config, lib, pkgs, ... }:
let
  cfg = config.stripy.apps;
  names = builtins.fromJSON (builtins.readFile cfg.file);
  validName = n: builtins.isString n
    && builtins.match "[A-Za-z0-9_][A-Za-z0-9_'+-]*(\\.[A-Za-z0-9_][A-Za-z0-9_'+-]*)*" n != null;
  resolve = n: lib.attrByPath (lib.splitString "." n) null pkgs;
  usable = builtins.filter (n: validName n && resolve n != null) names;
  missing = builtins.filter (n: !(validName n) || resolve n == null) names;
  resolved = map resolve usable;
in {
  options.stripy.apps = {
    enable = lib.mkOption { type = lib.types.bool; default = true; description = "Install the applications listed in apps.json."; };
    file = lib.mkOption { type = lib.types.path; default = ./apps.json; description = "JSON list of nixpkgs attribute names managed by Stripy Store."; };
    extraUnfree = lib.mkOption {
      type = lib.types.listOf lib.types.str;
      default = [ "steam" "steam-original" "steam-unwrapped" "steam-run" ];
      description = "Additional non-free package names allowed because they are runtime parts of requested applications.";
    };
  };

  config = lib.mkIf cfg.enable {
    assertions = [{
      assertion = missing == [ ];
      message = "StripyOS apps.json lists unknown or invalid package(s): ${lib.concatStringsSep ", " missing}";
    }];
    environment.systemPackages = resolved;
    # Deliberate policy: non-free packages are allowed only when explicitly requested
    # (or listed in extraUnfree). Defining your own allowUnfreePredicate/allowUnfree overrides this.
    nixpkgs.config.allowUnfreePredicate =
      lib.mkDefault (pkg: builtins.elem (lib.getName pkg) ((map lib.getName resolved) ++ cfg.extraUnfree));
  };
}
