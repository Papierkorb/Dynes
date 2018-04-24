#ifndef PPU_HPP
#define PPU_HPP

#include <cstdint>
#include <QFlags>
#include <stdexcept>

namespace Ppu {
  /** Flags for the PPUCTRL register. */
  enum Control {
    /**  Should access PPUDATA increment by 1 (unset) or by 32 (set) bytes? */
    BigIncrement = (1 << 2),

    /** If set, the Sprite pattern table is at 0x1000, else at 0x0000. */
    SpriteSelect = (1 << 3),

    /** If set, the Background pattern table is at 0x1000, else at 0x0000. */
    BackgroundSelect = (1 << 4),

    /** If set, sprites are 8x16.  Else, they're 8x8 pixels. */
    BigSprites = (1 << 5),

    /** Not really useful. */
    ChipSelect = (1 << 6),

    /** Trigger a NMI of the VBlank phase? */
    NmiEnabled = (1 << 7),
  };

  Q_DECLARE_FLAGS(Controls, Control)

  /** Flags for the PPUMASK register. */
  enum Mask {
    /** Render in grayscale instead of colors? */
    Grayscale = (1 << 0),

    ShowBackgroundLeftmost = (1 << 1),
    ShowSpritesLeftmost = (1 << 2),

    /** Enable rendering of the background */
    EnableBackground = (1 << 3),

    /** Enable rendering of sprites */
    EnableSprites = (1 << 4),

    /** Emphasize red */
    EmphasizeRed = (1 << 5),

    /** Emphasize green */
    EmphasizeGreen = (1 << 6),

    /** Emphasize blue */
    EmphasizeBlue = (1 << 7),
  };

  Q_DECLARE_FLAGS(Masks, Mask)

  /** Flags for the PPUSTATUS register. */
  enum Status {
    /** Sprite overflow, more than 8 sprites on a single scanline. */
    SpriteOverflow = (1 << 5),

    /** A non-zero pixel of sprite 0 overlaps a non-zero background pixel. */
    SpriteHit = (1 << 6),

    /**
     * VBlank phase has started.  This is the only bit cleared upon reading
     * this register.
     */
    VBlankStart = (1 << 7),
  };

  Q_DECLARE_FLAGS(StatusFlags, Status)

  /** Name table mirroring modes. */
  enum Mirroring {
    /** All four name tables map to the first table. */
    Single,

    /** Name tables 2, 3 map to 0, 1 respectively. */
    Horizontal,

    /** Name tables 1, 3 map to 0, 2 respectively. */
    Vertical,

    /** Four independent name tables, no mirroring. */
    Four,
  };

  /** Attribute flags for a \c OamSprite. */
  enum OamSpriteFlag {
    PaletteLow = (1 << 0),
    PaletteHigh = (1 << 1),
    // Bits 2..4 are unused.

    /** Should the background drawn over? */
    NoPriority = (1 << 5),
    FlipHorizontal = (1 << 6),
    FlipVertical = (1 << 7),
  };

  /** OAM structure of sprites. */
  struct OamSprite {
    uint8_t y;
    uint8_t tileId;
    uint8_t attribute;
    uint8_t x;

    /** Returns the palette selected for this sprite. */
    constexpr int palette() const { return this->attribute & 3; }
  } __attribute__((packed));

  /** Scroll information. */
  union Scroll {
    struct {
      /** Scroll within a single tile. */
      uint8_t fine : 3;

      /** Tile-wise scroll. */
      uint8_t coarse : 5;

      /** Name table select. */
      uint8_t nameTable : 2;
    };

    /** Low byte. */
    uint8_t low;

    /** Full value. */
    uint16_t value;
  };

  static_assert(sizeof(OamSprite) == 4, "OAM Sprites are exactly 4 Bytes in size");

  /** Total addressable memory of the PPU. */
  static constexpr int TOTAL_SIZE = 0x4000;

  /** Memory address of the first pattern table. */
  static constexpr int PATTERN_TABLE0 = 0x0000;

  /** Memory address of the second pattern table. */
  static constexpr int PATTERN_TABLE1 = 0x1000;

  /** Base address of the four name tables. */
  static constexpr int NAME_TABLE_BASE = 0x2000;

  /** Size of a name table, in Bytes. */
  static constexpr int NAME_TABLE_SIZE = 1024; // 1KiB

  /** Size of a pattern table, in Bytes. */
  static constexpr int PATTERN_SIZE = 4096; // 4KiB

  /** Count of patterns per pattern  table. */
  static constexpr int PATTERNS_PER_TABLE = PATTERN_SIZE / 16;

  /** Size of the PPU-local RAM, in Bytes. */
  static constexpr int VRAM_SIZE = 2048; // 2KiB

  /** Size of the OAM RAM, in Bytes. */
  static constexpr int OAM_SIZE = 256; // 256B

  /** Size of the color palettes memory, in Bytes. */
  static constexpr int PALETTES_SIZE = 32; // 32B

  /** Size of a single sprite in the OAM, in Bytes. */
  static constexpr int OAM_SPRITE_SIZE = 4;

  /** Count of sprites storable in the OAM at once. */
  static constexpr int SPRITE_COUNT = OAM_SIZE / OAM_SPRITE_SIZE; // = 64

  /**
   * Total scanlines of the PPU.  This one is the same for all platforms.
   * Some scanlines may not be output in some.
   */
  static constexpr int SCANLINES = 260;

  /**
   * PPU cycles per scanline.  This one is actually platform dependent.
   * TODO: Don't hardcode this to the NTSC value.
   */
  static constexpr int CYCLES_PER_SCANLINE = 341;

  /** Number of tile rows in a name table. */
  static constexpr int NAMETABLE_ROWS = 30;

  /** Number of tile columns in a name table. */
  static constexpr int NAMETABLE_COLUMNS = 32;

  /** Max amount of sprites drawn per scanline. */
  static constexpr int SPRITES_PER_LINE = 8;

  /** Color palette of the NES. */
  static const uint32_t COLORS[] = {
  //  vv Alpha is always 0xFF
  //  AARRGGBB
    0xFF757575,
    0xFF271B8F,
    0xFF0000AB,
    0xFF47009F,
    0xFF8F0077,
    0xFFAB0013,
    0xFFA70000,
    0xFF7F0B00,
    0xFF432F00,
    0xFF004700,
    0xFF005100,
    0xFF003F17,
    0xFF1B3F5F,
    0xFF000000,
    0xFF000000,
    0xFF000000,
    0xFFBCBCBC,
    0xFF0073EF,
    0xFF233BEF,
    0xFF8300F3,
    0xFFBF00BF,
    0xFFE7005B,
    0xFFDB2B00,
    0xFFCB4F0F,
    0xFF8B7300,
    0xFF009700,
    0xFF00AB00,
    0xFF00933B,
    0xFF00838B,
    0xFF000000,
    0xFF000000,
    0xFF000000,
    0xFFFFFFFF,
    0xFF3FBFFF,
    0xFF5F97FF,
    0xFFA78BFD,
    0xFFF77BFF,
    0xFFFF77B7,
    0xFFFF7763,
    0xFFFF9B3B,
    0xFFF3BF3F,
    0xFF83D313,
    0xFF4FDF4B,
    0xFF58F898,
    0xFF00EBDB,
    0xFF000000,
    0xFF000000,
    0xFF000000,
    0xFFFFFFFF,
    0xFFABE7FF,
    0xFFC7D7FF,
    0xFFD7CBFF,
    0xFFFFC7FF,
    0xFFFFC7DB,
    0xFFFFBFB3,
    0xFFFFDBAB,
    0xFFFFE7A3,
    0xFFE3FFA3,
    0xFFABF3BF,
    0xFFB3FFCF,
    0xFF9FFFF3,
    0xFF000000,
    0xFF000000,
    0xFF000000
  };

  /** Total count of colors. */
  static const int COLOR_COUNT = sizeof(COLORS) / sizeof(*COLORS);

  /** Color palette.  Indexes 4 colors into the \c COLORS list. */
  struct Palette {
    uint8_t colors[4];

    Palette(uint8_t c[4]) : Palette(c[0], c[1], c[2], c[3]) { }
    Palette(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
      this->colors[0] = a;
      this->colors[1] = b;
      this->colors[2] = c;
      this->colors[3] = d;
    }

    /** Returns the real ARGB color value for the color at \a index. */
    uint32_t argb(int index) {
      if (index < 0 || index > 3) throw std::runtime_error("Palette.argb, index out of bounds");
      if (this->colors[index] >= COLOR_COUNT)
        return 0xFFFF0000; // Return red on OOB.
      return COLORS[this->colors[index]];
    }
  };
}

#endif // PPU_HPP
