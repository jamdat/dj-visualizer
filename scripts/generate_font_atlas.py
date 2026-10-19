"""
Generates a multi-channel Signed Distance Field (SDF) + Alpha + Bevel font atlas
texture for the 4 retro arcade / neon concert fonts.

Output:
  - assets/textures/font_atlas.png (1024 x 1536 RGBA)
  - Channels:
      R: SDF (128 = edge, >128 inside glyph, <128 outside)
      G: Bevel / Crest height (0.0 to 1.0)
      B: Rim highlight mask
      A: Crisp anti-aliased binary alpha mask
"""

import os
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import scipy.ndimage

FONTS = [
    ("NeonSans", "assets/fonts/NeonSans.ttf", 64),
    ("Ryga", "assets/fonts/Ryga.ttf", 60),
    ("RasterForge", "assets/fonts/RasterForge.ttf", 54),
    ("AtariClassic", "assets/fonts/AtariClassic.ttf", 46)
]

COLS = 16
ROWS = 6
CELL_W = 64
CELL_H = 64
FONT_H = ROWS * CELL_H  # 384
ATLAS_W = COLS * CELL_W # 1024
ATLAS_H = FONT_H * len(FONTS) # 1536

def generate_font_atlas(output_path):
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    
    # 1. Render hi-res raster bitmap masks for each glyph
    mask_img = Image.new('L', (ATLAS_W, ATLAS_H), 0)
    draw = ImageDraw.Draw(mask_img)

    for f_idx, (fname, fpath, fsize) in enumerate(FONTS):
        font = ImageFont.truetype(fpath, fsize)
        y_offset = f_idx * FONT_H
        for ascii_code in range(32, 128):
            idx = ascii_code - 32
            col = idx % COLS
            row = idx // COLS
            x0 = col * CELL_W
            y0 = y_offset + row * CELL_H
            char = chr(ascii_code)

            bbox = draw.textbbox((0, 0), char, font=font)
            bw = bbox[2] - bbox[0]
            bh = bbox[3] - bbox[1]

            x_draw = x0 + (CELL_W - bw) / 2 - bbox[0]
            y_draw = y0 + (CELL_H - bh) / 2 - bbox[1]

            draw.text((x_draw, y_draw), char, font=font, fill=255)

    mask = np.array(mask_img, dtype=np.float32) / 255.0
    inside = mask > 0.5
    outside = ~inside

    # 2. Euclidean Distance Transforms for SDF and Bevel
    print("[Atlas] Computing Euclidean Distance Transform...")
    dist_inside = scipy.ndimage.distance_transform_edt(inside)
    dist_outside = scipy.ndimage.distance_transform_edt(outside)

    # Signed distance: positive inside, negative outside, mapped so 0 dist = 128
    spread = 6.0
    signed_dist = dist_inside - dist_outside
    sdf = np.clip(128.0 + (signed_dist / spread) * 127.0, 0, 255).astype(np.uint8)

    # Bevel channel: crest height from edge inward (0 at edge to 255 at peak)
    max_bevel_dist = 4.0
    bevel = np.clip((dist_inside / max_bevel_dist) * 255.0, 0, 255).astype(np.uint8)

    # Rim mask: sharp highlight band right at the bevel transition
    rim = np.where((bevel > 25) & (bevel < 180), 255, 0).astype(np.uint8)

    # Alpha mask: anti-aliased edge
    alpha = np.clip(mask * 255.0, 0, 255).astype(np.uint8)

    rgba = np.stack([sdf, bevel, rim, alpha], axis=-1)
    out_img = Image.fromarray(rgba, 'RGBA')
    out_img.save(output_path)
    tga_path = os.path.splitext(output_path)[0] + '.tga'
    out_img.save(tga_path)
    print(f"[Atlas] Saved {output_path} and {tga_path} ({ATLAS_W}x{ATLAS_H} RGBA)")

if __name__ == '__main__':
    out1 = 'assets/textures/font_atlas.png'
    out2 = 'build/bin/assets/textures/font_atlas.png'
    generate_font_atlas(out1)
    if os.path.exists('build/bin'):
        generate_font_atlas(out2)
