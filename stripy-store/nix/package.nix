{ stdenv, lib, cmake, python3, qt6 }:
stdenv.mkDerivation {
  pname = "stripy-store";
  version = "0.2.0";
  src = lib.cleanSource ../.;
  nativeBuildInputs = [ cmake qt6.wrapQtAppsHook ];
  nativeCheckInputs = [ python3 ];
  buildInputs = [ qt6.qtbase qt6.qtdeclarative qt6.qtsvg qt6.qtwayland ];
  doCheck = true;                       # runs tests/test_logic and the catalog validator
  meta = {
    description = "Application store for StripyOS";
    mainProgram = "stripy-store";
    platforms = lib.platforms.linux;
  };
}
