CONFIG  += object_parallel_to_source
DEFINES += QT_DEPRECATED_WARNINGS
QMAKE_CXXFLAGS += -std=c++17 -ffunction-sections -fdata-sections
DEFINES += _GNU_SOURCE __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS __STDC_LIMIT_MACROS

INCLUDEPATH += $$PWD/lib/include
LIBS += -L$$OUT_PWD/../lib

# Build-time configuration
#CONFIG += dynes_core_dynarec_llvm   # LLVM Dynarec (LLVM 4.0)
#CONFIG += dynes_core_lua            # Lua Dynarec (Lua 5.3+)
CONFIG += dynes_core_dynarec_amd64  # AMD64 Dynarec (No dependencies!)

###

CONFIG(dynes_core_dynarec_llvm) {
DEFINES += NDEBUG LLVM_BUILD_GLOBAL_ISEL
LIBS += $$system(llvm-config --libs)

DEFINES += DYNES_CORE_DYNAREC_LLVM
}

CONFIG(dynes_core_lua) {
DEFINES += DYNES_CORE_LUA
LIBS += -llua
}

CONFIG(dynes_core_dynarec_amd64) {
DEFINES += DYNES_CORE_DYNAREC_AMD64
}
