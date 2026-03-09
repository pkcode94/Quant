# Building Quant in VS Code

## Prerequisites
- Visual Studio 2026 Community Edition (with C++ tools) installed
- VS Code with C++ extension (ms-vscode.cpptools)
- Windows 10/11

## Setup

1. **Install VS Code Extensions**
   - Open extensions (Ctrl+Shift+X)
   - Install recommended extensions from `.vscode/extensions.json`
   - Key extension: `C/C++ Extension Pack` by Microsoft

2. **Verify Setup**
   - Press `Ctrl+Shift+P` and run "CMake: Configure"
   - Or just proceed to build

## Build Tasks

Access via `Terminal` → `Run Task` or press `Ctrl+Shift+B`:

| Task | Command | Purpose |
|------|---------|---------|
| **Build Quant (Debug, x64)** | Ctrl+Shift+B | Default build (Debug mode) |
| Build Quant (Release, x64) | - | Release mode (optimized) |
| Rebuild Quant (Debug, x64) | - | Full clean + rebuild |
| Clean Quant | - | Delete build artifacts |
| Run Quant (server-only) | - | Start HTTP server on port 8080 |

## Quick Compile & Run

```bash
# In VS Code terminal:
Ctrl+Shift+B                          # Build (default task)
Ctrl+Shift+P > Run Task > Run Quant   # Start server
```

## Debugging

1. Press `F5` or `Ctrl+Shift+D` → "Quant (server-only)"
2. Breakpoints will pause execution
3. IntelliSense (Ctrl+Space) provides code completion

## Troubleshooting

### Build Fails with "MSBuild not found"
- Check Tools → Options → Projects and Solutions → VC++ Project Settings
- Ensure VS 2026 Community with C++ is installed

### IntelliSense not working
- Press `Ctrl+Shift+P` and run "C/C++: Rescan Solutions"
- Close and reopen the workspace

### Header includes not found
- Verify `.vscode/c_cpp_properties.json` paths match your VisualStudio installation
- Update `compilerPath` if using a different MSVC version

## Project Structure

```
d:\q\
├── .vscode/              ← VS Code configuration (NEW)
│   ├── tasks.json       ← Build tasks
│   ├── launch.json      ← Debug config
│   ├── c_cpp_properties.json ← IntelliSense paths
│   ├── settings.json    ← Editor settings
│   └── extensions.json  ← Recommended extensions
├── Quant/               ← Main project
│   ├── Quant.vcxproj   ← Project file
│   ├── Quant.cpp       ← Main source
│   └── ... (headers)
├── libquant/            ← Library code
├── libquant-engine/     ← Engine library
├── libquant-mcp/        ← MCP library
├── x64/Debug/           ← Build output
│   └── Quant.exe       ← Compiled executable
└── users/admin/db/      ← Runtime database
    └── trades.json     ← Trade data (6 trades)
```

## Database Note

- **MCP writes to:** `users/admin/db/trades.json`
- **GUI reads from:** `users/admin/db/trades.json` (per authenticated user)
- **Root `db/trades.json`** is empty and ignored

## Clearing Browser Cache

If GUI shows only 1 trade instead of 6:
- **Hard refresh:** Ctrl+Shift+R (or Cmd+Shift+R on Mac)
- Clear browser cache entirely
- Reopen http://localhost:8080
