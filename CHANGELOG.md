# Changelog

All notable changes to this project are documented here. The format is
based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this
project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- PlayStation 5 homebrew port (ELF payload) built with the ps5-payload-dev
  SDK and its SDL2 port.
- 16:9 fire simulation (384x216) scaled to 1920x1080.
- Cracktro-style text scroller, five color palettes, rising embers, clock
  overlay, parallax starfield, chiptune music (embedded XM player), wind,
  adjustable fire intensity and a CRT scanline filter.
- DualSense controls with an on-screen help overlay (Touchpad).
- `setup-ps5-sdk.sh` for one-command host setup, and a `homebrew` make
  target that assembles the websrv launcher layout.
