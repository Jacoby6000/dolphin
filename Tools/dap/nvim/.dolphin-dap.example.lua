--- Copy to your decomp / game project root as `.dolphin-dap.lua`.
--- Paths may use `~` or `$HOME`; they are expanded at runtime.
---
--- Manual attach (terminal): run `:DolphinDapCmd` in Neovim to copy a ready-made
--- command, or use the template below. `-C` must use the `Dolphin` system name
--- (not `Main` — ignored silently). Verify with: ss -tlnp | grep 5678

return {
  -- Built dolphin-emu-nogui (must include DAP support from feature/dap-server).
  dolphin = "~/projects/ai/yolo/dolphin-dap/build/Binaries/dolphin-emu-nogui",

  -- Qt build for GUI attach/launch (defaults to dolphin path with "-nogui" stripped).
  -- dolphin_gui = "~/projects/ai/yolo/dolphin-dap/build/Binaries/dolphin-emu",

  -- ELF, DOL, or disc image to execute. A directly booted ELF supplies its own DWARF.
  program = "~/projects/ai/yolo/melee/build/GALE01/main.elf",

  -- Optional sidecar debug ELF when program is a retail disc image or DOL.
  -- elf = "~/projects/ai/yolo/melee/build/GALE01/main.elf",

  -- TCP port for attach configs (launch uses a dynamic port via nvim-dap).
  port = 5678,

  -- nogui `--platform` when using dolphin-emu-nogui with video (default: auto → x11 on Linux).
  -- platform = "x11", -- or "win32", "macos", "fbdev", "headless", "auto"

  -- Optional: Unix socket attach on Linux (start Dolphin with
  -- `-C Dolphin.General.DAPSocket=/tmp/dolphin-dap.sock`).
  -- socket = "/tmp/dolphin-dap.sock",

  -- Optional: working directory for the launch executable.
  -- cwd = "~/projects/ai/yolo/melee",
}

-- Manual attach example (paste in a terminal, then pick "Dolphin attach (:5678)" in Neovim):
-- dolphin-emu-nogui \
--   -C Dolphin.General.DAPPort=5678 \
--   --exec ~/projects/ai/yolo/melee/build/GALE01/main.elf \
--   --platform x11
