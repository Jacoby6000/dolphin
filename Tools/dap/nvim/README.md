# Neovim Client Setup

[nvim-dap](https://github.com/mfussenegger/nvim-dap) can connect to a running Dolphin
server directly or use the bundled integration to start Dolphin.

## Requirements

Install `nvim-dap` with your preferred plugin manager.

## Basic Attach Configuration

This minimal configuration connects to Dolphin on TCP port `5678` and does not require
the bundled integration:

```lua
local dap = require("dap")

dap.adapters.dolphin = {
  type = "server",
  host = "127.0.0.1",
  port = 5678,
}

local config = {
  name = "Attach to Dolphin",
  type = "dolphin",
  request = "attach",
}

for _, language in ipairs({ "c", "cpp" }) do
  dap.configurations[language] = dap.configurations[language] or {}
  table.insert(dap.configurations[language], config)
end
```

Start Dolphin using one of the commands in
[`Running the server`](../README.md#running-the-server), then use `:DapContinue` and
select **Attach to Dolphin**.

## Bundled Integration

The bundled integration can start Dolphin, resolve source paths, and create several
launch and attach configurations. Its optional default debugger interface uses
[`nvim-dap-ui`](https://github.com/rcarriga/nvim-dap-ui) and
[`nvim-nio`](https://github.com/nvim-neotest/nvim-nio).

Add `Tools/dap/nvim` from this Dolphin checkout to Neovim's runtime path, then initialize
the integration:

```lua
vim.opt.rtp:append("/path/to/dolphin/Tools/dap/nvim")
require("dolphin-dap").setup()
```

Use your preferred plugin manager or Neovim configuration layout. No particular keymaps
are required. If you do not use `nvim-dap-ui`, initialize with:

```lua
require("dolphin-dap").setup({ setup_dap_ui = false })
```

A portable `lazy.nvim` example is available in [`lazy-dolphin-dap.lua`](lazy-dolphin-dap.lua).

## Project Configuration

Copy [`.dolphin-dap.example.lua`](.dolphin-dap.example.lua) to `.dolphin-dap.lua` in
the root of the project you want to debug.

For a fully linked decomp build, configure both the ELF and ISO:

```lua
return {
  dolphin = "/path/to/dolphin-emu-nogui",
  program = "/path/to/main.elf",
  disc = "/path/to/game.iso",
  source_paths = {
    "/path/to/project/src",
  },
  enable_cheats = false,
}
```

The ISO supplies the game files and disc environment. Dolphin ignores the ISO's DOL and
executes the ELF, whose symbols and DWARF match the running code.

For a partially decompiled project, run the ISO's DOL and use an address-matching ELF as
a metadata sidecar:

```lua
return {
  dolphin = "/path/to/dolphin-emu-nogui",
  program = "/path/to/game.iso",
  elf = "/path/to/main.elf",
  source_paths = {
    "/path/to/project/src",
  },
}
```

The sidecar does not replace the ISO's DOL. Use it only when its code and data addresses
exactly match the running DOL.

Source stepping and locals are unreliable in optimized source files. Build the files you
need to inspect without optimization; unrelated files can remain optimized.

## Starting a Session

Use `:DapContinue` or another standard `nvim-dap` command and select one of the registered
Dolphin configurations. Define keymaps using the normal `nvim-dap` functions if desired.

To start Dolphin separately, use a command from the main
[`Running the server`](../README.md#running-the-server) guide, then select the Dolphin
attach configuration in Neovim. `:DolphinDapCmd` prints and copies a command generated
from the current `.dolphin-dap.lua` file.

The configuration supports:

- `dolphin`: path to `dolphin-emu-nogui`.
- `dolphin_gui`: optional path to the Qt Dolphin executable.
- `program`: ELF, DOL, or ISO to execute.
- `disc`: ISO mounted while executing an ELF or DOL.
- `elf`: metadata-only debug ELF for the executable selected by `program`.
- `source_paths`: ordered directories used to locate source files; spawned Dolphin instances also
  receive them through the global `Dolphin.Debug.SourcePaths` setting.
- `host` and `port`: TCP attach address, defaulting to `127.0.0.1:5678`.
- `socket`: Unix socket path for local attach.
- `platform`: video platform for the NoGUI executable.
- `enable_cheats`: whether saved cheats are enabled for this launch.
- `cwd`: optional Dolphin working directory.

See [Source debugging with DWARF](../README.md#source-debugging-with-dwarf) for the
difference between executed and sidecar ELFs.
