TARGET   = dynes
TEMPLATE = lib
CONFIG  += staticlib

QT      += core
include(../common.pri)

###
SOURCES += \
  src/core/configuration.cpp \
  src/core/data.cpp \
  src/core/disassembler.cpp \
  src/core/inesfile.cpp \
  src/core/instruction.cpp \
  src/core/runner.cpp \
  src/core/gamepad.cpp \
  src/cpu/base.cpp \
  src/cpu/dumphook.cpp \
  src/cpu/hook.cpp \
  src/cpu/memory.cpp \
  src/cpu.cpp \
  src/cartridge/base.cpp \
  src/cartridge/nrom.cpp \
  src/cartridge/mmc1.cpp \
  src/ppu/memory.cpp \
  src/ppu/renderer.cpp \
  src/analysis/function.cpp \
  src/analysis/branch.cpp \
  src/analysis/functiondisassembler.cpp \
  src/analysis/conditionalinstruction.cpp

HEADERS += \
  include/core/configuration.hpp \
  include/core/data.hpp \
  include/core/disassembler.hpp \
  include/core/inesfile.hpp \
  include/core/instruction.hpp \
  include/core/runner.hpp \
  include/core/gamepad.hpp \
  include/cpu/base.hpp \
  include/cpu/dumphook.hpp \
  include/cpu/hook.hpp \
  include/cpu/state.hpp \
  include/cpu.hpp \
  include/cpu/memory.hpp \
  include/cartridge/base.hpp \
  include/cartridge/nrom.hpp \
  include/cartridge/mmc1.hpp \
  include/ppu/surfacemanager.hpp \
  include/ppu/memory.hpp \
  include/ppu/renderer.hpp \
  include/ppu.hpp \
  include/analysis/function.hpp \
  include/analysis/branch.hpp \
  include/analysis/functiondisassembler.hpp \
  include/analysis/conditionalinstruction.hpp \
  include/analysis/repository.hpp

### Interpret

HEADERS += include/interpret/core_interpret.hpp
SOURCES += src/interpret/core.cpp

### Dynarec/LLVM

CONFIG(dynes_core_dynarec_llvm) {
HEADERS += \
    include/dynarec/function.hpp \
    include/dynarec/common.hpp \
    include/dynarec/structtranslator.hpp \
    include/dynarec/functioncompiler.hpp \
    include/dynarec/compiler.hpp \
    include/dynarec/functionframe.hpp \
    include/dynarec/instructiontranslator.hpp \
    include/dynarec/memorytranslator.hpp \
    include/dynarec/configuration.hpp \
    include/dynarec/orcexecutor.hpp \
    include/dynarec/core_dynarec.hpp

SOURCES += \
    src/dynarec/function.cpp \
    src/dynarec/core.cpp \
    src/dynarec/structtranslator.cpp \
    src/dynarec/common.cpp \
    src/dynarec/functioncompiler.cpp \
    src/dynarec/compiler.cpp \
    src/dynarec/functionframe.cpp \
    src/dynarec/instructiontranslator.cpp \
    src/dynarec/memorytranslator.cpp \
    src/dynarec/configuration.cpp \
    src/dynarec/orcexecutor.cpp
}

### Lua

CONFIG(dynes_core_lua) {
HEADERS += \
    include/lua/core_lua.hpp \
    include/lua/codegenerator.hpp \
    include/lua/function.hpp

SOURCES += \
    src/lua/core.cpp \
    src/lua/codegenerator.cpp \
    src/lua/function.cpp
}

### Dynarec/AMD64

CONFIG(dynes_core_dynarec_amd64) {
HEADERS += \
    include/amd64/linker.hpp \
    include/amd64/assembler.hpp \
    include/amd64/executablememory.hpp \
    include/amd64/linker.hpp \
    include/amd64/core_amd64.hpp \
    include/amd64/functiontranslator.hpp \
    include/amd64/memorymanager.hpp \
    include/amd64/symbolregistry.hpp \
    include/amd64/instructiontranslator.hpp \
    include/amd64/memorytranslator.hpp \
    include/amd64/function.hpp \
    include/amd64/constants.hpp

SOURCES += \
    src/amd64/linker.cpp \
    src/amd64/assembler.cpp \
    src/amd64/memorymanager.cpp \
    src/amd64/symbolregistry.cpp \
    src/amd64/core.cpp \
    src/amd64/functiontranslator.cpp \
    src/amd64/executablememory.cpp \
    src/amd64/executablememory_linux.cpp \
    src/amd64/function.cpp \
    src/amd64/instructiontranslator.cpp \
    src/amd64/memorytranslator.cpp \
    src/amd64/guest_call.s
}
