# HPR-Store

A cross-platform native extension and store manager UI for **HPR** (Human Pattern Recorder), built with **C++**, **Slint**, **Lua**, and **libarchive**.

It allows browsing, filtering, installing, and deleting extensions and themes for HPR directly from inside the HPR application interface.

> [!IMPORTANT]
> **This is not a standalone application.** HPR-Store is itself an HPR extension — a plugin you drop into HPR's extension folder. Once installed, it acts as a store manager that lets you browse and install other extensions and themes from within HPR's UI.

> [!NOTE]
> **HPR Version Compatibility:** HPR-Store requires **HPR v0.9.7 or higher**. Older versions of HPR lack necessary extension manager APIs (such as `HPR.reloadMyself` and dynamic native library loading support).

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
2. **Extract** the zip — you will get a folder named `HPR-Store/` containing four files:
   - `HPR-Store.lua`
   - `libHPR-Store.so` (Linux)
   - `HPR-Store.dll` (Windows)
   - `registry.json`
3. **Copy** the entire `HPR-Store/` folder to HPR's extension directory:
   - **Linux:** `~/.config/HPR/extensions/`
   - **Windows:** `%APPDATA%\HPR\HPR_Config\extensions\`
4. **Open HPR**, go to **Settings → Extensions**, and enable **"Allow Extensions to Load Native Libraries"**.
5. **Restart HPR** (or press the rescan button in the Extensions menu). HPR-Store will appear in your extensions list.
6. Click the **Action** button on HPR-Store to open the store window.

---

## 🔄 Self-Update

HPR-Store maintains its own release cycle independent of HPR.

When the registry reports a newer version of HPR-Store, an **Update HPR-Store** button appears in the top-left of the store window's title bar. Clicking it:

1. Downloads the new release zip to a temp directory.
2. Renames the currently loaded library to `.old` (avoiding OS file locks on Windows, avoiding in-place overwrite crashes on Linux).
3. Copies the new binary and Lua files into the extension folder.
4. Calls `HPR.reloadMyself()` — HPR hot-reloads the store extension with the new binary, completely without restarting.

Leftover `.old` files from a previous update are cleaned up automatically on next launch.

---

## 📦 Publishing an Extension or Theme

Want your extension listed in HPR-Store? Submit a PR!

1. Create a GitHub release with a `.zip` file containing your extension or theme folder.
2. Fork this repository, add your entry to `registry.json`, and open a Pull Request.
3. Once merged, all HPR-Store users will see your extension on their next database refresh.

### `registry.json` Entry Schema

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | ✅ | Unique kebab-case identifier (e.g. `"my-extension"`) |
| `name` | string | ✅ | Display name shown in the store |
| `author` | string | ✅ | Your name or GitHub username |
| `authorGithub` | string | — | Link to your GitHub profile |
| `description` | string | ✅ | Short one-line description |
| `longDescription` | string | — | Full Markdown description shown in the detail view |
| `version` | string | ✅ | Current version string (e.g. `"0.2"`) |
| `downloadUrl` | string | ✅ | Direct URL to the release `.zip` archive |
| `sourceUrl` | string | — | URL to the source code repository |
| `type` | string | ✅ | `"EXTENSION"` or `"THEME"` |
| `tags` | array | — | Searchable tag strings |
| `supportedHPRVersions` | array | — | List of compatible HPR version strings |
| `previewImages` | array | — | URLs to preview images shown in the UI carousel |
| `downloadCount` | int | — | Download counter (maintained by the registry) |
| `starCount` | int | — | Star counter (used for sort-by-stars) |
| `lastUpdated` | string | — | ISO date of last update (e.g. `"2026-08-09"`) |

---

## 🛠 Building from Source

### Dependencies (Linux)
- **Slint Compiler & Library**
- **libarchive** (extracts `.zip` and `.tar` archives)
- **ixwebsocket** (performs HTTP fetches and package downloads)
- **nlohmann_json** (parses registry JSON files)
- **openssl** / **zlib**

```bash
# Install system dependencies (Ubuntu / Debian)
sudo apt install build-essential cmake libarchive-dev libssl-dev zlib1g-dev

# Arch Linux
sudo pacman -S base-devel cmake libarchive openssl zlib
```

```bash
# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

The post-build step automatically copies the compiled library, `HPR-Store.lua`, and `registry.json` to HPR's active extensions directory.

### Dependencies (Windows)
- **Visual Studio 2022** / MSVC C++ Desktop Development tools
- **CMake** (version >= 3.21)
- **vcpkg** (for OpenSSL and zlib static linking)
- libarchive is fetched automatically via CMake FetchContent

```bash
# Setup vcpkg (one-time)
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
vcpkg install openssl:x64-windows-static zlib:x64-windows-static
```

```bash
# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows-static ^
  -DCMAKE_PREFIX_PATH="C:/Program Files/Slint-cpp 1.16.1"
cmake --build . --config Release
```

---

## 🧩 Technical Architecture

### Lua Entrypoint (`src/HPR-Store.lua`)
Registers HPR metadata and binds lifecycle callbacks (`init`, `onTick`, `onExit`, `onAction`) to native library symbols loaded via `package.loadlib`.

### Native Library (`src/library.cpp`)
Exposes `initialize`, `destroy`, and `showUi` as `extern "C"` symbols. Holds the global `lua_State*` pointer used to bridge C++ back into Lua for lifecycle calls (`HPR.unloadExtension`, `HPR.reloadMyself`, `HPR.refreshExtensions`).

### UI Controller (`src/GUI.cpp`)
Owns the `StoreWindow` Slint instance and wires it to `UIEventBridge`, which maps all user interactions to C++ installer and registry logic.

### Registry & Installer (`src/registryManager.cpp`, `src/installer.cpp`)
- Downloads remote `registry.json` from GitHub via `ixwebsocket`.
- Downloads and extracts `.zip` release archives via `libarchive`.
- Before any file modification, synchronously unloads the target extension via `HPR.unloadExtension` through the live Lua state.
- For self-updates: uses `dladdr` (Linux) and `GetModuleFileNameW` (Windows) to resolve the exact loaded library path, then performs rename-staging before overwriting.
