# Blazingtron 2

Native Windows 11 rewrite of the original blazingtron (Qt/Python) VIP service price calculator.

**Blazingtron 2** is a pure native Win32 GUI application written in **C + x64 Assembly** (NASM).

- No Python, no Qt, no .NET, no runtime dependencies
- Small single .exe (~50-80KB)
- Uses Win32 API directly + SSE2 assembly for the calculation engine ("blazing" fast math)
- Modern look on Windows 11 via Common Controls v6 manifest

## Features (matching original)

- Service value input (live)
- Discount presets: 0% / 7% / 10% / 12%
- Booster cut presets: 30% / 35% / 40% / 50%
- Platform selection: PC / XBOX / PS4 / Sherpa
- Custom note / order info field
- Auto-generated clipboard string: `PLATFORM - €XX.XX - your note`
- One-click **Copy Note** (with "copied!" feedback)
- Exit button
- All calculations performed in hand-written x64 assembly (SSE2)

## Building on Windows 11

### Recommended (easiest)

1. Install **MSYS2** (https://www.msys2.org/)
2. Open **MSYS2 MinGW 64-bit** terminal and run:
   ```bash
   pacman -Syu
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-binutils nasm
   ```
3. Add `C:\msys64\mingw64\bin` to your Windows PATH (or always build from the MSYS2 shell)
4. In the project folder run:
   ```bash
   make
   # or
   .\build.bat
   ```

### Alternative: TDM-GCC + NASM

- Download TDM-GCC (64-bit)
- Download NASM from https://nasm.us (put nasm.exe in PATH)
- Run `build.bat` from a normal Command Prompt

### Using Visual Studio (more work)

You can build with `cl.exe` + `ml64.exe` (from "x64 Native Tools Command Prompt"):

```bat
ml64 /c /Fo calc.obj src\calc.asm
rc /r res\resource.rc
cl /Fe:Blazingtron2.exe /O2 /MD src\main.c calc.obj res\resource.res ^
   user32.lib gdi32.lib comctl32.lib kernel32.lib /link /SUBSYSTEM:WINDOWS
```

(You will need to adjust the resource slightly for MSVC.)

## Running

Just double-click `Blazingtron2.exe` after building.

Default values on start:
- Service: 100.00
- 10% discount
- 30% cut
- PC platform

Change any field/radio and everything updates live. Click **Copy Note** to put the formatted string on the clipboard.

## Project Structure

```
blazingtron-2/
├── src/
│   ├── main.c       # Full Win32 GUI (window, controls, clipboard, message handling)
│   └── calc.asm     # NASM x64 SSE2 price math (calc_vip_price + calc_booster_cut)
├── res/
│   ├── resource.rc
│   └── blazingtron.manifest   # Modern controls + PerMonitorV2 DPI
├── assets/
│   └── blazing.png            # Original icon (convert to .ico for embedding)
├── build.bat
├── Makefile
└── README.md
```

## Why C + Assembly?

The original was Python + Qt (heavy, slow startup, Python runtime required).

Blazingtron 2:
- Starts instantly
- Tiny memory footprint
- All floating-point math runs through custom SSE2 assembly routines
- Pure native Windows 11 executable

## Notes / Limitations

- Decimal separator is `.` (dot) to match original behavior. European users using `,` can modify `ParseServiceValue`.
- Custom icon requires converting `blazing.png` → `blazing.ico` and uncommenting in `resource.rc`.
- No floating toolbar or advanced features were added — faithful functional rebuild + native speed.
- For money values, production use should consider integer cents to avoid any floating-point rounding. Current version matches original float behavior.

## Downloads (Pre-built Binaries)

Pre-compiled portable version (no installation required):

- **Windows 11 x64 Portable**: [Blazingtron2-v2.0-Portable.zip](https://github.com/wizav1337/Blazingtron2/releases/download/v2.0/Blazingtron2-v2.0-Portable.zip)
  - Contains `Blazingtron2.exe` + the two required runtime DLLs
  - Just extract and run

Source code and build scripts are in this repository.

## License

Same as original (see original repo).

---

Built as a native replacement for personal / boosting workflow use on Windows 11.
