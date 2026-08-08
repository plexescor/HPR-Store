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

#ifdef _WIN32

//thank you claude for this function pointer typa shit
typedef void(*SlintInvokeFn)(void(*)(void*), void*);

static SlintInvokeFn g_hprInvoke = nullptr;

extern "C" HPR_EXPORT void initialize(lua_State* L)
{
    // get HPR.exe handle and load the function
    HMODULE hpr = GetModuleHandleA(nullptr); // HPR.exe itself
    g_hprInvoke = (SlintInvokeFn)GetProcAddress(hpr, "HPR_invokeOnSlintThread");
    
    if (!g_hprInvoke) {
        return;
    }

    struct InitData { std::promise<void>* ready; };
    auto data = new InitData{ new std::promise<void>() };
    auto fut = data->ready->get_future();

    g_hprInvoke([](void* ud) {
        auto d = static_cast<InitData*>(ud);
        gui = std::make_unique<GUI>();
        d->ready->set_value();
        delete d->ready;
        delete d;
    }, data);

    fut.wait();
}
#endif

extern "C" HPR_EXPORT void destroy(lua_State* L)
{
    std::promise<void> done;
    auto fut = done.get_future();

    slint::invoke_from_event_loop([&done]
    {
        if (!gui)
        {
            done.set_value();
            return;
        }

        gui->hide();
        gui.reset(); 
        done.set_value();
    });

    fut.wait();
}

extern "C" HPR_EXPORT void showUi(lua_State* L)
{
    slint::invoke_from_event_loop([]
    {
        if (!gui) return;
        gui->show();
    });
}
