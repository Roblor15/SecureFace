{ pkgs ? import <nixpkgs> {} }:
let
  recognition = pkgs.python3.withPackages (p: with p; [
    tblib
    face_recognition
    opencv4
  ]);
in
recognition.env # replacement for pkgs.mkShell
