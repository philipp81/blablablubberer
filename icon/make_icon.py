#!/usr/bin/env python3
"""
Erzeugt das Blablablubber-Icon:
  1. baut ein SVG (Regenbogen-Sonne, Goldrahmen, Schnörkel, Sterne, Sprechblase, Banner)
  2. rendert es mit qlmanage (macOS Quick Look) nach PNG, 1024x1024
  3. schneidet es auf die macOS-Squircle-Form zu (Pillow)
  4. baut daraus Blablablubber.iconset und Blablablubber.icns (iconutil)
"""
import colorsys
import glob
import math
import os
import random
import shutil
import subprocess
import sys
import tempfile

HERE    = os.path.dirname(os.path.abspath(__file__))
SVG     = os.path.join(HERE, "blablablubber.svg")
PNG     = os.path.join(HERE, "blablablubber-1024.png")
PREVIEW = os.path.join(HERE, "preview.png")
ICONSET = os.path.join(HERE, "Blablablubber.iconset")
ICNS    = os.path.join(HERE, "Blablablubber.icns")

random.seed(20261023)

S  = 1024
CX = CY = 512
BOX = (100, 100, 824, 824)   # Apple-Vorlage: 824 px Squircle auf 1024 px Canvas
RX  = 186

PALETTE = ["#ff2d55", "#ff9500", "#ffcc00", "#34c759", "#00c7be",
           "#32ade6", "#5856d6", "#af52de", "#ff375f", "#ffd60a"]
NAVY = "#0d1f4a"
FONT = "Impact, 'Arial Black', Helvetica, sans-serif"


def hsl(h, s, l):
    r, g, b = colorsys.hls_to_rgb(h % 1.0, l, s)
    return "#%02x%02x%02x" % (int(r * 255), int(g * 255), int(b * 255))


def fmt(v):
    return "%.1f" % v


def star_points(cx, cy, r_out, r_in, n=5, rot=-90):
    pts = []
    for i in range(2 * n):
        r = r_out if i % 2 == 0 else r_in
        a = math.radians(rot + i * 180.0 / n)
        pts.append("%s,%s" % (fmt(cx + r * math.cos(a)), fmt(cy + r * math.sin(a))))
    return " ".join(pts)


def sparkle_path(cx, cy, s):
    return ("M %s,%s Q %s,%s %s,%s Q %s,%s %s,%s Q %s,%s %s,%s Q %s,%s %s,%s Z" % (
        fmt(cx), fmt(cy - s), fmt(cx), fmt(cy), fmt(cx + s), fmt(cy),
        fmt(cx), fmt(cy), fmt(cx), fmt(cy + s),
        fmt(cx), fmt(cy), fmt(cx - s), fmt(cy),
        fmt(cx), fmt(cy), fmt(cx), fmt(cy - s)))


def scroll_path(size, turns=1.75, n=80):
    """Doppel-Schnörkel (S-Form mit eingerollten Enden), zentriert auf (0,0)."""
    R = size * 0.42
    thmax = 2 * math.pi * turns
    k = R / thmax
    c = -size * 0.5
    left = []
    for i in range(n + 1):
        th = thmax * i / n
        r = k * th
        left.append((c + r * math.cos(th), r * math.sin(th)))
    right = [(-x, -y) for (x, y) in reversed(left)]
    d = "M %s,%s " % (fmt(left[0][0]), fmt(left[0][1]))
    d += " ".join("L %s,%s" % (fmt(x), fmt(y)) for x, y in left[1:])
    (x1, y1), (x2, y2) = left[-1], right[0]
    d += " C %s,%s %s,%s %s,%s " % (fmt(x1 + size * 0.45), fmt(y1), fmt(x2 - size * 0.45), fmt(y2), fmt(x2), fmt(y2))
    d += " ".join("L %s,%s" % (fmt(x), fmt(y)) for x, y in right[1:])
    return d


def scroll(cx, cy, size, rot, color="url(#gold)", outline=NAVY, beads=True):
    w = max(2.5, size * 0.10)
    d = scroll_path(size)
    g = ['<g transform="translate(%s,%s) rotate(%s)">' % (fmt(cx), fmt(cy), fmt(rot))]
    g.append('<path d="%s" fill="none" stroke="%s" stroke-width="%s" stroke-linecap="round" stroke-linejoin="round"/>'
             % (d, outline, fmt(w + max(2.5, size * 0.05))))
    g.append('<path d="%s" fill="none" stroke="%s" stroke-width="%s" stroke-linecap="round" stroke-linejoin="round"/>'
             % (d, color, fmt(w)))
    if beads:
        for bx in (-size * 0.5, size * 0.5):
            g.append('<circle cx="%s" cy="0" r="%s" fill="%s" stroke="%s" stroke-width="%s"/>'
                     % (fmt(bx), fmt(w * 0.9), color, outline, fmt(max(1.5, w * 0.35))))
    g.append('</g>')
    return "\n".join(g)


def build_svg():
    o = []
    o.append('<svg xmlns="http://www.w3.org/2000/svg" width="1024" height="1024" viewBox="0 0 1024 1024">')
    o.append('''<defs>
  <clipPath id="squircle"><rect x="100" y="100" width="824" height="824" rx="186"/></clipPath>
  <radialGradient id="depth" cx="50%" cy="42%" r="68%">
    <stop offset="0" stop-color="#ffffff" stop-opacity="0.55"/>
    <stop offset="0.45" stop-color="#ffffff" stop-opacity="0"/>
    <stop offset="1" stop-color="#000000" stop-opacity="0.5"/>
  </radialGradient>
  <linearGradient id="gold" x1="0" y1="0" x2="1" y2="1">
    <stop offset="0" stop-color="#fff6b0"/>
    <stop offset="0.25" stop-color="#ffd84a"/>
    <stop offset="0.5" stop-color="#b8860b"/>
    <stop offset="0.75" stop-color="#ffe066"/>
    <stop offset="1" stop-color="#8a5a00"/>
  </linearGradient>
  <linearGradient id="goldv" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#fff6b0"/>
    <stop offset="0.5" stop-color="#e6b422"/>
    <stop offset="1" stop-color="#8a5a00"/>
  </linearGradient>
  <linearGradient id="red" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#ff6b78"/>
    <stop offset="0.5" stop-color="#e0162b"/>
    <stop offset="1" stop-color="#8e0a1b"/>
  </linearGradient>
  <linearGradient id="blue" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#5f8fe8"/>
    <stop offset="0.6" stop-color="#1b3a7a"/>
    <stop offset="1" stop-color="#0d1f4a"/>
  </linearGradient>
  <radialGradient id="bubble" cx="35%" cy="30%" r="70%">
    <stop offset="0" stop-color="#ffffff" stop-opacity="0.95"/>
    <stop offset="0.35" stop-color="#bfe9ff" stop-opacity="0.7"/>
    <stop offset="1" stop-color="#2f9bdc" stop-opacity="0.85"/>
  </radialGradient>
  <filter id="shadow" x="-25%" y="-25%" width="150%" height="150%">
    <feGaussianBlur in="SourceAlpha" stdDeviation="10"/>
    <feOffset dy="12" result="o"/>
    <feComponentTransfer><feFuncA type="linear" slope="0.55"/></feComponentTransfer>
    <feMerge><feMergeNode/><feMergeNode in="SourceGraphic"/></feMerge>
  </filter>
  <filter id="glow" x="-50%" y="-50%" width="200%" height="200%">
    <feGaussianBlur stdDeviation="6" result="b"/>
    <feMerge><feMergeNode in="b"/><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge>
  </filter>
</defs>''')

    # ---- Hintergrund: Regenbogen-Sonne --------------------------------------
    o.append('<g clip-path="url(#squircle)">')
    N = 48
    R = 900
    rot0 = -math.pi / 2
    for i in range(N):
        a0 = rot0 + 2 * math.pi * i / N
        a1 = rot0 + 2 * math.pi * (i + 1) / N
        col = hsl(i / N, 0.95, 0.62 if i % 2 == 0 else 0.46)
        o.append('<polygon points="%s,%s %s,%s %s,%s" fill="%s"/>' % (
            CX, CY, fmt(CX + R * math.cos(a0)), fmt(CY + R * math.sin(a0)),
            fmt(CX + R * math.cos(a1)), fmt(CY + R * math.sin(a1)), col))
    o.append('<rect x="0" y="0" width="1024" height="1024" fill="url(#depth)"/>')

    # ---- Konfetti ------------------------------------------------------------
    for _ in range(90):
        x, y = random.uniform(120, 904), random.uniform(120, 904)
        r = random.uniform(4, 11)
        o.append('<circle cx="%s" cy="%s" r="%s" fill="%s" stroke="#ffffff" stroke-width="2" opacity="0.9"/>'
                 % (fmt(x), fmt(y), fmt(r), random.choice(PALETTE)))

    # ---- lauter kleine Schnörkeldinger ---------------------------------------
    for _ in range(46):
        x, y = random.uniform(140, 884), random.uniform(140, 884)
        o.append(scroll(x, y, random.uniform(22, 48), random.uniform(0, 360),
                        color=random.choice(PALETTE), outline="#ffffff", beads=False))

    # ---- Sterne und Funkeln --------------------------------------------------
    for _ in range(34):
        x, y = random.uniform(130, 894), random.uniform(130, 894)
        r = random.uniform(9, 26)
        col = random.choice(["#ffffff", "#ffe066", "#fff6b0", "#ffd60a"])
        o.append('<polygon points="%s" fill="%s" stroke="%s" stroke-width="2.5" filter="url(#glow)"/>'
                 % (star_points(x, y, r, r * 0.45), col, NAVY))
    for _ in range(26):
        x, y = random.uniform(130, 894), random.uniform(130, 894)
        o.append('<path d="%s" fill="#ffffff" opacity="0.95" filter="url(#glow)"/>'
                 % sparkle_path(x, y, random.uniform(10, 30)))
    o.append('</g>')

    # ---- Goldrahmen ----------------------------------------------------------
    o.append('<rect x="100" y="100" width="824" height="824" rx="186" fill="none" stroke="%s" stroke-width="44"/>' % NAVY)
    o.append('<rect x="100" y="100" width="824" height="824" rx="186" fill="none" stroke="url(#gold)" stroke-width="32"/>')
    o.append('<rect x="100" y="100" width="824" height="824" rx="186" fill="none" stroke="#fff6b0" stroke-width="3" opacity="0.8"/>')
    o.append('<rect x="128" y="128" width="768" height="768" rx="162" fill="none" stroke="#ffffff" stroke-width="3" opacity="0.6" stroke-dasharray="14 10"/>')
    # Ecken- und Randschnörkel
    for (x, y, rot, size) in [(205, 205, -45, 115), (819, 205, 45, 115),
                              (195, 862, 0, 46), (829, 862, 0, 46),
                              (160, 330, 90, 62), (864, 330, 90, 62),
                              (160, 640, 90, 62), (864, 640, 90, 62)]:
        o.append(scroll(x, y, size, rot))

    # ---- großer Stern oben + zwei Begleiter ----------------------------------
    o.append('<polygon points="%s" fill="url(#goldv)" stroke="%s" stroke-width="8" filter="url(#glow)"/>'
             % (star_points(512, 170, 66, 28), NAVY))
    o.append('<polygon points="%s" fill="#ffffff" stroke="%s" stroke-width="5" filter="url(#glow)"/>'
             % (star_points(400, 168, 32, 14), NAVY))
    o.append('<polygon points="%s" fill="#ffffff" stroke="%s" stroke-width="5" filter="url(#glow)"/>'
             % (star_points(624, 168, 32, 14), NAVY))

    # ---- große Sprechblase ---------------------------------------------------
    bubble = ("M 322,240 H 702 A 100,100 0 0 1 802,340 V 490 A 100,100 0 0 1 702,590 "
              "H 440 L 290,690 L 350,590 H 322 A 100,100 0 0 1 222,490 V 340 A 100,100 0 0 1 322,240 Z")
    o.append('<g filter="url(#shadow)">')
    o.append('<path d="%s" fill="url(#red)" stroke="%s" stroke-width="14" stroke-linejoin="round"/>' % (bubble, NAVY))
    o.append('<path d="%s" fill="none" stroke="url(#gold)" stroke-width="6" stroke-linejoin="round"/>' % bubble)
    o.append('</g>')
    o.append('<ellipse cx="470" cy="300" rx="200" ry="42" fill="#ffffff" opacity="0.32"/>')
    o.append('<text x="512" y="415" text-anchor="middle" font-family="%s" font-weight="900" font-size="150" '
             'fill="#ffffff" stroke="%s" stroke-width="12" paint-order="stroke" letter-spacing="4">BLA BLA</text>' % (FONT, NAVY))
    o.append('<text x="512" y="548" text-anchor="middle" font-family="%s" font-weight="900" font-size="120" '
             'fill="url(#goldv)" stroke="%s" stroke-width="10" paint-order="stroke" letter-spacing="6">BLUBB!</text>' % (FONT, NAVY))

    # ---- Blubberblasen -------------------------------------------------------
    for (x, y, r) in [(850, 640, 26), (886, 582, 16), (858, 532, 11), (176, 712, 20), (150, 660, 12), (226, 760, 9)]:
        o.append('<circle cx="%s" cy="%s" r="%s" fill="url(#bubble)" stroke="#ffffff" stroke-width="3"/>' % (x, y, r))
        o.append('<circle cx="%s" cy="%s" r="%s" fill="#ffffff" opacity="0.9"/>' % (fmt(x - r * 0.35), fmt(y - r * 0.4), fmt(r * 0.22)))

    # ---- Banner unten --------------------------------------------------------
    o.append('<g filter="url(#shadow)">')
    o.append('<polygon points="140,730 240,730 240,812 140,812 176,771" fill="#8e0a1b" stroke="%s" stroke-width="6" stroke-linejoin="round"/>' % NAVY)
    o.append('<polygon points="884,730 784,730 784,812 884,812 848,771" fill="#8e0a1b" stroke="%s" stroke-width="6" stroke-linejoin="round"/>' % NAVY)
    o.append('<polygon points="212,792 240,792 240,812" fill="#5a0612"/>')
    o.append('<polygon points="812,792 784,792 784,812" fill="#5a0612"/>')
    o.append('<rect x="212" y="712" width="600" height="80" fill="url(#red)" stroke="%s" stroke-width="8"/>' % NAVY)
    o.append('<rect x="212" y="712" width="600" height="80" fill="none" stroke="url(#gold)" stroke-width="4"/>')
    o.append('</g>')
    o.append('<text x="512" y="770" text-anchor="middle" font-family="%s" font-weight="900" font-size="58" '
             'fill="#ffffff" stroke="%s" stroke-width="5" paint-order="stroke" letter-spacing="3">BLABLABLUBBER</text>' % (FONT, NAVY))

    o.append('</svg>')
    return "\n".join(o)


def render_png(svg_path, out_png):
    """SVG -> PNG per qlmanage, danach mit Pillow auf die Squircle-Form zuschneiden."""
    from PIL import Image, ImageDraw, ImageChops

    tmp = tempfile.mkdtemp(prefix="blablablubber-icon-")
    try:
        subprocess.run(["qlmanage", "-t", "-s", str(S), "-o", tmp, svg_path],
                       check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        files = glob.glob(os.path.join(tmp, "*.png"))
        if not files:
            raise RuntimeError("qlmanage hat kein PNG erzeugt")
        img = Image.open(files[0]).convert("RGBA")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    if img.size != (S, S):
        canvas = Image.new("RGBA", (S, S), (0, 0, 0, 0))
        img.thumbnail((S, S), Image.LANCZOS)
        canvas.paste(img, ((S - img.width) // 2, (S - img.height) // 2))
        img = canvas

    k = 4
    mask = Image.new("L", (S * k, S * k), 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (BOX[0] * k, BOX[1] * k, (BOX[0] + BOX[2]) * k, (BOX[1] + BOX[3]) * k), radius=RX * k, fill=255)
    mask = mask.resize((S, S), Image.LANCZOS)
    img.putalpha(ImageChops.multiply(img.getchannel("A"), mask))
    img.save(out_png)
    return img


def build_iconset(img):
    from PIL import Image
    if os.path.isdir(ICONSET):
        shutil.rmtree(ICONSET)
    os.makedirs(ICONSET)
    for base in (16, 32, 128, 256, 512):
        for scale in (1, 2):
            px = base * scale
            name = "icon_%dx%d%s.png" % (base, base, "@2x" if scale == 2 else "")
            img.resize((px, px), Image.LANCZOS).save(os.path.join(ICONSET, name))
    subprocess.run(["iconutil", "-c", "icns", ICONSET, "-o", ICNS], check=True)
    img.resize((512, 512), Image.LANCZOS).save(PREVIEW)


def main():
    with open(SVG, "w", encoding="utf-8") as f:
        f.write(build_svg())
    print("SVG geschrieben:", SVG)
    img = render_png(SVG, PNG)
    print("PNG gerendert:  ", PNG, img.size)
    build_iconset(img)
    print("Icon gebaut:    ", ICNS)
    print("Vorschau:       ", PREVIEW)


if __name__ == "__main__":
    sys.exit(main())
