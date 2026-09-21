#!/usr/bin/env python3
"""Generate sce_sys/icon0.png (512x512) for Fireplace5: a fire still with
"FIREPLACE5" in the same chrome-gradient 8x8 lettering the app uses.

Usage: make_icon.py <fire-background.png> <out.png>
Reproducible: parses the app's own font8x8_basic.h and chrome palette.
"""
import re
import sys
from PIL import Image

HERE = __file__.rsplit("/", 1)[0]
FONT_H = HERE + "/../src/font8x8_basic.h"

# Chrome gradient per glyph row, matching chrome[8] in src/main.cpp.
CHROME = [
    (0x10, 0x28, 0x70), (0x30, 0x60, 0xc0), (0x80, 0xb0, 0xf0),
    (0xd8, 0xe8, 0xff), (0xff, 0xff, 0xff), (0x48, 0x78, 0xd0),
    (0x20, 0x48, 0xa0), (0x10, 0x28, 0x70),
]


def load_font():
    """Parse font8x8_basic[128][8] into a list of 8 byte-rows per glyph."""
    text = open(FONT_H).read()
    body = text[text.index("{", text.index("font8x8_basic")):]
    rows = re.findall(r"\{([^}]*)\}", body)
    glyphs = []
    for r in rows[:128]:
        vals = [int(v, 16) for v in re.findall(r"0[xX][0-9a-fA-F]+", r)]
        glyphs.append(vals if len(vals) == 8 else [0] * 8)
    return glyphs


def render(bg_path, out_path):
    glyphs = load_font()
    text = "FIREPLACE5"

    # Work on a 256x256 low-res canvas (chunky pixels), then scale to 512.
    W = H = 256
    # Center-crop the fire background to square and fit the canvas.
    bg = Image.open(bg_path).convert("RGB")
    s = min(bg.size)
    bg = bg.crop(((bg.width - s) // 2, (bg.height - s) // 2,
                  (bg.width + s) // 2, (bg.height + s) // 2)).resize((W, H))
    px = bg.load()

    # Two passes (outline, then chrome fill) like draw_scroller/draw_text.
    scale = 3                       # each glyph-pixel is 3x3 on the canvas
    tw = len(text) * 8 * scale
    x0 = (W - tw) // 2
    y0 = int(H * 0.40)              # a bit below center, above the flames

    def block(x, y, c):
        """Fill a scale x scale block at glyph-pixel (x,y)."""
        for yy in range(scale):
            for xx in range(scale):
                px_x, px_y = x + xx, y + yy
                if 0 <= px_x < W and 0 <= px_y < H:
                    px[px_x, px_y] = c

    for pass_ in (0, 1):
        for ci, ch in enumerate(text):
            g = glyphs[ord(ch)]
            for col in range(8):
                for row in range(8):
                    if not (g[row] & (1 << col)):
                        continue
                    bx = x0 + (ci * 8 + col) * scale
                    by = y0 + row * scale
                    if pass_ == 0:
                        # 1px outline around the whole scaled block.
                        for dy in (-1, 0, 1):
                            for dx in (-1, 0, 1):
                                block(bx + dx, by + dy, (0, 0, 0))
                    else:
                        block(bx, by, CHROME[row])

    bg.resize((512, 512), Image.NEAREST).save(out_path)
    print("wrote", out_path)


if __name__ == "__main__":
    render(sys.argv[1], sys.argv[2])
