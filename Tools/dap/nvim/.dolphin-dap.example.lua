--- Copy to your decomp / game project root as `.dolphin-dap.lua`.
--- Paths may use `~` or `$HOME`; they are expanded at runtime.
---
--- Manual attach (terminal): run `:DolphinDapCmd` in Neovim to copy a ready-made
--- command, or use the template below. `-C` must use the `Dolphin` system name
--- (not `Main` — ignored silently). Verify with: ss -tlnp | grep 5678

return {
  -- Built dolphin-emu-nogui (must include DAP support from feature/dap-server).
  dolphin = "/path/to/dolphin-emu-nogui",

  -- Qt build for GUI attach/launch (defaults to dolphin path with "-nogui" stripped).
  -- dolphin_gui = "/path/to/dolphin-emu",

  -- The ELF to execute. Its memory layout, symbols, and embedded DWARF are authoritative.
  program = "/path/to/main.elf",

  -- The corresponding game ISO. Dolphin uses its bootstrap and filesystem environment,
  -- but ignores its embedded DOL and executes `program` instead.
  disc = "/path/to/game.iso",

  -- Advanced metadata-only sidecar mode for an executable with the exact same link layout.
  -- This does not replace the executable selected by `program`.
  -- elf = "/path/to/main.elf",

  -- Ordered roots used to resolve basename-only source paths from older DWARF.
  -- Each root should contain unique basenames; ambiguous matches are not guessed.
  source_paths = {
    "/path/to/project/src",
    "/path/to/project/extern/dolphin/src",
  },

  -- Optional launch override; disable codes that target another executable layout.
  enable_cheats = false,

  -- TCP port for attach configs (launch uses a dynamic port via nvim-dap).
  port = 5678,

  -- nogui `--platform` when using dolphin-emu-nogui with video (default: auto → x11 on Linux).
  -- platform = "x11", -- or "win32", "macos", "fbdev", "headless", "auto"

  -- Optional: Unix socket attach on Linux (start Dolphin with
  -- `-C Dolphin.General.DAPSocket=/tmp/dolphin-dap.sock`).
  -- socket = "/tmp/dolphin-dap.sock",

  -- Optional: working directory for the launch executable.
  -- cwd = "/path/to/project",
}

-- Manual attach example (paste in a terminal, then pick "Dolphin attach (:5678)" in Neovim):
-- dolphin-emu-nogui \
--   -C Dolphin.General.DAPPort=5678 \
--   -C Dolphin.Core.DefaultISO=/path/to/game.iso \
--   -C Dolphin.Core.BootExecutableWithDefaultDisc=true \
--   --exec /path/to/main.elf \
--   --platform x11
