# Visual Studio Code Client Setup

The bundled extension registers Dolphin as a debugger in Visual Studio Code and connects
to a running Dolphin DAP server.

## Install

Package and install the extension:

```bash
cd /path/to/dolphin/Tools/dap/vscode
npx @vscode/vsce package
code --install-extension dolphin-dap-client-0.1.0.vsix
```

## Configure

Create `.vscode/launch.json` in the project you want to debug:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Attach to Dolphin",
      "type": "dolphin",
      "request": "attach",
      "host": "127.0.0.1",
      "port": 5678,
      "stopOnEntry": true
    }
  ]
}
```

Start Dolphin with TCP port `5678` using one of the modes in
[`Running the server`](../README.md#running-the-server). Then open **Run and Debug**,
select **Attach to Dolphin**, and start debugging.

For a local Unix socket, replace `host` and `port` with:

```json
"socket": "/tmp/dolphin-dap.sock"
```

Source stepping and locals are unreliable in optimized source files. Build the files you
need to inspect without optimization; unrelated files can remain optimized.
