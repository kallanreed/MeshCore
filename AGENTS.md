# Repository Guidelines

## Main Project
- only interested in the Heltec T114 NRF TXT variant
- the custom firmware UI is defined in examples/companion_radio/nrf-txt
- avoid modifying common code outside of that folder (ask if you need to)
- keep the UI view model clean and withouth dependencies

## Project Structure & Module Organization
- `src/` holds the core MeshCore implementation; `include/` contains public headers.
- `examples/` provides firmware apps (companion radio, repeater, room server) and is the best place to start for device-specific behavior.
- `variants/` and `arch/` define PlatformIO targets, board configs, and platform-specific helpers.
- `lib/` contains vendored third-party libraries; avoid editing unless necessary.
- `docs/` stores supporting documentation and FAQs.

## Build, Test, and Development Commands
- `pio run -e <env>` builds a specific PlatformIO environment (see `variants/*/platformio.ini`).
- `sh build.sh build-firmware <env>` builds one firmware and writes artifacts to `out/` (requires `FIRMWARE_VERSION` in the environment).
- `sh build.sh build-companion-firmwares` / `build-repeater-firmwares` / `build-room-server-firmwares` build grouped targets.
- `pio device monitor -b 115200` attaches a serial monitor (matches `monitor_speed` in `platformio.ini`).

## Coding Style & Naming Conventions
- Use the existing embedded C++ style: braces on the same line and 2-space indentation.
- Keep code simple and low-level; avoid extra layers or abstractions.
- No dynamic memory allocation except during setup/begin functions.
- Do not reformat existing files; keep diffs focused and minimal.
- For UI previews, INVERSE fill is expected to invert previously drawn content; draw highlight rectangles after text when using INVERT selection.

## Testing Guidelines
- There is no dedicated automated test suite in this repo.
- Validate changes by building the relevant PlatformIO environment and testing on hardware.
- For example apps, verify serial output and mesh behavior using the appropriate client tools.

## Commit & Pull Request Guidelines
- Commit messages follow a short, imperative style (e.g., "Fix sleep on T-Beam", "Add venv dirs to .gitignore").
- Open PRs against the `dev` branch.
- For impactful changes, open an issue first to discuss scope and approach.
- PRs should explain the target device(s), firmware type (companion/repeater/room server), and any manual validation performed.
