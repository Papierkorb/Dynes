#include <ppu/renderer.hpp>

#include <cpu/base.hpp>

#if __has_cpp_attribute(fallthrough)
#  define FALLTHROUGH [[fallthrough]]
#else
#  define FALLTHROUGH
#endif

//#define DEBUG_SPRITES 0xFFFF0000

/** Returns the \a n'th bit of \a x, shifted into the lowest position. */
inline constexpr int bit(int x, int n) { return (x >> n) & 1; }
inline constexpr bool bitTest(uint8_t *bitmap, int n) { return (bitmap[n / 8] >> (n % 8)) & 1; }

namespace Ppu {
struct Sprite {
  int id = -1; ///< Index in the OAM
  int x = 0; ///< X coordinate on screen
  int y = 0; ///< Y coordinate slice in the current scan line
  uint8_t palette = 0; ///< Palette index to use
  uint8_t tileId = 0; ///< Tile id in the pattern table
  uint8_t flags = 0; ///< Flags
};

struct ScanLineTiles {
  int y; // Inner y offset in [0, 8)
  int x; // Offset, for scrolling.  Most likely negative.

  // For scrolling, prepare to store one tile more than a single name table has.
  uint8_t tiles[NAMETABLE_COLUMNS + 1];
  uint8_t palettes[NAMETABLE_COLUMNS + 4];

  bool enabled; ///< Background rendering enabled?
};

struct ScanLineSprites {
  int count = 0;
  Sprite sprites[SPRITES_PER_LINE];
  bool overflow = false;
  bool enabled; ///< Sprite rendering enabled?
  int height; ///< Sprite height, either 8 or 16 (in pixel).
};

/** Horizontal slice of a tile. */
union TileSlice {
  uint8_t row[8];
  uint64_t value;
};

static_assert(sizeof(TileSlice) == sizeof(uint64_t), "TileSlice should be 8 byte wide");

struct RendererPrivate {
  Memory *vram;
  SurfaceManager *surfaces;
  Cpu::Base *cpu;

  //
  static constexpr int PATTERN_TABLE = 0x0000;
  static constexpr int PATTERN_TABLE_SIZE = 0x1000; // 4KiB
  static constexpr int NAME_TABLE = 0x2000;
  static constexpr int NAME_TABLE_SIZE = 0x400; // 1KiB
  static constexpr int ATTR_TABLE_OFFSET = 0x3C0;
  static constexpr int ATTR_TABLE_SIZE = 64; // 64B
  static constexpr int WIDTH = 256;
  static constexpr int HEIGHT = 240;

  // Private
  uint32_t pixels[WIDTH * HEIGHT]; // TODO: Don't hardcode to NTSC
  int scanLine = 0;

  static constexpr int patternTableAddress(bool which) {
    return which ? PATTERN_TABLE1 : PATTERN_TABLE0;
  }

  /** Returns the pattern table for background tiles. */
  int backgroundPatternTable() const {
    return patternTableAddress(this->vram->control.testFlag(Ppu::BackgroundSelect));
  }

  /** Returns the pattern table for sprite tiles. */
  int spritePatternTable() const {
    return patternTableAddress(this->vram->control.testFlag(Ppu::SpriteSelect));
  }

  void fetchNameTableTiles(int base, int row, int column, int count, uint8_t *buffer) {
    int tileAddr = base + NAMETABLE_COLUMNS * row + column;

    for (int i = 0; i < count; i++, tileAddr++) {
      buffer[i] = this->vram->read(tileAddr);

      if ((tileAddr & 0x1F) == NAMETABLE_COLUMNS - 1)
        tileAddr = ((tileAddr & ~0x1F) ^ NAME_TABLE_SIZE) - 1;
      //          Set coarse X to 0 ^
      //              And then wrap to next name table ^
    }
  }

  void fetchTileAttributes(int base, int row, int column, int count, uint8_t *buffer) {
    int attrBase = base + ATTR_TABLE_OFFSET;
    int attrYOffset = (row << 1) & ~7;
    int attrXOffset = column / 4; //(column << 1) & ~7;
    int attrAddress = attrBase + attrYOffset + attrXOffset;
    int shift = (row & 2);
    int left = shift * 2;
    int right = left + 2;

    // Align columns to be multiple of 4:
    uint8_t first = this->vram->read(attrAddress);
    switch (column & 3) {
    // Abuse fall-through of switches.  This is basically a manually
    // unrolled/partial version of the for loop below.
    case 1:
      buffer[0] = (first >> left) & 3;
      buffer++;
      column++;
      count--;
      FALLTHROUGH;
    case 2:
      buffer[0] = (first >> right) & 3;
      buffer++;
      column++;
      FALLTHROUGH;
    case 3:
      buffer[0] = (first >> right) & 3;
      buffer++;
      column++;
      count--;
      attrAddress++;
      break;
    default: break; // Do nothing.
    }

    for (int i = 0; i < count; i += 4, attrAddress++) {
      uint8_t byte = this->vram->read(attrAddress);

      buffer[i + 0] = (byte >> left) & 3;
      buffer[i + 1] = (byte >> left) & 3;
      buffer[i + 2] = (byte >> right) & 3;
      buffer[i + 3] = (byte >> right) & 3;
    }
  }

  /**
   * Figures out which tiles to draw in the current scan line.
   */
  ScanLineTiles analyzeScanLineNameTable() {
    ScanLineTiles data;
    data.x = this->vram->scrollX.fine;
    data.y = this->vram->scrollY.fine;
    data.enabled = this->vram->mask.testFlag(EnableBackground);

    if (data.enabled) {
      // Fetch tile indices to render from the name table:
      int column = this->vram->scrollX.coarse;
      int row = this->vram->scrollY.coarse;
      int count = NAMETABLE_COLUMNS - column;

      int nameTable = NAME_TABLE + this->vram->scrollY.nameTable * NAME_TABLE_SIZE;
      int second = ((nameTable & ~0x1F) ^ NAME_TABLE_SIZE);
      int secondCount = NAMETABLE_COLUMNS - count;

      if (data.x) secondCount++;

      this->fetchNameTableTiles(nameTable, row, column, count + secondCount, data.tiles);

      // Fetch attributes of the tiles in this scan line:
      this->fetchTileAttributes(nameTable, row, column, count, data.palettes);
      this->fetchTileAttributes(second, row, 0, secondCount, data.palettes + count);
    }

    return data;
  }

  static Sprite spriteFromOam(int id, OamSprite oam, int scanLine) {
    Sprite s;
    s.id = id;
    s.x = oam.x;
    s.y = scanLine - oam.y;
    s.palette = oam.palette();
    s.flags = oam.attribute;
    s.tileId = oam.tileId;
    return s;
  }

  /**
   * Figures out which sprites could be drawn on the current scan line.
   */
  ScanLineSprites analyzeScanLineSprites() {
    const OamSprite *oam = this->vram->sprites();
    ScanLineSprites sprites;

    sprites.enabled = this->vram->mask.testFlag(EnableSprites);
    sprites.height = this->vram->control.testFlag(BigSprites) ? 16 : 8;

    if (sprites.enabled) {
      uint8_t min = this->scanLine - (sprites.height - 1); // Min/Max Y coordinates
      uint8_t max = this->scanLine;

      // Find sprites hitting the current scan line
      for (int i = 0; i < SPRITE_COUNT; i++) {
        if (oam[i].y > max || oam[i].y < min)
          continue; // Not in current scan line

        if (sprites.count >= SPRITES_PER_LINE) {
          sprites.overflow = true;
        } else {
          sprites.sprites[sprites.count] = spriteFromOam(i, oam[i], max);
          sprites.count++;
        }
      }
    }

    return sprites;
  }

  /**
   * Gets the horizontal 8px-wide slice in the \a index'th tile in the pattern
   * table at \a base.  The \a y'th row in the 8px-high tile is fetched.
   *
   * Pattern data supports 4 different colors per pixel.  This takes two bits to
   * represent, stored in two distinct planes.  Each plane stores one bit
   * of color information per pixel.  Each byte stores information of 8 pixels.
   * Each plane needing 8x8 Bits, or 8 Byte, make the second plane offset by
   * 8 Bytes from the beginning of the pattern data.
   *
   * The first plane stores the low bit, the second the high bit.  The color
   * is then looked up in the color palette chosen for this tile (the name table
   * uses its attribute tables, and sprites points to it in their structure).
   *
   * The lowest bit (Bit 0) corresponds to the first pixel (Pixel 0) in each
   * row.
   */
  TileSlice tileSlice(int base, int index, int y, bool flip = false) {
    if (flip) y = 7 - y;
    int address = base + index * 16 + y;
    int lo = this->vram->read(address + 0);
    int hi = this->vram->read(address + 8);

    TileSlice slice;
    for (int i = 0; i < 8; i++) slice.row[i] = bit(lo, 7 - i) | (bit(hi, 7 - i) << 1);
    return slice;
  }

  /**
   * Draws the background tiles from \a bg into the frame buffer.  Also updates
   * the \a dots bitmap, setting or clearing a bit at the same "x" depending on
   * if the pixel there was non-zero (= set), or zero (= unset).
   */
  void drawBackground(const ScanLineTiles &bg, uint8_t *dots) {
    int patterns = this->backgroundPatternTable();

    uint32_t *output = this->pixels + this->scanLine * WIDTH;
    int startX = bg.x;
    int pos = 0;
    int columnCount = NAMETABLE_COLUMNS + !!bg.x;
    // If there's fine-x scrolling, draw an extra tile ^

    Palette palettes[4] = {
      this->vram->palette(0), this->vram->palette(1), this->vram->palette(2), this->vram->palette(3)
    };

    int minPos = this->vram->mask.testFlag(ShowBackgroundLeftmost) ? 0 : 8;

    for (int column = 0; column < columnCount; column++) {
      TileSlice slice = this->tileSlice(patterns, bg.tiles[column], bg.y);
      Palette palette = palettes[bg.palettes[column]];

      uint8_t bits = 0;
      for (int x = startX; x < 8 && pos < WIDTH; x++, pos++) {
        if (pos >= minPos) {
          bits |= (!!slice.row[x]) << x;
          output[pos] = palette.argb(slice.row[x]);
        } else { // Draw backdrop color otherwise
          output[pos] = palette.argb(0);
        }
      }

      dots[column] = bits;
      startX = 0;
    }
  }

  /**
   * Checks if any opaque pixel in \a slice, positioned at \a x, collides with
   * an opaque pixel in the background.  If so, updates \c PPUSTATUS setting the
   * SpriteHit flag.
   */
  void sprite0HitTest(int x, TileSlice slice, uint8_t *dots) {
    if (slice.value == 0) return;

    for (int i = 0; i < 8 && x < WIDTH - 1; i++, x++) {
      if (slice.row[i] && bitTest(dots, x)) {
        this->vram->status.setFlag(SpriteHit, true);
        break;
      }
    }
  }

  static int tileIndex(int index, int height, int y, bool flip) {
    if (height > 8) {
      if (flip) y = height - 1 - y;

      if (y >= 8) return (index & ~1) + 1;
      else return (index & ~1);
    } else {
      return index;
    }
  }

  /**
   * Draws the sprites into the frame buffer as indicated by \a sprites.
   */
  void drawSprites(const ScanLineSprites &sprites, uint8_t *dots, bool doHitTest) {
    if (sprites.overflow) this->vram->status.setFlag(SpriteOverflow, true);
    if (sprites.count < 1) return;

    int minPos = this->vram->mask.testFlag(ShowSpritesLeftmost) ? 0 : 8;
    uint32_t *output = this->pixels + this->scanLine * WIDTH;
    Palette palettes[4] = {
      this->vram->palette(4), this->vram->palette(5), this->vram->palette(6), this->vram->palette(7)
    };

    // Grab the tile slices of all possible sprites
    TileSlice slices[8];

    for (int i = 0; i < sprites.count; i++) {
      // If we're using double-height sprites, the lowest bit of the tile id
      // determines the pattern table to use.
      // For normal-height sprites, we use the selected sprite pattern table.
      Sprite s = sprites.sprites[i];
      int patterns = (sprites.height > 8) ? patternTableAddress(s.tileId & 1) : this->spritePatternTable();
      int tileId = tileIndex(s.tileId, sprites.height, s.y, s.flags & FlipVertical);
      slices[i] = this->tileSlice(patterns, tileId, s.y & 7, s.flags & FlipVertical);

      // Do the Sprite0 hit test while we're here
      if (doHitTest && s.id == 0) this->sprite0HitTest(s.x, slices[i], dots);
    }

    // Check for every single dot in the column
    for (int x = minPos; x < WIDTH; x++) {
      int palette = 0;
      int color = 0;
      bool noPriority = false;

      // Check each sprite if it should be drawn here
      for (int i = 0; i < sprites.count; i++) {
        Sprite s = sprites.sprites[i];
        if (x < s.x || x > s.x + 7) continue; // Sprite in X-range?

        int localX = x - s.x;
        if (s.flags & FlipHorizontal) localX = 7 - localX;

        color = slices[i].row[localX]; // The color this sprite would draw

        if (color) { // Found something?
          palette = s.palette;
          noPriority = s.flags & NoPriority;
          break; // Even a no-priority sprite takes precedence
        }
      }

      // Apply this dot.
      if (color) {
        if (!noPriority || !bitTest(dots, x)) // Respect priority
          output[x] = palettes[palette].argb(color);
      }
    }
  }

  /**
   * Draws the current scan line into the frame buffer.
   */
  void handleVisibleScanLine() {
    // Bitmap of dots in this scan line.  While drawing the background, we put
    // a 1 for an opaque pixel (Color != 0), and a 0 for an "insivible" pixel
    // (Color = 0, the backdrop color).  Add an extra byte to account for an
    // extra tile for fine-X scrolling.
    uint8_t dots[WIDTH / 8 + 1]; // 256 / 8 + 1 = 65

    ScanLineTiles bg = this->analyzeScanLineNameTable();
    ScanLineSprites sprites = this->analyzeScanLineSprites();

    if (bg.enabled) this->drawBackground(bg, dots);
    if (sprites.enabled) this->drawSprites(sprites, dots, bg.enabled);

    // If neither background nor sprite rendering is enabled, fill the screen
    // with the backdrop color.
    if (!bg.enabled && !sprites.enabled) {
      uint32_t color = this->vram->palette(0).argb(0); // Get backdrop color
      for (int i = 0; i < WIDTH * HEIGHT; i++) this->pixels[i] = color;
    }

#ifdef DEBUG_SPRITES
    drawSpriteDebug(DEBUG_SPRITES);
#endif
  }

#ifdef DEBUG_SPRITES
  static constexpr int bound(int offset) {
    return (offset >= WIDTH * HEIGHT) ? (WIDTH * HEIGHT) - 1 : offset;
  }

  void drawSpriteDebug(uint32_t color) {
    const OamSprite *sprites = this->vram->sprites();
    for (int i = 0; i < SPRITE_COUNT; i++) {
      OamSprite s = sprites[i];
      if (s.x >= 0xFE || s.y >= 0xFE) continue;

      int sX = s.x; // Upcast to int
      int sY = s.y;

      for (int x = sX; x < sX + 8; x++) this->pixels[bound(x + sY * WIDTH)] = color;
      for (int x = sX; x < sX + 8; x++) this->pixels[bound(x + (sY + 8) * WIDTH)] = color;
      for (int y = sY; y < sY + 8; y++) this->pixels[bound(sX + y * WIDTH)] = color;
      for (int y = sY; y < sY + 8; y++) this->pixels[bound((sX + 8) + y * WIDTH)] = color;
    }
  }

#endif

  /**
   * Post phase, after which all visible scan lines have been drawn.
   * This method notifies the front-end of the finished frame to be displayed.
   */
  void handlePostScanLine() {
    this->surfaces->displayFrameBuffer(this->pixels);
  }

  /**
   * Pre phase, before any visible scan line.  Reinitializes some flags.
   */
  void handlePreScanLine() {
    this->vram->status.setFlag(Ppu::SpriteOverflow, false);
    this->vram->status.setFlag(Ppu::SpriteHit, false);
    this->vram->status.setFlag(Ppu::VBlankStart, false);
    this->vram->scrollY = this->vram->nextScrollY;
//    this->vram->scrollY.nameTable = this->vram->control & 3;
  }

  /**
   * NMI Phase, in which the \c PPUSTATUS register gets its VBlank flag set,
   * and the CPU is sent a NMI if the user program requested it.
   */
  void handleNmiScanLine() {
    this->vram->status.setFlag(Ppu::VBlankStart, true);

    if (this->vram->control.testFlag(Ppu::NmiEnabled)) {
      this->cpu->interrupt(Cpu::NonMaskable);
    }
  }

  void incrementScanLine() {
    this->scanLine++;
    Scroll &y = this->vram->scrollY;

    if (y.fine < 7) {
      y.fine++;
    } else {
      y.fine = 0;
      if (y.coarse == 29) {
        y.coarse = 0;
        y.nameTable ^= 2; // Flip bit1
      } else if (y.coarse == 31) {
        y.coarse = 0;
      } else {
        y.coarse++;
      }
    }
  }

  /** Processes the next scan line. */
  bool nextScanLine() {
    if (this->scanLine < 240) {
      handleVisibleScanLine();
    } else if (this->scanLine == 240) {
      handlePostScanLine();
    } else if (this->scanLine == 241) {
      handleNmiScanLine();
    } else if (this->scanLine == 261) {
      handlePreScanLine();
      this->scanLine = 0;
      return true;
    }

    this->vram->scrollX = this->vram->nextScrollX; // Prefetch
    this->incrementScanLine();
    return false;
  }

  void reset() {
    this->scanLine = 0;
    ::memset(this->pixels, 0, sizeof(this->pixels));
  }
};

Renderer::Renderer(Memory *vram, SurfaceManager *surfaces, Cpu::Base *cpu)
  : d(new RendererPrivate)
{
  this->d->vram = vram;
  this->d->surfaces = surfaces;
  this->d->cpu = cpu;

  this->d->reset();
}

Renderer::~Renderer() {
  delete this->d;
}

bool Renderer::drawScanLine() {
  return this->d->nextScanLine();
}

}
