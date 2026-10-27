{
  description = "happy-decomp: development shell for macOS (uv, upx, Python 3.12, Wine for VC6 and reccmp)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      # Gcenx's Wine is an x86_64 build: on Apple Silicon it runs under Rosetta 2.
      systems = [ "aarch64-darwin" "x86_64-darwin" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
    in
    {
      packages = forAllSystems (pkgs: {
        # nixpkgs has no Wine for Darwin. Gcenx's build (github.com/Gcenx/macOS_Wine_builds),
        # pinned by hash: the SHA-256 is the digest GitHub publishes for the release asset.
        # Wine 10 or later: WoW64 mode, which runs VC6's 32-bit exes, is built in.
        wine = pkgs.stdenvNoCC.mkDerivation rec {
          pname = "wine-stable-gcenx";
          version = "11.0_1";
          src = pkgs.fetchurl {
            url = "https://github.com/Gcenx/macOS_Wine_builds/releases/download/${version}/wine-stable-${version}-osx64.tar.xz";
            hash = "sha256-tQ3FDsf0HVixFaa2hdTRMVujx5e9OqD0khPycDy4I4g=";
          };
          sourceRoot = ".";
          dontConfigure = true;
          dontBuild = true;
          # Keep the binaries exactly as built and signed by Gcenx.
          dontFixup = true;
          installPhase = ''
            mkdir -p $out
            cp -R "Wine Stable.app" $out/
            # reccmp calls `wine` and `winepath` by name (reccmp/cvdump/runner.py, reccmp/dir.py).
            ln -s "$out/Wine Stable.app/Contents/Resources/wine/bin" $out/bin
          '';
          meta.platforms = systems;
        };
      });

      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          packages = [
            self.packages.${pkgs.stdenv.hostPlatform.system}.wine
            pkgs.python312
            pkgs.uv
            pkgs.upx
          ];

          # Python 3.12 from nixpkgs for the .venv and reccmp, never one downloaded by uv.
          # reccmp (`uv tool install reccmp==0.1.7`) is installed in the repository, in .uv/.
          shellHook = ''
            export UV_PYTHON="${pkgs.python312}/bin/python3.12"
            export UV_PYTHON_DOWNLOADS=never
            export UV_TOOL_DIR="$PWD/.uv/tools"
            export UV_TOOL_BIN_DIR="$PWD/.uv/bin"
            export PATH="$UV_TOOL_BIN_DIR:$PATH"
            # No noise and no prompt to install Mono or Gecko (also set by scripts/vc6.py,
            # but reccmp runs cvdump through Wine on its own).
            export WINEDEBUG=-all
            export WINEDLLOVERRIDES="mscoree,mshtml="
            # MoltenVK (bundled with Gcenx's Wine) logs [mvk-info] lines on stderr at every launch.
            export MVK_CONFIG_LOG_LEVEL=0
          '';
        };
      });
    };
}
