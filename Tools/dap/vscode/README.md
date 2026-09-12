# Dolphin DAP Client

This extension registers Dolphin as a debugger and connects the editor directly to a
running Dolphin DAP server. It does not start LLDB or GDB.

Package and install it with your editor's command-line interface:

```bash
cd /path/to/dolphin/Tools/dap/vscode
npx @vscode/vsce package
code --install-extension dolphin-dap-client-0.1.1.vsix
```

Code - OSS users should replace `code` with `code-oss`.

Add ordered source roots when the ELF's DWARF contains relative or basename-only paths:

```json
"sourcePaths": [
  "${workspaceFolder}/src",
  "${workspaceFolder}/extern/dolphin/src"
]
```
