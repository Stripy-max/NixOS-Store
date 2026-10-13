# Stripy Store 0.2

Native (Qt 6 / QML + C++) application store for StripyOS. Installs are **system-wide and declarative**:
the store edits `apps.json` in your configuration folder and runs `nixos-rebuild switch` through `pkexec`.

## Setup (one time)
1. Copy `nix/stripy-apps.nix` into your configuration folder, create `apps.json` containing `[]`
   (or press *Create apps.json* in Settings), and add the module to your flake — see `nix/example-flake.nix`.
2. In Stripy Store → Settings, check the folder and the configuration name (`nixosConfigurations.<name>`).
   Defaults: `$STRIPY_DIR` or `~/stripyos` or `/etc/stripyos`; name = hostname (`$STRIPY_HOST`).
3. Build: `nix-build -E 'with import <nixpkgs> {}; callPackage ./nix/package.nix {}'`

## Honest state model
* **Requested** = `apps.json`. **Applied** = recorded only after a rebuild exited with code 0.
* Installed = requested ∧ applied. Otherwise: *pending*, *failed*, or *not verified* (no baseline yet).
* A failed install/remove restores `apps.json` only if it still contains exactly what the store wrote.
  A failed rebuild does not roll the running system back; it simply never switched.

## Policy
* Non-free packages are allowed only if explicitly requested (`allowUnfreePredicate`, `mkDefault`).
  If you define your own `allowUnfreePredicate`/`allowUnfree`, it replaces this policy.
* The catalog (`data/catalog.json`, schema 2) can be overridden by `$STRIPY_CATALOG` or `<config folder>/catalog.json`.

## Tests
`cmake -S . -B build -DSTRIPY_BUILD_APP=OFF && cmake --build build && ctest --test-dir build` (Qt-free: logic + catalog schema).
Maintainers with Nix: `tools/check_catalog_attrs.sh` verifies every catalog attribute exists.
