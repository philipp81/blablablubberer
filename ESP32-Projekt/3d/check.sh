#!/bin/sh
# Prüft das Modell auf Kollisionen: rendert jede Probe aus probe.scad und gibt das Schnittvolumen aus.
# Erwartung: alles "leer" oder 0 mm³, nur boost_body und boost_drop zeigen die vier Klammernasen (ca. 7 mm³).
set -e
cd "$(dirname "$0")"
OPENSCAD=${OPENSCAD:-$(command -v openscad || echo /Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD)}
OUT=${TMPDIR:-/tmp}/megafon-check; mkdir -p "$OUT"
for p in hood shell cells_body cells_hood esp_body boost_body amp_body toggle_body mic_body elec_cells elec_hood elec_plate plate_body plate_hood lift slide boost_drop; do
  rm -f "$OUT/p_$p.stl"
  "$OPENSCAD" --backend=manifold --export-format=binstl -o "$OUT/p_$p.stl" -D "probe=\"$p\"" probe.scad > "$OUT/log_$p.txt" 2>&1 || true
  if [ -f "$OUT/p_$p.stl" ]; then
    python3 - "$OUT/p_$p.stl" "$p" <<'EOF'
import struct, sys
data = open(sys.argv[1], "rb").read(); n = struct.unpack("<I", data[80:84])[0]
total = 0.0; off = 84; xs, ys, zs = [], [], []
for _ in range(n):
    v = struct.unpack("<12f", data[off:off+48]); off += 50
    a, b, c = v[3:6], v[6:9], v[9:12]
    cx = b[1]*c[2] - b[2]*c[1]; cy = b[2]*c[0] - b[0]*c[2]; cz = b[0]*c[1] - b[1]*c[0]
    total += (a[0]*cx + a[1]*cy + a[2]*cz) / 6.0
    for k in range(3, 12, 3): xs.append(v[k]); ys.append(v[k+1]); zs.append(v[k+2])
print(f"{sys.argv[2]:12s} {total:9.3f} mm3   x {min(xs):7.2f}..{max(xs):7.2f}  y {min(ys):7.2f}..{max(ys):7.2f}  z {min(zs):7.2f}..{max(zs):7.2f}")
EOF
  else
    echo "$p: leer"
  fi
done
