PS5_HOST ?= ps5
PS5_PORT ?= 9021

TITLE := Fireplace5
ELF   := eboot.elf

ifdef PS5_PAYLOAD_SDK
    include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
    $(warning PS5_PAYLOAD_SDK is undefined - only the 'native' target will work)
endif

# SDL2 flags via the SDK's pkg-config wrapper. The PS5 SDL2 build does not
# actually pull in libsamplerate (no unresolved src_* symbols), and there is
# no separate libm on the PS5 (math lives in libc), so both are filtered out.
SDL_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL_LIBS   := $(filter-out -lsamplerate -lm,$(shell $(PKG_CONFIG) --libs sdl2 2>/dev/null))

CFLAGS   := -Wall -O2 $(SDL_CFLAGS)
CXXFLAGS := -Wall -O2 $(SDL_CFLAGS)
LDLIBS   := $(SDL_LIBS)

# --- PS5 payload build ---
$(ELF): main.o xm_player.o
	$(CXX) $^ -o $@ $(LDLIBS)

main.o: src/main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

xm_player.o: src/xm_player.c
	$(CC) $(CFLAGS) -c $< -o $@

# Deploy to a jailbroken PS5 running elfldr on PS5_PORT.
test: $(ELF)
	$(PS5_DEPLOY) -h $(PS5_HOST) -p $(PS5_PORT) $(ELF)

# Assemble the /data/homebrew/Fireplace5 layout for the websrv launcher.
homebrew: $(ELF) sce_sys/icon0.png
	mkdir -p dist/$(TITLE)/sce_sys
	cp $(ELF) dist/$(TITLE)/eboot.elf
	cp sce_sys/icon0.png dist/$(TITLE)/sce_sys/icon0.png
	cp assets/external.xm dist/$(TITLE)/external.xm
	cd dist && rm -f $(TITLE).zip && zip -r $(TITLE).zip $(TITLE)

# Assemble a source folder for a debug/homebrew fake PKG (LibProsperoPKG /
# PPR-PKG builder). It expects the executable as a raw ELF named eboot.bin.
# Point the builder's "Source folder" at dist/$(TITLE)-pkgsrc.
pkgsrc: $(ELF) sce_sys/icon0.png pkg/sce_sys/param.json
	mkdir -p dist/$(TITLE)-pkgsrc/sce_sys
	cp $(ELF) dist/$(TITLE)-pkgsrc/eboot.bin
	cp sce_sys/icon0.png dist/$(TITLE)-pkgsrc/sce_sys/icon0.png
	cp pkg/sce_sys/param.json dist/$(TITLE)-pkgsrc/sce_sys/param.json
	cp assets/external.xm dist/$(TITLE)-pkgsrc/external.xm
	@echo "PKG source ready: dist/$(TITLE)-pkgsrc"

# --- Desktop smoke test (no PS5 SDK; needs libsdl2-dev) ---
native: src/main.cpp src/xm_player.c
	gcc -O2 -c src/xm_player.c -o xm_player-native.o
	g++ -O2 -Wall -o $(TITLE)-native src/main.cpp xm_player-native.o $(shell sdl2-config --cflags --libs) -lm

clean:
	rm -rf $(ELF) *.o $(TITLE)-native dist

.PHONY: test homebrew pkgsrc native clean
