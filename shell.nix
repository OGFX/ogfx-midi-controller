{ pkgs ? import <nixpkgs> {} }:

(pkgs.buildFHSUserEnv {
  name = "simple-x11-env";
  targetPkgs = pkgs: (with pkgs;
    [ udev
      alsaLib
      fontconfig
      zlib
      udev
      gtk2
      glib
      pango
      cairo
      gdk-pixbuf
    ]) ++ (with pkgs.xorg;
    [ libX11
      libXcursor
      libXrandr
      libXext
      libXtst
      libXi
      libXft
      libXxf86vm
      libSM
    ]);
  multiPkgs = pkgs: (with pkgs;
    [ udev
      alsaLib
      fontconfig
      zlib
    ]);
  runScript = "bash";
}).env

