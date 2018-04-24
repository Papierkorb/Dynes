# DyNES, a dynamically-recompiling NES

An **extensible**, **dynamically recompiling** NES emulator.  As study on how to
build it.  May also serve as sample how to architecture a project like this.

Written in C++ using Qt5 for GUI.

## Building

You can open the `Dynes.pro` in the QtCreator IDE, or run these commands:

```sh
# Create a build directory
mkdir build
cd build

# Run QMake
qmake ..

# Build
make -j$(nproc)
```

### Configuration

You can enable specific CPU cores at build-time by supplying the name to `qmake`
like so: `qmake .. "CONFIG+=dynes_core_lua"`.  Repeat the CONIFG argument for
multiple cores.

## CPU cores

Right now, there are four to choose from:

### Interpret

Standard interpret.  While a modern computer will have no issues running it
at full speed, it's not optimized for speed.  It's pretty simple however.

Built-in and can't be disabled.

### AMD64 Dynarec

Fully self-contained AMD64 dynarec.  Only works on AMD64 (`x86_64`) machines.

**Requires** Nothing.

**Config option** is `dynes_core_dynarec_amd64`

### LLVM Dynarec

A LLVM-based dynamic recompiler.  Uses LLVM to dynamically recompile code from
6502 machine code to LLVM bytecode, and then using a JIT, to what the host
platform uses.

**Requires** LLVM 4.0

**Config option** is `dynes_core_dynarec_llvm` (Disabled by default)

It is possible to use this core with LLVM versions 5.0 and 6.0 when using
a debug-build instead of a release-build: `qmake .. "CONFIG+=debug" ...`.

### Lua Dynarec

An experiment what happens if you use Lua (A simple yet fully-featured scripting
language) as run-time.  Dynamically recompiles from 6502 machine code to Lua
script, and then calls it.  It offers surprisingly good performance.

**Requires** Lua version 5.3 *or* LuaJit

**Config option** is `dynes_core_lua` (Disabled by default)

## Cartridge mappers

Only **NROM** and **MMC1** are supported.

## Missing

* Sound
* More cartridge mappers
