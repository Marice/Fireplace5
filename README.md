# Fireplace5 (PlayStation 5)

A lovely pixelated fireplace with a demoscene twist, running natively on the
PlayStation 5 as a homebrew payload. Part of the Fireplace family:
[Fireplace-NX](https://github.com/Marice/Fireplace-NX) (Switch),
[nextui-fireplace-pak](https://github.com/Marice/nextui-fireplace-pak) (TrimUI Brick),
and now the PS5. The fire effect is based on
[hanshq.net](https://www.hanshq.net/fire.html).

The fire is simulated on a 384x216 buffer and scaled to 1920x1080. A
cracktro-style scroller waves across the flames, with rising embers, a
parallax starfield, chiptune music, wind, adjustable intensity and a CRT
filter.

> **Requires a jailbroken PS5.** This is unsigned homebrew: it runs only on
> a console with an ELF loader (elfldr / etaHEN / websrv) from a public
> exploit chain. There is no downgrade path, so a retail console on
> non-exploitable firmware cannot run it.

## Controls (DualSense)

Press **Touchpad** any time for this list on-screen.

| Button | Action |
|---|---|
| **Cross** | Toggle the text scroller (on at startup) |
| **Circle** | Cycle ambience: off → embers → embers + clock |
| **Square** / **Triangle** | Fire intensity down / up (5 levels) |
| **D-pad** | Cycle color palette: classic → blue → green → purple → mono |
| **L1** / **R1** (hold) | Wind: flames and embers lean left / right |
| **R2** | Toggle CRT scanline filter |
| **Options** | Pause / resume music |
| **Touchpad** | Toggle the help overlay |
| **L2 + R2** (together) | Quit back to the launcher |

## Building

Needs the [ps5-payload-dev SDK](https://github.com/ps5-payload-dev/sdk)
(clang-18, target `x86_64-sie-ps5`) with the
[SDL2 port](https://github.com/ps5-payload-dev/SDL) installed into it (SDL2
lands under the SDK's `user/homebrew` prefix). Then:

```sh
export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
make                # builds eboot.elf
make homebrew       # assembles dist/Fireplace5/ + zip for the launcher
```

A desktop test build (`make native`, needs `libsdl2-dev`) is available to
preview the effect and controls (keyboard: Space=scroller, `b`=ambience,
`-`/`=`=intensity, arrows=palette, `q`/`e`=wind, `c`=CRT, `p`=music,
`h`=help, Esc=quit).

## Installing on a jailbroken PS5

**Dev loop** — send the ELF to a console running elfldr on port 9021:

```sh
make test PS5_HOST=<ps5-ip>
```

**Launcher tile** — copy the `dist/Fireplace5/` folder to
`/data/homebrew/Fireplace5/` on the PS5 (`eboot.elf` + `sce_sys/icon0.png`).
It then appears in the [websrv](https://github.com/ps5-payload-dev/websrv)
homebrew menu.

**Debug/homebrew PKG** — to build an installable fake PKG (e.g. with the
LibProsperoPKG / PPR-PKG builder):

```sh
make pkgsrc         # assembles dist/Fireplace5-pkgsrc/ (eboot.bin + sce_sys/)
```

Then in the PKG builder set:

| Field | Value |
|---|---|
| Source folder | `dist/Fireplace5-pkgsrc` |
| Content ID | `UP0000-FIRE00005_00-FIREPLACE5000000` |
| Title | `Fireplace5` |
| Version | `01.00` |
| Package type | `Homebrew` |
| Image mode | `PLAINTEXT_NOAUTH` |

The executable must be a raw ELF named `eboot.bin` (the target renames it);
`sce_sys/param.json` (in `pkg/`) carries the metadata. This produces an
unsigned debug package for your own jailbroken console only.

## Credits

- Fire effect: [hanshq.net](https://www.hanshq.net/fire.html), original palette by Jare
- 8x8 bitmap font: [font8x8](https://github.com/dhepper/font8x8) by Daniel Hepper (public domain)
- XM player: [jar_xm](https://github.com/kd7tck/jar) by Joshua Reisenauer (public domain)
- PS5 toolchain & SDL2 port: [ps5-payload-dev](https://github.com/ps5-payload-dev)
- Switch original: Marice Lamain
