{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  packages = [
    pkgs.cmake
    pkgs.gcc
    pkgs.vulkan-headers
    pkgs.vulkan-loader
    pkgs.vulkan-validation-layers
    pkgs.vulkan-tools
    pkgs.vulkan-helper
    pkgs.vulkan-volk
    pkgs.glm
    pkgs.cmake
    pkgs.ninja
    pkgs.glfw
    pkgs.tree
    pkgs.gcc
  ];
}

