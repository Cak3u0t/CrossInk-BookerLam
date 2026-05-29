# 📁 CrossInk‑BookerLam Repository Overview

**Purpose** – A quick‑reference markdown that gives a senior firmware engineer a clear mental map of the project layout, key constraints, and typical verification steps. Place this file in the artifact directory for future browsing.

---

## 📂 Directory Map & Core Areas

| Area | Description | Key Files / Directories |
|------|------------|--------------------------|
| **Activity System** | UI screens, navigation flow, lifecycle (`onEnter`/`onExit`). | `src/Activity.cpp`, `src/Activity.h`, `src/ActivityManager.cpp`, `src/ActivityManager.h` |
| **Settings & State** | Persistent configuration, runtime state, JSON I/O. | `src/CrossPointSettings.*`, `src/CrossPointState.*`, `src/JsonSettingsIO.*` |
| **HAL / Platform Layer** | Abstracts ESP32‑C3 peripherals (GPIO, SPI, SD, display). | `src/hal/` |
| **Font & I18n** | On‑SD‑card font install, language packs, UI typography. | `src/FontInstaller.*`, `src/SdCardFontSystem.*`, `src/fontIds.h` |
| **Rendering Pipeline** | E‑ink rasterisation, bitmap conversion, caching. | `src/GfxRenderer/`, `src/InflateReader/`, `src/Serialization/` |
| **EPUB / TXT / XTC Parsers** | Book format parsing, cache generation. | `src/Epub/`, `src/Txt/`, `src/Xtc/` |
| **Network & OPDS** | Wi‑Fi, HTTP client, OPDS catalog handling, web server. | `src/network/`, `src/OpdsServerStore.*`, `src/JsonParser.*` |
| **Simulator Support** | SDL2‑based desktop emulator for rapid iteration. | `simulator/` (env `simulator` in `platformio.ini`) |
| **UI Components** | Buttons, menus, home screen, reader UI, settings screens. | `src/components/` (subfolders `home/`, `reader/`, `settings/`, `browser/`) |
| **Caching / Persistence** | `.crosspoint/` on SD card holds settings, caches, stats. | See `.claude/CONTEXT.md` § "Real Hardware / Storage" |
| **Build System** | PlatformIO with size‑constrained variants (`tiny`, `xlarge`, `no_emoji`, `simulator`). | `platformio.ini` |

---

## 🔑 Architectural Highlights

1. **Activity Lifecycle** – Activities are heap‑allocated; allocate long‑lived buffers in `onEnter()` and free in `onExit()`. Typical stack sizes: 2048 B (simple) or 4096 B (network/EPUB). 
2. **Memory Discipline** – Usable RAM ≈ 380 KB. Use `static const` for large tables (fonts, language data) so they live in flash. Avoid `std::string` in hot loops; prefer `char[]`, `string_view`, `snprintf`. 
3. **File‑IO Constraints** – `FsFile` (SdFat wrapper) allows **one reader per file path** at a time. Close handles before reopening. Cache directories under `.crosspoint/epub_<hash>/` store binary render data. 
4. **Button Mapping** – Centralised in `MappedInputManager` via `MappedInputManager::Button::*` enums. Custom front‑button remapping is stored in settings and respected by the reader UI. 
5. **Rendering Path** – Only `GfxRenderer::BW` writes to the e‑ink framebuffer (48 KB). Image handling delegated to `InflateReader` (PNG) and `JpegToBmpConverter` (JPEG). Sim‑stubbed in the simulator. 
6. **Simulator vs Real HW** – Simulator disables image decoders, uses POSIX file I/O, and makes `esp_deep_sleep_start()` a no‑op. Use it for UI/logic iteration; test heap‑intensive paths on actual hardware. 
7. **Build Variants** – `tiny` (minimal fonts, no Emoji, smallest RAM); `xlarge` (larger fonts, Emoji, higher RAM); `no_emoji` (all fonts, no Emoji). Choose variant in `pio run -e <variant>`. 
8. **Logging** – Use `LOG_INF`, `LOG_DBG`, `LOG_ERR`. Verbose Wi‑Fi logs are enabled as of v1.3.0. 

---

## ✅ Verification Checklist (after any change)

1. **Compile** for the intended target (`pio run -e tiny|xlarge|simulator`).
2. **Run** on the simulator first; watch heap usage warnings.
3. **Flash** to hardware (if applicable) and capture logs via `scripts/debugging_monitor.py`.
4. **Validate**:
   - No "out of memory" reboots.
   - UI actions respond without lag.
   - Cached `.crosspoint` folders update correctly (e.g., after font changes, delete the affected book cache). 
5. **Run** a quick functional test: open a book, toggle a UI option, and verify the state persists after a restart.

---

## 📚 Where to Dive Deeper
- **AGENTS.md** – Canonical repo instruction file (role definition, core rules).
- **.claude/CONTEXT.md** – Durable repo‑specific gotchas (simulator limits, hardware storage rules).
- **README.md** – High‑level features & quick‑start guide.
- **CHANGELOG.md** – Recent bug‑fixes that may affect your area.

---

*Created on 2026‑05‑27. Keep this file as your go‑to reference when navigating the CrossInk‑BookerLam firmware.*
