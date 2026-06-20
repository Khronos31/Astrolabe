#!/usr/bin/env python3
"""
Astrolabe background — 240×240 RGB565
Authentic stereographic-projection layout for latitude 40°N.

Visual elements:
  Plate : almucantars every 5° (h=0..80°), azimuth lines every 10°,
          tropic circles (cancer / equator / capricorn)
  Rete  : outer structural ring, ecliptic ring (offset), 24 star-pointer tabs
  Limb  : double-ring with 1° / 5° / 10° / 30° ticks
"""

import math
from PIL import Image, ImageDraw

# ── canvas ────────────────────────────────────────────────────────
SIZE = 240
CX = CY = 120

# ── geometry ──────────────────────────────────────────────────────
PHI      = math.radians(40)    # observer latitude
R_EQ     = 60                  # equator circle radius (pixels)
R_DISC   = 116                 # outer edge of physical disc
R_LIMB   = 112                 # inner edge of limb graduation zone
R_RETE   = 98                  # outer rete ring radius
R_ICONS  = 95                  # icon orbit (subtle guide)
R_PLATE  = R_RETE - 4         # plate clip radius

# ── palette ───────────────────────────────────────────────────────
BG           = (10,   7,   2)
BRASS_BRIGHT = (212, 175,  55)
BRASS_GOLD   = (184, 134,  11)
BRASS_MID    = (139, 102,  30)
BRASS_DARK   = ( 80,  58,  10)
BRASS_DEEP   = ( 48,  34,   6)
BRASS_SUBTLE = ( 24,  17,   3)

PLATE_HI  = ( 65,  47,   9)   # almucantar / azimuth bright
PLATE_MID = ( 40,  29,   5)   # medium plate lines
PLATE_LO  = ( 26,  18,   3)   # faint plate lines

RETE_GOLD  = (196, 152,  28)
RETE_DIM   = ( 90,  65,  12)
ECLIP_COL  = (160, 118,  18)
TROPIC_COL = ( 70,  50,   8)

# ── helpers ───────────────────────────────────────────────────────

def c(cx, cy, r, col, w=1):
    draw.ellipse([cx-r, cy-r, cx+r, cy+r], outline=col, width=w)

def line_pr(cx, cy, r1, r2, deg, col, w=1):
    rad = math.radians(deg)
    co, si = math.cos(rad), math.sin(rad)
    draw.line([cx+r1*co, cy+r1*si, cx+r2*co, cy+r2*si], fill=col, width=w)


# ── stereographic projection helpers ─────────────────────────────

def almucantar(altitude_deg, phi=PHI, req=R_EQ):
    """Return (cx_screen, cy_screen, r) for almucantar at given altitude."""
    h   = math.radians(altitude_deg)
    den = math.sin(h) + math.sin(phi)
    if den < 1e-9:
        return None
    r   = req * math.cos(h) / den
    # centre is displaced north (= upward on screen) from disc centre
    yc  = CY - req * math.cos(phi) / den
    return (CX, yc, r)


def zenith_point(phi=PHI, req=R_EQ):
    """Projected zenith (x, y) on the screen."""
    dz = req * math.cos(phi) / (1.0 + math.sin(phi))
    return (CX, CY - dz)


# ─────────────────────────────────────────────────────────────────
img  = Image.new("RGB", (SIZE, SIZE), BG)
draw = ImageDraw.Draw(img)

# We draw everything, then clip to the disc circle.
# To get the plate mesh clipped to R_PLATE, we use a separate plate layer.

plate_img  = Image.new("RGB", (SIZE, SIZE), BG)
plate_draw = ImageDraw.Draw(plate_img)

# ═══════════════════════════════════════════════════════════════════════
# PLATE LAYER — almucantar circles + azimuth lines
# ═══════════════════════════════════════════════════════════════════════

ZX, ZY = zenith_point()   # projected zenith

# Azimuth lines every 10° (drawn first, behind almucantars)
# Approximated as straight lines from zenith toward the plate boundary.
# Real azimuth circles would be arcs, but lines give the right visual "spoke" feel.
for az in range(0, 360, 10):
    rad = math.radians(az)
    # Direction from zenith outward in that azimuth
    # On plate: N=up, E=right
    dx =  math.sin(rad)
    dy = -math.cos(rad)

    # Extend to beyond disc edge
    ext = 160
    x2 = ZX + ext * dx
    y2 = ZY + ext * dy

    is_card = (az % 90 == 0)
    col = PLATE_HI if is_card else (PLATE_MID if az % 30 == 0 else PLATE_LO)
    w   = 2 if is_card else 1
    plate_draw.line([ZX, ZY, x2, y2], fill=col, width=w)

# Almucantar circles: every 5° altitude, 0° to 80°
for alt in range(0, 85, 5):
    res = almucantar(alt)
    if res is None:
        continue
    acx, acy, ar = res
    is_dec = (alt % 10 == 0)
    col  = PLATE_HI if is_dec else PLATE_LO
    w    = 2 if (alt % 30 == 0) else 1
    # draw as full circle (clipping will hide what's outside the plate)
    plate_draw.ellipse([acx-ar, acy-ar, acx+ar, acy+ar], outline=col, width=w)

# Tropic of Capricorn (δ = −23.5°)
r_tc = R_EQ * math.cos(math.radians(23.5)) / (1 - math.sin(math.radians(23.5)))
plate_draw.ellipse([CX-r_tc, CY-r_tc, CX+r_tc, CY+r_tc], outline=TROPIC_COL, width=1)

# Equator
plate_draw.ellipse([CX-R_EQ, CY-R_EQ, CX+R_EQ, CY+R_EQ], outline=BRASS_DARK, width=1)

# Tropic of Cancer (δ = +23.5°)
r_tn = R_EQ * math.cos(math.radians(23.5)) / (1 + math.sin(math.radians(23.5)))
plate_draw.ellipse([CX-r_tn, CY-r_tn, CX+r_tn, CY+r_tn], outline=TROPIC_COL, width=1)

# Clip plate layer to R_PLATE circle
plate_mask = Image.new("L", (SIZE, SIZE), 0)
ImageDraw.Draw(plate_mask).ellipse(
    [CX-R_PLATE, CY-R_PLATE, CX+R_PLATE, CY+R_PLATE], fill=255)
img.paste(plate_img, mask=plate_mask)
draw = ImageDraw.Draw(img)

# ═══════════════════════════════════════════════════════════════════════
# RETE — outer ring + ecliptic ring + star-pointer filigree
# ═══════════════════════════════════════════════════════════════════════

# --- Outer rete ring ---
draw.ellipse([CX-R_RETE,   CY-R_RETE,   CX+R_RETE,   CY+R_RETE],
             outline=RETE_GOLD, width=3)
draw.ellipse([CX-R_RETE+4, CY-R_RETE+4, CX+R_RETE-4, CY+R_RETE-4],
             outline=RETE_DIM,  width=1)

# --- Rete structural crossbars (thin struts inside the ring) ---
for a in [0, 45, 90, 135]:
    rad = math.radians(a)
    ri  = R_RETE - 5
    plate_draw_local = ImageDraw.Draw(img)
    img_draw_l = ImageDraw.Draw(img)
    x1 = CX - ri * math.cos(rad); y1 = CY - ri * math.sin(rad)
    x2 = CX + ri * math.cos(rad); y2 = CY + ri * math.sin(rad)
    draw.line([x1, y1, x2, y2], fill=BRASS_SUBTLE, width=1)

# --- Ecliptic ring (offset south, slightly smaller than rete inner) ---
ECLIP_R  = 50
ECLIP_CX = CX
ECLIP_CY = CY + 16    # offset toward south
draw.ellipse([ECLIP_CX-ECLIP_R-3, ECLIP_CY-ECLIP_R-3,
              ECLIP_CX+ECLIP_R+3, ECLIP_CY+ECLIP_R+3], outline=ECLIP_COL, width=2)
draw.ellipse([ECLIP_CX-ECLIP_R+2, ECLIP_CY-ECLIP_R+2,
              ECLIP_CX+ECLIP_R-2, ECLIP_CY+ECLIP_R-2], outline=BRASS_DEEP, width=1)

# Ecliptic tick marks — 12 zodiac divisions + 5° sub-ticks
for deg in range(0, 360, 5):
    is_zodiac = (deg % 30 == 0)
    rad  = math.radians(deg - 90)
    r_o  = ECLIP_R + 3
    r_i  = r_o - (7 if is_zodiac else 3)
    col  = RETE_GOLD if is_zodiac else ECLIP_COL
    x1 = ECLIP_CX + r_o * math.cos(rad); y1 = ECLIP_CY + r_o * math.sin(rad)
    x2 = ECLIP_CX + r_i * math.cos(rad); y2 = ECLIP_CY + r_i * math.sin(rad)
    draw.line([x1, y1, x2, y2], fill=col, width=2 if is_zodiac else 1)

# Ecliptic→rete connector struts (spokes linking ecliptic ring to rete ring)
for sa in [0, 60, 120, 180, 240, 300]:
    rad = math.radians(sa - 90)
    ex = ECLIP_CX + ECLIP_R * math.cos(rad)
    ey = ECLIP_CY + ECLIP_R * math.sin(rad)
    # head toward rete ring boundary at same angle FROM disc center
    ra = math.atan2(ey - CY, ex - CX)
    rx = CX + (R_RETE - 5) * math.cos(ra)
    ry = CY + (R_RETE - 5) * math.sin(ra)
    draw.line([ex, ey, rx, ry], fill=RETE_DIM, width=1)

# --- Star pointer tabs: 24 tabs of varying lengths placed at irregular angles ---
# Real astrolabes have named-star pointers at non-uniform positions;
# we place them pseudo-randomly but reproducibly.
# Tab: a thin "pin" projecting from rete ring inward (or sometimes a tongue protruding outward).
star_angles = [
    # (angle_from_12oclock, tab_length, side)   side: +1=inward -1=outward
    (  5,  22, +1), ( 18,  16, +1), ( 32,  26, +1), ( 47,  14, +1),
    ( 63,  20, +1), ( 79,  18, +1), ( 95,  24, +1), (112,  16, +1),
    (128,  20, +1), (144,  14, +1), (158,  22, +1), (172,  18, +1),
    (185,  26, +1), (200,  14, +1), (213,  20, +1), (227,  16, +1),
    (242,  24, +1), (258,  14, +1), (272,  20, +1), (285,  18, +1),
    (298,  16, +1), (315,  22, +1), (332,  18, +1), (348,  14, +1),
]
for ang, length, side in star_angles:
    rad = math.radians(ang - 90)
    r_base = R_RETE - 4
    r_tip  = r_base - length * side

    bx = CX + r_base * math.cos(rad); by = CY + r_base * math.sin(rad)
    tx = CX + r_tip  * math.cos(rad); ty = CY + r_tip  * math.sin(rad)

    # Stem
    draw.line([bx, by, tx, ty], fill=RETE_GOLD, width=1)

    # Small tip dot (like a star-pointer head)
    draw.ellipse([tx-2, ty-2, tx+2, ty+2], fill=RETE_GOLD, outline=None)

    # Collar at base (tiny cross-bar)
    perp = math.radians(ang - 90 + 90)
    cx2 = bx + 3 * math.cos(perp); cy2 = by + 3 * math.sin(perp)
    cx3 = bx - 3 * math.cos(perp); cy3 = by - 3 * math.sin(perp)
    draw.line([cx2, cy2, cx3, cy3], fill=RETE_DIM, width=1)

# ═══════════════════════════════════════════════════════════════════════
# MATER RING — the body between plate edge and limb
# ═══════════════════════════════════════════════════════════════════════
R_MATER_IN  = R_PLATE
R_MATER_OUT = R_LIMB - 2

draw.ellipse([CX-R_MATER_OUT, CY-R_MATER_OUT, CX+R_MATER_OUT, CY+R_MATER_OUT],
             fill=BRASS_DEEP, outline=None)
draw.ellipse([CX-R_MATER_IN,  CY-R_MATER_IN,  CX+R_MATER_IN,  CY+R_MATER_IN],
             fill=BG, outline=None)
draw.ellipse([CX-R_MATER_OUT, CY-R_MATER_OUT, CX+R_MATER_OUT, CY+R_MATER_OUT],
             outline=BRASS_MID, width=1)
draw.ellipse([CX-R_MATER_IN,  CY-R_MATER_IN,  CX+R_MATER_IN,  CY+R_MATER_IN],
             outline=BRASS_DARK, width=1)

# ═══════════════════════════════════════════════════════════════════════
# LIMB — outer graduation ring with tick marks
# ═══════════════════════════════════════════════════════════════════════

# Limb fill ring
draw.ellipse([CX-R_DISC, CY-R_DISC, CX+R_DISC, CY+R_DISC],
             fill=BRASS_DEEP, outline=None)
draw.ellipse([CX-R_LIMB, CY-R_LIMB, CX+R_LIMB, CY+R_LIMB],
             fill=BG, outline=None)

# Limb border rings
draw.ellipse([CX-R_DISC,   CY-R_DISC,   CX+R_DISC,   CY+R_DISC],   outline=BRASS_GOLD, width=2)
draw.ellipse([CX-R_DISC+3, CY-R_DISC+3, CX+R_DISC-3, CY+R_DISC-3], outline=BRASS_DARK, width=1)
draw.ellipse([CX-R_LIMB,   CY-R_LIMB,   CX+R_LIMB,   CY+R_LIMB],   outline=BRASS_GOLD, width=1)

# Graduation ticks — 1° fine, 5° medium, 10° large, 30° major
for deg in range(0, 360, 1):
    is_major  = (deg % 30 == 0)
    is_large  = (deg % 10 == 0)
    is_medium = (deg % 5  == 0)

    col = BRASS_BRIGHT if is_major else (BRASS_GOLD if is_large else
          (BRASS_MID if is_medium else BRASS_DARK))
    r_in = (R_DISC - 12) if is_major else ((R_DISC - 8) if is_large else
            ((R_DISC - 5) if is_medium else (R_DISC - 3)))
    w = 2 if is_major else 1

    rad = math.radians(deg - 90)
    co, si = math.cos(rad), math.sin(rad)
    draw.line([(CX + (R_DISC-1)*co, CY + (R_DISC-1)*si),
               (CX + r_in*co,       CY + r_in*si)],
              fill=col, width=w)

# Cardinal dots
for deg in [0, 90, 180, 270]:
    rad = math.radians(deg - 90)
    rm  = (R_DISC + R_LIMB) // 2
    lx  = int(CX + rm * math.cos(rad))
    ly  = int(CY + rm * math.sin(rad))
    draw.ellipse([lx-2, ly-2, lx+2, ly+2], fill=BRASS_BRIGHT)

# ═══════════════════════════════════════════════════════════════════════
# ICON ORBIT TRACK (very subtle guide circle)
# ═══════════════════════════════════════════════════════════════════════
draw.ellipse([CX-R_ICONS, CY-R_ICONS, CX+R_ICONS, CY+R_ICONS],
             outline=BRASS_SUBTLE, width=1)

# ═══════════════════════════════════════════════════════════════════════
# CENTER PIVOT
# ═══════════════════════════════════════════════════════════════════════
draw.ellipse([CX-11, CY-11, CX+11, CY+11], fill=BRASS_DEEP,  outline=BRASS_MID,   width=1)
draw.ellipse([CX-5,  CY-5,  CX+5,  CY+5],  fill=BRASS_DARK,  outline=None)
draw.ellipse([CX-2,  CY-2,  CX+2,  CY+2],  fill=BRASS_BRIGHT, outline=None)

# ═══════════════════════════════════════════════════════════════════════
# CLIP the entire image to the disc circle
# ═══════════════════════════════════════════════════════════════════════
disc_mask = Image.new("L", (SIZE, SIZE), 0)
ImageDraw.Draw(disc_mask).ellipse([CX-R_DISC, CY-R_DISC, CX+R_DISC, CY+R_DISC], fill=255)
bg_img = Image.new("RGB", (SIZE, SIZE), BG)
img    = Image.composite(img, bg_img, disc_mask)

# ── PNG preview ──────────────────────────────────────────────────
img.save("/config/GitHub/Astrolabe/tools/astrolabe_bg.png")
print("PNG saved.")

# ── RGB565 C array ───────────────────────────────────────────────
out_path = "/config/GitHub/Astrolabe/main/apps/launcher/astrolabe_bg.h"
with open(out_path, "w") as f:
    f.write("#pragma once\n#include <stdint.h>\n\n")
    f.write(f"// Astrolabe background 240×240 RGB565 (byte-swapped for pushImage)\n")
    f.write(f"static const uint16_t astrolabe_bg[{SIZE*SIZE}] = {{\n")
    px = list(img.getdata())
    for i, (r, g, b) in enumerate(px):
        v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        v = ((v & 0xFF) << 8) | (v >> 8)          # byte-swap
        f.write(f"    0x{v:04X}," if i % 12 == 0 else f" 0x{v:04X},")
        if i % 12 == 11:
            f.write("\n")
    f.write("\n};\n")

print(f"C array: {SIZE*SIZE} px  ({SIZE*SIZE*2} bytes)")
