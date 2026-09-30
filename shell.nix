{ pkgs ? import (builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/25.05.tar.gz";
  }) {}
}:

 (pkgs.mkShell.override { stdenv = pkgs.gcc13Stdenv; })   {
  name = "snipersim-env";

  nativeBuildInputs = with pkgs; [
    binutils
    gnumake
    cmake       # builds pimsim (linked into sniper for the PIM bridge)
    gcc13
    curl
    git
    wget
    pkg-config
    (python3.withPackages (ps: with ps; [ numpy ]))
    which
    # RISC-V cross compiler for pimsim's DPU kernels (riscv32-none-elf-gcc)
    pkgsCross.riscv32-embedded.buildPackages.gcc
  ];

  buildInputs = with pkgs; [
    boost
    bzip2
    sqlite
    ncurses
    zlib
    zlib.dev
  ];

  shellHook = ''
    export SNIPER_ROOT=$PWD

    # The RISC-V cross compiler's setup hook claims CC, CXX, LD, AR and the
    # rest for riscv32-none-elf-*. Sniper bakes $CC/$CXX into the generated
    # config/buildconf.* and would build itself with the cross compiler, so
    # point the standard names back at the native toolchain. The kernels use
    # the prefixed names (sw/dpu_runtime/dpu.mk), which are still on the PATH.
    export CC=gcc CXX=g++ CPP=cpp
    export LD=ld AR=ar AS=as NM=nm RANLIB=ranlib STRIP=strip
    export OBJCOPY=objcopy OBJDUMP=objdump READELF=readelf
    export SIZE=size STRINGS=strings

    #    export NEWER_STDCXX=${pkgs.gcc13.cc.lib}/lib
    # Newer libstdc++ for host-built apps (avantgraph etc.) needing GLIBCXX_3.4.31/32.
    echo "  build:  make -j$(nproc)"
    echo "  test:   cd test/fft && make run"
    echo "  app preload: LD_PRELOAD=\$NEWER_STDCXX/libstdc++.so.6 run-sniper ..."
  '';
}
