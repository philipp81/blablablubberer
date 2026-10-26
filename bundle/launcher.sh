#!/bin/sh
# Startet Blablablubber in einem neuen Terminal-Fenster.
# (Das eigentliche Programm liegt im Bundle unter Contents/Resources/bin.)
HERE="$(cd "$(dirname "$0")/.." && pwd)"
exec open -a Terminal "$HERE/Resources/bin/blablablubber"
