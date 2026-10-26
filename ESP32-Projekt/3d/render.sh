#!/bin/sh
# Erzeugt STL-Dateien und Vorschaubilder aus megafon.scad (braucht OpenSCAD)
set -e
cd "$(dirname "$0")"
OPENSCAD=${OPENSCAD:-$(command -v openscad || echo /Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD)}
for p in koerper haube rueckplatte kegel griffschale tastermutter; do
  "$OPENSCAD" --backend=manifold --export-format=binstl -o "megafon-$p.stl" -D "part=\"$p\"" megafon.scad
done
for i in 1 2 3; do "$OPENSCAD" --backend=manifold --export-format=binstl -o "megafon-tastermutter-$i.stl" -D "part=\"tastermutter$i\"" megafon.scad; done
"$OPENSCAD" --backend=manifold -o preview-zusammengebaut.png --autocenter --viewall --imgsize=1400,1000 --camera=0,0,0,65,0,35,0 -D 'part="alle"' megafon.scad
"$OPENSCAD" --backend=manifold -o preview-offen.png --autocenter --viewall --imgsize=1400,1000 --camera=0,0,0,60,0,30,0 -D 'part="offen"' megafon.scad
"$OPENSCAD" --backend=manifold -o preview-innen.png --autocenter --viewall --imgsize=1400,1000 --projection=o --camera=-50,0.01,300,-50,0,0 -D 'part="innen"' megafon.scad
"$OPENSCAD" --backend=manifold -o preview-hinten.png --autocenter --viewall --imgsize=1200,900 --camera=0,0,0,70,0,250,0 -D 'part="alle"' megafon.scad
"$OPENSCAD" --backend=manifold -o preview-einschub.png --autocenter --viewall --imgsize=1200,900 --camera=0,0,0,60,0,235,0 -D 'part="einschub"' megafon.scad
"$OPENSCAD" --backend=manifold -o preview-zellen.png --autocenter --viewall --imgsize=1200,900 --camera=0,0,0,32,0,270,0 -D 'part="zellen"' megafon.scad
"$OPENSCAD" --backend=manifold -o preview-schnitt.png --autocenter --viewall --imgsize=1200,900 --camera=0,0,0,38,0,235,0 -D 'part="schnitt"' megafon.scad
"$OPENSCAD" --backend=manifold -o preview-griff.png --autocenter --viewall --imgsize=1200,900 --camera=0,0,0,78,0,165,0 -D 'part="griff"' megafon.scad
"$OPENSCAD" --backend=manifold -o preview-rueckplatte.png --autocenter --viewall --imgsize=1000,800 --camera=0,0,0,55,0,210,0 -D 'part="rueckplatte"' megafon.scad
"$OPENSCAD" --backend=manifold -o preview-koerper.png --autocenter --viewall --imgsize=1200,900 --camera=0,0,0,60,0,210,0 -D 'part="koerper"' megafon.scad
echo "fertig: megafon-koerper.stl megafon-haube.stl megafon-rueckplatte.stl megafon-kegel.stl megafon-griffschale.stl megafon-tastermutter.stl megafon-tastermutter-1..3.stl"
