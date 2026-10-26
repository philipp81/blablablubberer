# Blablablubber – Makefile (macOS)
CC      ?= cc
CFLAGS  ?= -O2 -std=gnu11 -Wall -Wextra
LDLIBS   = -lpthread -lm -framework CoreFoundation -framework CoreAudio -framework AudioToolbox -framework AudioUnit
PREFIX  ?= /usr/local

APP      = Blablablubber.app
ICNS     = icon/Blablablubber.icns

.PHONY: all app icon test install uninstall clean

all: blablablubber

blablablubber: blablablubber.o miniaudio_impl.o
	$(CC) -o $@ $^ $(LDLIBS)

blablablubber.o: blablablubber.c third_party/miniaudio.h
	$(CC) $(CFLAGS) -c $<

# miniaudio ist Fremdcode, dessen Warnungen uns nicht interessieren
miniaudio_impl.o: miniaudio_impl.c third_party/miniaudio.h
	$(CC) -O2 -w -c $<

# macOS-App-Bundle mit Icon. Doppelklick öffnet Blablablubber im Terminal.
app: blablablubber $(ICNS)
	rm -rf $(APP)
	mkdir -p $(APP)/Contents/MacOS $(APP)/Contents/Resources/bin
	cp bundle/Info.plist $(APP)/Contents/Info.plist
	cp bundle/launcher.sh $(APP)/Contents/MacOS/Blablablubber
	chmod +x $(APP)/Contents/MacOS/Blablablubber
	cp blablablubber $(APP)/Contents/Resources/bin/blablablubber
	cp $(ICNS) $(APP)/Contents/Resources/Blablablubber.icns
	touch $(APP)
	@echo "Fertig: $(APP) (öffnen mit: open $(APP))"

# Icon neu erzeugen (SVG -> PNG -> .icns), braucht python3 mit Pillow
icon:
	python3 icon/make_icon.py

$(ICNS):
	python3 icon/make_icon.py

test: blablablubber
	./blablablubber --selftest

install: blablablubber
	install -d $(PREFIX)/bin
	install -m 755 blablablubber $(PREFIX)/bin/blablablubber

uninstall:
	rm -f $(PREFIX)/bin/blablablubber

clean:
	rm -rf *.o blablablubber $(APP)
