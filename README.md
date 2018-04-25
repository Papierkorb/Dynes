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

## Testing

Tests are done by using test ROMs, which can be automatically ran an verified
on all (compiled) CPU cores.  These automatic verifications are called
"casettes", and reside in `test/casettes`.

### How to run

Build the project like shown above.  Then run the `test` binary, supplying it
with the paths to the casette files.  All casettes will be ran against all CPU
cores.

```
# Make use of your shells path expansion feature to run all casettes:
$ build/test/test test/casettes/*.conf
```

### Casette files

Casette configuration files end in `.conf` and are text- and line-based.
Each line describes a command to execute.  Command names and arguments are
*case-insensitive*, except for path names.  Commands and arguments are
separated by one (or multiple) horizontal space(s).  The end of a line marks
the end of a command.

Commands accepting a path name will resolve it relative to the current casette
file.

Empty lines, and lines beginning with `#`, are ignored.

```
# This is ignored.
   # As is this, and the following empty line too:

# Comments MUST start at the beginning, the following will NOT work:
OPEN foo.nes  # Some comment.
```

In syntax descriptions `<x>` means that `x` is a mandatory argument and `[x]`
is an optional argument.  The brackets are not to be written and only serve
explanatory purposes in this document. A `...` means that the previous argument
can be repeated as often as you want.  A pipe (`|`), like in `[x|y]`, means that
either `x` or `y` are expected.  Full upper-cased `[KEY|WORDS]` mean that one of
the keywords is expected, otherwise it's a free-form argument.

#### Commands

**ONFAIL** sets a custom message to be shown if the *following* command fails.
If the following command succeeds, the message is discarded.

Syntax: `ONFAIL <Message>`

**OPEN** opens a .nes ROM.  Expects the path to the ROM file as argument.

Syntax: `OPEN <Path to .nes>`

```
ONFAIL Uses the test by foobar, download from http://example.com
OPEN foobar.nes
```

**ADVANCE** advances the emulator by one or more frames.  Optionally, one or
more buttons can be pressed.  These will be held down during the whole duration
of the command, and will be released afterwards.

Syntax: `ADVANCE [Count of frames, defaults to 1] [UP|DOWN|LEFT|RIGHT|A|B|START|SELECT] ...`

```
# Advances one frame:
ADVANCE

# Advances 60 frames (= one second):
ADVANCE 60

# Advances one frame with LEFT+A pressed:
ADVANCE LEFT A

# Advances 10 frames while pressing START+DOWN
ADVANCE 10 START DOWN
```

**FRAME** saves the current displayed framebuffer as `.bmp` file onto the disk.
The file name can contain a `%` which will be replaced by the name of the
current core.  This command can be used to generate the pictures for a
**COMPARE**.

Syntax: `FRAME <FileName.bmp>`

```
# Stores the current frame into foo.bmp.  If ran with multiple cores they'll
# override each others file!
FRAME foo.bmp

# Stores the current frame into "foo_%.bmp", where "%" will be replaced by the
# current core's name.  May produce "foo_interpret.bmp" and "foo_amd64.bmp".
FRAME foo_%.bmp
```

**COMPARE** compares the current displayed framebuffer with a given `.bmp` file.
The file name can also contain `%`, and works just like in **FRAME**.  This
command is best used with a **FRAME** preceeding it to ease debugging output
issues.

Syntax: `COMPARE <FileName.bmp>`

```
# Compares the current frame with foo.bmp:
COMPARE foo.bmp

# Compares the current frame with foo_interpret.bmp, foo_amd64.bmp, etc.
COMPARE foo_%.bmp
```

## Missing

* Sound
* More cartridge mappers

## License

This project is licensed under the **GPLv3**.
