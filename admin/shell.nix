{ pkgs ? import <nixos2105> {} }:
with pkgs;
mkShell {
  nativeBuildInputs = [
    python2
    python2Packages.ldap
    python2Packages.requests
    python2Packages.pyserial
  ];
}
