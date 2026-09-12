# Neovim + Dolphin DAP

Dolphin **is** the DAP adapter — there is no separate debug adapter binary. Neovim
connects over TCP (or a Unix socket on Linux) after Dolphin is listening.

## lazy.nvim setup

1. Copy the lazy spec into your Neovim config:

   ```bash
   cp Tools/dap/nvim/lazy-dolphin-dap.lua ~/.config/nvim/lua/plugins/dolphin-dap.lua
   ```

   Edit `DOLPHIN_DAP_RT` at the top of that file if your checkout path differs.

2. Remove or merge duplicate keymaps from an existing `lua/plugins/nvim-dap.lua`
   (the dolphin plugin spec already registers `<leader>d*` maps).

3. Add per-project settings at the game/decomp root:

   ```bash
   cp Tools/dap/nvim/.dolphin-dap.example.lua /path/to/melee/.dolphin-dap.lua
   # edit the ELF, ISO, and Dolphin binary paths
   ```

Buffer diagnostics moved to `<leader>ld` so `<leader>d*` is free for DAP (see `dolphin-dap.lua`).

4. `:Lazy sync`, restart Neovim, open a `.c` file in your decomp tree, then:

   - `<leader>dA` — attach to a running Dolphin (manual terminal workflow)
   - `<leader>da` — pick attach / launch configuration
   - `<leader>dc` — attach if needed, then continue (unpause the game)
   - `<leader>du` — toggle DAP UI
   - `:DolphinDapCmd` — copy a manual terminal launch command (attach workflow)

## Workflows

### Attach (Dolphin already running)

Terminal:

```bash
dolphin-emu-nogui \
  -C Dolphin.General.DAPPort=5678 \
  -C Dolphin.Core.DefaultISO=/path/to/game.iso \
  -C Dolphin.Core.BootExecutableWithDefaultDisc=true \
  --exec /path/to/main.elf \
  --platform headless
```

Wait for boot (or `ss -tlnp | grep 5678`), then in Neovim choose **Dolphin attach (:5678)**.

**Important:** `-C` must use the `Dolphin` system prefix (`Dolphin.General.DAPPort=5678`).
`Main.General.DAPPort=5678` is silently ignored and DAP will not listen.

GUI build (same flags, no `--platform headless`):

```bash
dolphin-emu \
  -C Dolphin.General.DAPPort=5678 \
  -C Dolphin.Core.DefaultISO=/path/to/game.iso \
  -C Dolphin.Core.BootExecutableWithDefaultDisc=true \
  --exec /path/to/main.elf
```

Or pick **Dolphin attach (Qt spawn, :5678)** in Neovim to start `dolphin-emu` and
connect on the configured port.

Nogui with an emulator window (no Qt UI, but video output):

```bash
dolphin-emu-nogui \
  -C Dolphin.General.DAPPort=5678 \
  -C Dolphin.Core.DefaultISO=/path/to/game.iso \
  -C Dolphin.Core.BootExecutableWithDefaultDisc=true \
  --exec /path/to/main.elf \
  --platform x11
```

On Linux use `x11`; on Windows `win32`; on macOS `macos`. Or pick **Dolphin launch
nogui (video, x11)** / **Dolphin attach (nogui video spawn, :5678)** in Neovim.

### Launch (Neovim starts Dolphin)

For source-level decomp debugging, specify both inputs: set `program` to the built ELF
and `disc` to the corresponding game ISO. Neovim spawns Dolphin with a dynamic DAP
port and connects automatically.

```lua
return {
  dolphin = "/path/to/build/Binaries/dolphin-emu-nogui",
  program = "/path/to/melee/build/GALE01/main.elf",
  disc = "/path/to/melee.iso",
  source_paths = { "/path/to/melee/src", "/path/to/melee/extern/dolphin/src" },
  enable_cheats = false,
}
```

`disc` mounts the ISO and enables its bootstrap environment. Dolphin uses the ISO to
establish the disc ID, FST, OS state, and DVD/filesystem access, but ignores the DOL in
the ISO as the program to execute. It loads and executes `program` instead, so the ELF's
memory layout, symbols, and embedded DWARF remain authoritative.

Do not set `program` to the ISO and `elf` to a separately linked decomp ELF. In that
configuration the ISO's DOL executes and `elf` supplies metadata only; source addresses
are wrong unless the ELF has exactly the same link layout as that DOL. The `program`
field still accepts ELF, DOL, and disc images for other workflows, and legacy `iso` is
accepted as an alias for `program`.
Set `enable_cheats = false` when codes saved for another executable layout must not run.

| Config | Binary | Platform |
|--------|--------|----------|
| **Dolphin launch headless** | `dolphin-emu-nogui` | `headless` (no video) |
| **Dolphin launch nogui (video, …)** | `dolphin-emu-nogui` | `x11` / `win32` / `macos` |
| **Dolphin launch (Qt)** | `dolphin-emu` | _(none — full Qt UI)_ |

Set `platform = "x11"` (or `"auto"`) in `.dolphin-dap.lua` to override the default
video backend. Optional `dolphin_gui` overrides the Qt binary path (defaults to
`dolphin` with `-nogui` stripped).

## Source paths / DWARF

The recommended decomp workflow boots the ELF as `program` and mounts the ISO as
`disc`; the ELF supplies both the executed code and its embedded symbols/DWARF.
`--debug-elf` and the `elf` setting are metadata-only sidecar modes and do not replace
the executable loaded from `program`.

Source stepping and locals do not work reliably in optimized source files (translation
units). Build the files you need to debug without optimization; otherwise stepping may
skip lines and locals may be missing or incorrect. Other files can remain optimized.

Set ordered `source_paths` roots when old MWCC DWARF reports only basenames. The
integration tries a direct relative path, then a recursive basename lookup within each
root. It does not guess when a root contains multiple matching files; use a narrower
root in that case.

## Unix socket (Linux)

Start Dolphin with:

```bash
dolphin-emu-nogui \
  -C Dolphin.General.DAPSocket=/tmp/dolphin-dap.sock \
  -C Dolphin.Core.DefaultISO=/path/to/game.iso \
  -C Dolphin.Core.BootExecutableWithDefaultDisc=true \
  --exec /path/to/main.elf
```

Or set in `~/.config/dolphin-emu/Dolphin.ini`:

```ini
[General]
DAPSocket = /tmp/dolphin-dap.sock
```

Set the same path in `.dolphin-dap.lua` as `socket = "/tmp/dolphin-dap.sock"` and
use **Dolphin attach (unix socket)**.

GDB and DAP are mutually exclusive — do not enable `GDBPort` at the same time.
