#include "GUI.hpp"
#include "lua.hpp"
#include "sol.hpp"
#include <future>
#include <iostream>
#include <memory>

#ifdef _WIN32
#define HPR_EXPORT __declspec(dllexport)
#include <windows.h>
#else
#define HPR_EXPORT
#endif

static std::unique_ptr<GUI> gui;
static lua_State *g_L = nullptr;
static std::mutex g_luaMutex;
static std::string g_hprStoreVersion = "0.1";

std::string getHprStoreCurrentVersion() { return g_hprStoreVersion; }

void reloadMyselfViaLua() {
  std::lock_guard<std::mutex> lock(g_luaMutex);
  if (!g_L) {
    std::cerr
        << "[HPR-Store-Library] reloadMyselfViaLua: lua_State pointer is null!"
        << std::endl;
    return;
  }

  std::cout << "[HPR-Store-Library] Calling HPR.reloadMyself() / "
               "HPR.reloadMyself_E() via Lua state..."
            << std::endl;
  try {
    sol::state_view lua(g_L);
    if (lua["HPR"].valid()) {
      sol::protected_function reloadFn = lua["HPR"]["reloadMyself"];
      if (!reloadFn.valid()) {
        reloadFn = lua["HPR"]["reloadMyself_E"];
      }

      if (reloadFn.valid()) {
        sol::protected_function_result res = reloadFn();
        if (!res.valid()) {
          sol::error err = res;
          std::cerr << "[HPR-Store-Library] Lua reloadMyself error: "
                    << err.what() << std::endl;
        } else {
          std::cout
              << "[HPR-Store-Library] Lua reloadMyself triggered successfully."
              << std::endl;
        }
      } else {
        std::cerr << "[HPR-Store-Library] reloadMyself function not found in "
                     "HPR table!"
                  << std::endl;
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "[HPR-Store-Library] Exception during Lua reloadMyself: "
              << e.what() << std::endl;
  }
}

void refreshExtensionsViaLua() {
  std::lock_guard<std::mutex> lock(g_luaMutex);
  if (!g_L) {
    std::cerr << "[HPR-Store-Library] refreshExtensionsViaLua: lua_State "
                 "pointer is null!"
              << std::endl;
    return;
  }

  std::cout << "[HPR-Store-Library] Calling HPR.refreshExtensions_E() / "
               "HPR.refreshExtensions() via Lua state..."
            << std::endl;
  try {
    sol::state_view lua(g_L);
    if (lua["HPR"].valid()) {
      sol::protected_function refreshFn = lua["HPR"]["refreshExtensions"];
      if (!refreshFn.valid()) {
        refreshFn = lua["HPR"]["refreshExtensions_E"];
      }

      if (refreshFn.valid()) {
        sol::protected_function_result res = refreshFn();
        if (!res.valid()) {
          sol::error err = res;
          std::cerr << "[HPR-Store-Library] Lua refreshExtensions error: "
                    << err.what() << std::endl;
        } else {
          std::cout << "[HPR-Store-Library] Lua refreshExtensions completed "
                       "successfully."
                    << std::endl;
        }
      } else {
        std::cerr << "[HPR-Store-Library] refreshExtensions function not found "
                     "in HPR table!"
                  << std::endl;
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "[HPR-Store-Library] Exception during Lua refreshExtensions: "
              << e.what() << std::endl;
  }
}

void unloadExtensionViaLua(const std::string &authorName,
                           const std::string &extensionName) {
  std::lock_guard<std::mutex> lock(g_luaMutex);
  if (!g_L) {
    std::cerr << "[HPR-Store-Library] unloadExtensionViaLua: lua_State pointer "
                 "is null!"
              << std::endl;
    return;
  }

  std::cout << "[HPR-Store-Library] Calling HPR.unloadExtension_E("
               " << authorName << "
               ", "
               " << extensionName << "
               ") synchronously via Lua state..."
            << std::endl;
  try {
    sol::state_view lua(g_L);
    if (lua["HPR"].valid()) {
      sol::protected_function unloadFn = lua["HPR"]["unloadExtension"];
      if (!unloadFn.valid()) {
        unloadFn = lua["HPR"]["unloadExtension_E"];
      }

      if (unloadFn.valid()) {
        sol::protected_function_result res =
            unloadFn(authorName, extensionName);
        if (!res.valid()) {
          sol::error err = res;
          std::cerr << "[HPR-Store-Library] Lua unload error: " << err.what()
                    << std::endl;
        } else {
          std::cout << "[HPR-Store-Library] Synchronous Lua extension unload "
                       "completed successfully."
                    << std::endl;
        }
      } else {
        std::cerr << "[HPR-Store-Library] unloadExtension_E function not found "
                     "in HPR table!"
                  << std::endl;
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "[HPR-Store-Library] Exception during Lua extension unload: "
              << e.what() << std::endl;
  }
}

#ifdef _WIN32

typedef void (*SlintInvokeFn)(void (*)(void *), void *);

static SlintInvokeFn g_hprInvoke = nullptr;

extern "C" HPR_EXPORT void initialize(lua_State *L) {
  g_L = L;
  HMODULE hpr = GetModuleHandleA(nullptr);
  g_hprInvoke = (SlintInvokeFn)GetProcAddress(hpr, "HPR_invokeOnSlintThread");

  if (!g_hprInvoke) {
    return;
  }

  struct InitData {
    std::promise<void> *ready;
  };
  auto data = new InitData{new std::promise<void>()};
  auto fut = data->ready->get_future();

  g_hprInvoke(
      [](void *ud) {
        auto d = static_cast<InitData *>(ud);
        gui = std::make_unique<GUI>();
        d->ready->set_value();
        delete d->ready;
        delete d;
      },
      data);

  fut.wait();
}
#endif

extern "C" HPR_EXPORT void destroy(lua_State *L) {
  g_L = L;
  std::promise<void> done;
  auto fut = done.get_future();

  slint::invoke_from_event_loop([&done] {
    if (!gui) {
      done.set_value();
      return;
    }

    gui->hide();
    gui.reset();
    done.set_value();
  });

  fut.wait();
}

extern "C" HPR_EXPORT void showUi(lua_State *L) {
  g_L = L;
  slint::invoke_from_event_loop([] {
    if (!gui) {
      gui = std::make_unique<GUI>();
    }
    gui->show();
  });
}
