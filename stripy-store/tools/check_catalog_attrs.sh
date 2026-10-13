#!/bin/sh
# Maintainer tool (needs Nix + network/cache): checks every catalog attribute exists in nixpkgs.
# Usage: tools/check_catalog_attrs.sh [nixpkgs-flake-ref]   (default: nixpkgs)
ref="${1:-nixpkgs}"; bad=0
for a in $(python3 -c "import json;print(*[x['attr'] for x in json.load(open('data/catalog.json'))['apps']])"); do
  if nix --extra-experimental-features 'nix-command flakes' eval --raw "$ref#$a.name" >/dev/null 2>&1; then :; else echo "MISSING: $a"; bad=1; fi
done
exit $bad
