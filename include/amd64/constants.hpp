#ifndef AMD64_CONSTANTS_HPP
#define AMD64_CONSTANTS_HPP

#include "assembler.hpp"

namespace Amd64 {

// Register mapping.  Chosen so that the important 6502 registers fall into
// callee-saved AMD64 registers, hence when CALLing, we don't have to save them.
static constexpr Register A = BL;
static constexpr Register X = BH;
static constexpr Register Y = R12B;
static constexpr Register YX = R12W;
static constexpr Register S = R13B;
static constexpr Register SR = R13;
static constexpr Register P = R14B;
static constexpr Register PX = R14W;
static constexpr Register CYCLES = R15D;

// These registers only matter when returning to the host, and are set just
// before doing so.  They can be in the unsafe region.
static constexpr Register PC = CX;
static constexpr Register REASON = AL;

// As can the scratch registers
static constexpr Register UX = CX;
static constexpr Register UH = CH;
static constexpr Register UL = CL;
static constexpr Register VL = R8B;
static constexpr Register VX = R8W;
static constexpr Register WL = R9B;
static constexpr Register WX = R9W;

// Special register to temporarily hold data read from memory.  Aliased to RAX,
// so we don't have to move it around at all when CALL'ing into the memory
// function.
static constexpr Register MEML = AL;
static constexpr Register MEMH = AH;
static constexpr Register MEMX = AX;
static constexpr Register ADDR = R10W;
static constexpr Register ADDRR = R10;

// UNIX ABI:
static constexpr Register ARG_1 = RDI; // Memory*  mem
static constexpr Register ARG_2R = RSI;
static constexpr Register ARG_2 = SI;  // uint16_t addr
static constexpr Register ARG_2L = SIL;  //
static constexpr Register ARG_3 = DL;  // uint8_t  value
static constexpr Register ARG_4 = CX;  // uint16_t op (For logging)
static constexpr Register RESULT8 = AL;  // uint8_t
static constexpr Register RESULT16 = AX;  // uint16_t

}

#endif // AMD64_CONSTANTS_HPP
