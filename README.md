# HPR-Store

A cross-platform native extension and store manager UI for **HPR** (Human Pattern Recorder), built with **C++**, **Slint**, **Lua**, and **libarchive**.

It allows browsing, filtering, installing, and deleting extensions and themes for HPR directly from inside the HPR application interface.

> [!IMPORTANT]
> **This is not a standalone application.** HPR-Store is itself an HPR extension — a plugin you drop into HPR's extension folder. Once installed, it acts as a store manager that lets you browse and install other extensions and themes from within HPR's UI.

---

## ⚙️ Required HPR Setting

Before HPR-Store can work, you **must** enable the following option in HPR's settings panel:

**Settings → Extensions → Allow Extensions to Load Native Libraries**

> [!WARNING]
> This setting allows Lua extensions to call `package.loadlib` / `require` to load external `.dll` or `.so` libraries. HPR warns that this bypasses sandbox restrictions — **only enable this for extensions you trust.**

---

## 🚀 Installation (Quick Start)

No building required if you grab a pre-built release.

1. **Download** the latest release zip from the [Releases](https://github.com/plexescor/HPR-Store/releases) page.
2. **Extract** the zip — you will get a folder named `HPR-Store/` containing three files:
   - `HPR-Store.lua`
   - `libHPR-Store.so` (Linux) **or** `HPR-Store.dll` (Windows)
   - `registry.json`
3. **Copy** the entire `HPR-Store/` folder to HPR's extension directory:
   - **Linux:** `~/.config/HPR/extensions/`
   - **Windows:** `%APPDATA%\HPR\HPR_Config\extensions\`
4. **Open HPR**, go to **Settings → Extensions**, and enable **"Allow Extensions to Load Native Libraries"**.
5. **Restart HPR** (or press the rescan button in the Extensions menu). HPR-Store will appear in your extensions list.
6. Click the **Action** button on HPR-Store to open the store window.

---

## Technical Overview

### 1. The Lua Entrypoint (`src/HPR-Store.lua`)
- Registers metadata with HPR (`HPR.extensionName = "HPR Store"`, `HPR.authorName = "Plexescor"`, and `versionSupport`).
- Resolves the absolute directory of the extension at runtime and loads the native library (`libHPR-Store.so` or `libHPR-Store.dll`) using `package.loadlib`.
- Binds HPR lifecycle callbacks (`init`, `onTick`, `onExit`, `onAction`) to the loaded native library symbols (`initialize`, `destroy`, `showUi`).
- When the extension action is triggered in HPR, it calls `showUi()` to display the store interface.

### 2. The Native Library (`src/library.cpp`)
- Exposes three `extern "C"` functions expected by the Lua script:
  - `initialize(lua_State* L)`: Instantiates the global `GUI` object on the Slint event loop thread.
  - `destroy(lua_State* L)`: Hides and resets the `GUI` object safely on the Slint thread.
  - `showUi(lua_State* L)`: Shows the Slint window on the screen.

### 3. The UI Controller (`src/GUI.cpp`)
- Manages the main `StoreWindow` Slint interface instance, the `RegistryManager` backend, and the `UIEventBridge` that hooks up C++ event logic to UI callbacks.

### 4. Registry and Local Database (`src/registryManager.cpp`)
- Loads local registry entries from `registry.json` stored in HPR's extension directory.
- Fetches remote updates directly from the GitHub repository database using `ixwebsocket`'s HTTP client.
- Filters and pages the store item catalog based on chosen type (Extensions, Themes, or Both) and installation status.
- Implements sorting modes (Star count, Download count, and Default shuffle).

### 5. Installer Engine (`src/installer.cpp`)
- Handles downloading binary release zip archives asynchronously via `ix::HttpClient`.
- Extracts archives dynamically to a temporary directory using **libarchive**.
- Performs root folder identification inside the archive:
  - **Themes**: Looks for a directory containing both `metadata.csv` and `app-window.slint`.
  - **Extensions**: Looks for the directory containing the first `.lua` entry.
- Normalizes root folder names if the extracted top-level folder name is `"extracted"`:
  - Theme folders are named after the name field value inside `metadata.csv` (lowercased and sanitized).
  - Extension folders are named after the stem of their `.lua` file (e.g. `extension` from `extension.lua`).
- Copies the identified folder to HPR's configuration directories (`~/.config/HPR/` on Linux / `%APPDATA%/HPR/HPR_Config/` on Windows).
- Automatically writes a record of installed items in `HPR-Store`'s local config folder (`~/.config/HPR-Store/installed.json` or Windows equivalent) to keep track of installed status.
- Handles clean uninstallation by recursively deleting the corresponding theme or extension folder from HPR and removing it from the database.

---

## Dependencies

### Linux
- **Slint Compiler & Library**
- **libarchive** (extracts `.zip` and `.tar` archives)
- **ixwebsocket** (performs HTTP database fetches and package downloads)
- **nlohmann_json** (parses databases and registry JSON files)
- **openssl** / **zlib**

Install system dependencies:
```bash
# Ubuntu / Debian
sudo apt install build-essential cmake libarchive-dev libssl-dev zlib1g-dev

# Arch Linux
sudo pacman -S base-devel cmake libarchive openssl zlib
```

### Windows
- **Visual Studio 2022** / MSVC C++ Desktop Development tools
- **CMake** (version >= 3.21)
- **vcpkg** (for OpenSSL static linking)
- libarchive (fetched automatically via FetchContent)

#### Setting up vcpkg + OpenSSL + zlib (one-time)
```bash
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
vcpkg install openssl:x64-windows-static zlib:x64-windows-static
```

#### Build Instructions (Windows)
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_PREFIX_PATH="C:/Program Files/Slint-cpp 1.16.1"
cmake --build . --config Release
```

---

## Build Instructions

To build the HPR-Store library shared object:

> [!NOTE]
> **Windows users:** Use the build commands from the [Windows Build Instructions](#build-instructions-windows) section above instead of the generic `cmake ..` line. The toolchain file and static triplet flags are required for OpenSSL + zlib to link statically into the DLL.

```bash
# 1. Create and enter build folder
mkdir build
cd build

# 2. Configure project
cmake .. -DCMAKE_BUILD_TYPE=Release

# 3. Compile
cmake --build . --config Release
```

The post-build step in CMake will automatically:
1. Copy the compiled shared library (`libHPR-Store.so` or `HPR-Store.dll`) to HPR's active extensions directory (`~/.config/HPR/extensions/HPR-Store/` or `%APPDATA%/HPR/HPR_Config/extensions/HPR-Store/`).
2. Copy `HPR-Store.lua` and `registry.json` next to the shared library.

---

## Installation in HPR

HPR automatically scans its config directories for dynamic extensions on startup.

To manually install the store into HPR:
1. Locate the config extension folder:
   - **Linux:** `~/.config/HPR/extensions/`
   - **Windows:** `%APPDATA%/HPR/HPR_Config/extensions/`
2. Create a folder named `HPR-Store/`.
3. Place the following three files inside the `HPR-Store/` directory:
   - `HPR-Store.lua` (the Lua script)
   - `registry.json` (the local database seed)
   - `libHPR-Store.so` (on Linux) / `HPR-Store.dll` (on Windows)
4. Start HPR. The extension will initialize and can be opened from HPR's Action Bar.
