# After Burner Climax Recompiled

Static recompilation of **After Burner Climax** (Xbox 360 XBLA, SEGA AM2, 2010; Title ID 58410A06, default.xex 1.0.0.0 built 2009-12-21) to native Windows and Linux (Vulkan), using the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) v0.9.0.

**No game data is included.** You need your own After Burner Climax XBLA package (the Xbox 360 LIVE file from
your console's storage).

## Download
Get the latest `AfterBurnerClimax-<version>-win64.zip` from [Releases](https://github.com/darkuni/afterburner-climax-recomp/releases),
unzip it, put your package file (or the `58410A06` folder copied from the console) in the same folder and run `after_burner_climax.exe`: the first start makes the
`assets` folder from it. Nothing else to install. Requirements: 64-bit Windows 10/11 and the
[Microsoft Visual C++ Redistributable 2015-2022 (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe).

## Building
GitHub Actions builds on every push to `main` (or run **Build Windows** by hand from the Actions tab); publishing a
release attaches the zip to it. Unzip a build and put your extracted game files in an `assets` folder next to the exe
(`tools/stfs_extract.py <package> assets` extracts the package):

```
after_burner_climax.exe
*.dll
assets\
  default.xex
  media\...
```

## Linux (native, Vulkan)
The same recompiled code builds as a native Linux executable. On Linux the SDK's Xenos GPU plugin is
Vulkan-only (D3D12 is compiled out), so nothing needs picking: the game renders through Vulkan. No Wine/Proton.

**Status:** builds with no source changes to the recompiled code; boots, extracts the package on first start
and reaches the main menu with correct graphics. Tested on CachyOS (Arch), clang 22, AMD Radeon 8060S (RADV,
Mesa), KDE Plasma Wayland (through Xwayland), 3440x1440. Not yet tested: NVIDIA, Steam Deck native, full
playthroughs.

### Build
Needs clang 20+, CMake 3.25+, Ninja, git, and the SDK's system libraries:

| Distro | Packages |
|---|---|
| Arch / CachyOS | `clang cmake ninja git gtk3 libx11 libxcb libxss vulkan-icd-loader alsa-lib libpulse pipewire` |
| Ubuntu 24.04 | clang-20 (apt.llvm.org), `cmake ninja-build build-essential git libgtk-3-dev libx11-xcb-dev libxss-dev libvulkan-dev libasound2-dev libpulse-dev libpipewire-0.3-dev` |

```
tools/build_linux.sh            # SDK v0.9.0 + darkuni SDK patches, then the game
```
The first run clones and builds the ReXGlue SDK (v0.9.0 plus the same `rexglue-sdk-builds` patches as the
Windows package) into `out/linux-deps/` - about 20-30 min; later runs only rebuild the game. Pass another
folder as the first argument to keep the SDK elsewhere (e.g. one shared with the OutRun recomp); `CC`/`CXX`/`JOBS`
are honoured. GitHub Actions does the same on Ubuntu (`.github/workflows/build-linux.yml`) and uploads
`after_burner_climax-linux-amd64.tar.gz`.

### Run
```
dist-linux/
  after_burner_climax
  librexruntime.so
  librexgpu-xenos.so
  <your package>   <- the package file (or the 58410A06 folder)
```
Put your package next to `after_burner_climax` and start it; the first start makes `assets/` from it, as on
Windows. The folder runs from anywhere (libraries load from next to the executable). Settings go to
`after_burner_climax.toml`, logs to `logs/`.

### Linux notes
- **X11 / Xwayland:** the SDK forces SDL's X11 video driver (its Vulkan presenter uses `VK_KHR_xcb_surface`), so
  on Wayland desktops it runs through Xwayland. Starting it from a plain TTY/SSH shell needs `DISPLAY` (and
  `XAUTHORITY`) from your desktop session.
- **Crashes:** unhandled faults end in a normal core dump (`coredumpctl gdb after_burner_climax`), with
  `after_burner_climax.map` for recompiled function names. The SDK owns SIGSEGV/SIGILL (guest MMIO traps and SEH
  emulation), so the project installs no handler of its own.
- **Quitting:** use F1 > Quit or the window close button. The process does not exit on SIGTERM
  (`kill`/`timeout`); use `pkill -KILL after_burner_climax` from a shell.
- **GPU choice:** with more than one Vulkan device, set `vulkan_device` (index, listed in the log) in
  `after_burner_climax.toml`.

## In-game settings (F1 or Esc)
Same menu as the Poker Smash / ReBirth recomps: display mode (also F11), monitor, window size, render resolution, VSync, aspect ratio, texture filtering, audio buffer, keyboard on/off, launcher on/off, per-button key rebinding, quit. Saved to `after_burner_climax.toml` next to the exe. F3 = performance overlay. Gamepad works in the menu too (B closes it). Runs as the full game.

### Default keyboard layout
| Action | Controller | Key |
|---|---|---|
| Fly (flick left/right = barrel roll) | Left stick | Arrow keys |
| Machine gun | A | Z |
| Missiles | B | X |
| Climax mode | LB / RB | C / V |
| Brake / accelerate | LT / RT | A / S |
| X / Y | X / Y | Q / W |
| Start / Back | Start / Back | Enter / Backspace |

## Launcher
Before the game starts, a launcher screen in the game window sets up the display and controls, then **Start game**
runs it. Same look and rows as the F1 menu, plus:

- **Monitor** (with more than one): the game window moves there live; fullscreen follows.
- **Render resolution: Auto** (default): picks the internal resolution from the monitor's native resolution
  (720p-class 1x, 1080p/1440p 2x, 4K 3x; Steam Deck 800p stays 1x). Or a fixed 1x / 2x / 3x.
- **Launcher at start**: Off goes straight into the game next time (turn it back on from F1). One-off skip:
  `after_burner_climax --recomp_launcher=false`.

It runs from ReXGlue's `OnFinalizePaths` hook: the window, GPU presenter and ImGui are up but the runtime is not
built yet, so render resolution and audio buffer changes apply when you press Start, without a restart.
Keyboard, mouse or gamepad (d-pad / left stick, A or Start = select, B = back), so it works on a Steam Deck too.

New installs start **fullscreen** (borderless, at the monitor's native resolution) with Auto render resolution;
existing `after_burner_climax.toml` settings are kept.

## Saves
Saves, profile, achievements and the shader cache are kept in `userdata/` next to the exe, so the whole game
folder is portable (copy it and your progress comes along). On the first start of this build an existing
per-user folder from earlier builds (`Documents\after_burner_climax` on Windows, `~/.local/share/after_burner_climax`
on Linux) is copied into `userdata/`; the original is left in place. If the game folder is not writable (e.g.
Program Files) the per-user folder is used as before. `--user_data_root=<dir>` overrides both.

## Layout
| Path | What |
|---|---|
| `after_burner_climax_manifest.toml` | ReXGlue project manifest (codegen input) |
| `after_burner_climax_overrides.toml` | Missed branch targets + pointer-only functions |
| `generated/` | Recompiled C++ output of `rexglue codegen` |
| `src/after_burner_climax_app.h` | App class - game folder default, Xenos GPU plugin, full-game license, first-launch defaults, launcher, menu/F11 binds |
| `src/game_menu.*`, `src/game_keys.h` | Launcher + F1/Esc settings menu and this game's key layout |
| `src/portable_data.*` | Saves / profile / shader cache in `userdata/` next to the exe (copies old per-user saves once) |
| `src/display_setup.*` | Monitor list / native resolution, Auto render resolution, live window move and resize |
| `tools/` | Package/XEX extraction (STFS, XEX2 basic + LZX), pointer-function scan, label check |
| `.github/workflows/build-windows.yml` | CI: LLVM + prebuilt SDK (darkuni/rexglue-sdk-builds, v0.9.0-p23) + game; uploads the package, attaches it to releases |
| `tools/build_linux.sh`, `.github/workflows/build-linux.yml` | Linux: SDK + patches + game, staged in `dist-linux/` |

## Codegen fixes so far
- `0x824A7C68`, `0x824D9DD8`, `0x82441BA8`, `0x824D92B0`: small thunks reached only by `b` from other functions.
- 58 functions reachable only through vtables/callbacks (`tools/scan_pointer_funcs.py`).

## Disclaimer
Unofficial fan project, not affiliated with or endorsed by SEGA or Microsoft. This repository and its releases contain
no game data (graphics, sound, levels): those come from your own package. `generated/` and the release exe contain code
machine-translated from the game's executable. The project's own code is under the [MIT License](LICENSE); the ReXGlue
SDK is BSD-3-Clause (portions derived from Xenia), and its licenses ship in the release zip.
