# WORR Build and Runtime Getting Started Guide

## Open the Visual Studio Solution
1. Install Visual Studio 2022 (Community or higher) with the **Desktop development with C++** workload.
2. Clone this repository and open `src/WORR.sln` from the repository root (`File` → `Open` → `Project/Solution`).
3. The solution contains a single `game` project that builds the Quake II game DLL.

The solution only defines x64 configurations. Use the toolbar drop-down to choose either `Debug|x64` or `Release|x64` before compiling.【F:src/WORR.sln†L5-L18】

## Select a Build Configuration and Produce the DLL
1. After selecting the desired configuration, build the `game` project (`Build` → `Build game`).
2. The project is configured as a dynamic library (`ConfigurationType=DynamicLibrary`) named `game_x64` for both Debug and Release builds.【F:src/game.vcxproj†L21-L45】【F:src/game.vcxproj†L55-L64】
3. By default, Release builds emit the DLL one level above the `src/` folder (the repository root), while Debug builds place artifacts two levels above `src/` (helpful when testing in a Quake II install that sits beside the source tree). Both configurations produce `game_x64.dll` in their respective output directories.【F:src/game.vcxproj†L55-L64】
4. Copy the resulting DLL into your Quake II Remastered installation’s `baseq2` module folder when you are ready to test in-game.

> **Tip:** If you prefer the Debug build to stay inside the repository, adjust the `OutDir` property inside the project settings before compiling.

## Dependency Management with vcpkg
The project’s vcpkg manifest (`src/vcpkg.json`) lists the libraries that are restored automatically when manifest mode is enabled:

- `fmt`
- `jsoncpp`

The manifest also pins the repository baseline (`bffcbb75f7…`) for reproducible builds.【F:src/vcpkg.json†L1-L10】 The Visual Studio project enables manifest integration (`<VcpkgEnableManifest>true</VcpkgEnableManifest>`) and links the libraries statically.【F:src/game.vcxproj†L65-L75】

### Enabling Manifest Mode
1. Install vcpkg (https://github.com/microsoft/vcpkg) and ensure `VCPKG_ROOT` points to your vcpkg installation.
2. Run `vcpkg integrate install` once so Visual Studio locates your manifest-based dependencies automatically.
3. When building from the command line, pass manifest flags explicitly, for example:
   ```powershell
   msbuild src/WORR.sln -p:Configuration=Release -p:Platform=x64 -p:VcpkgManifestMode=On -p:VcpkgRoot="C:\path\to\vcpkg"
   ```
   Alternatively, set `VCPKG_FEATURE_FLAGS=manifests` in your environment before invoking `msbuild` or `devenv`.

With manifest mode active, vcpkg will install the required `fmt` and `jsoncpp` packages automatically the first time you build the solution.

## Runtime File Layout and First-Run Setup
The game module expects to run inside a Quake II “baseq2” directory tree (`GAMEVERSION = "baseq2"`).【F:src/server/g_local.hpp†L15-L18】 Keep the following layout in mind:

### Player Configuration Files (`baseq2/pcfg`)
- Each player receives a JSON profile stored under `baseq2/pcfg/<social-id>.json`.
- The server ensures the `pcfg` folder exists on startup; missing folders trigger a warning and are created when possible.【F:src/server/gameplay/g_client_cfg.cpp†L36-L103】
- On first connect, the code writes a default JSON document with ratings, stats, and UI preferences if the player’s file does not exist.【F:src/server/gameplay/g_client_cfg.cpp†L66-L118】

**First run checklist:**
1. Create the `baseq2/pcfg` directory alongside your DLL if it is not already present.
2. Ensure the Windows account running the server has write access so the game can create and update player profiles.

### Map Pool and Rotation Files
The map system reads two files from `baseq2/`:

1. **Map pool database** (`mapdb.json`, controlled by `g_maps_pool_file`): lists every map that can appear in votes or cycles.【F:src/server/gameplay/g_main.cpp†L910-L915】【F:src/server/gameplay/g_map_manager.cpp†L365-L434】
   - The JSON file must include a top-level `maps` array.
   - Each entry needs at least `"bsp"` (the map file without the `.bsp` extension) and `"dm": true` to mark it as deathmatch-compatible.
   - Optional fields such as `title`, `min`, `max`, `gametype`, `ruleset`, and custom resource flags are honored if present.【F:src/server/gameplay/g_map_manager.cpp†L388-L432】
   - When missing or malformed, the loader prints `[MapPool]` warnings to the server console.

2. **Map cycle list** (`mapcycle.txt`, controlled by `g_maps_cycle_file`): identifies which maps should be eligible for automatic cycling.【F:src/server/gameplay/g_main.cpp†L910-L915】【F:src/server/gameplay/g_map_manager.cpp†L436-L498】
   - The file is parsed as plain text; whitespace-separated tokens are matched against the pool.
   - Single-line (`// ...`) and block (`/* ... */`) comments are stripped automatically, so you can annotate the file.【F:src/server/gameplay/g_map_manager.cpp†L460-L489】
   - Tokens that do not match a pool entry are reported as ignored.

**First run checklist:**
1. Copy or author a `mapdb.json` file in `baseq2/` with at least one valid entry. For example:
   ```json
   {
     "maps": [
       { "bsp": "q2dm1", "dm": true, "title": "The Edge" }
     ]
   }
   ```
2. Create `mapcycle.txt` in `baseq2/` and list the BSP names (e.g., `q2dm1 q2dm2`).
3. Restart the server or issue the `load_mappool` command after editing to reload the data.【F:src/server/gameplay/g_map_manager.cpp†L368-L435】

Completing the steps above gives you a working development environment: Visual Studio builds the DLL with manifest-restored dependencies, and the runtime folder structure matches what the server expects on first launch.
