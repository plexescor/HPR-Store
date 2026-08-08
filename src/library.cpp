#include "GUI.hpp"
#include "lua.hpp"
#include "sol.hpp"
#include <future>
#include <iostream>
#include <memory>

#ifdef _WIN32
#define HPR_EXPORT __declspec(dllexport)
#else
#define HPR_EXPORT
#endif

static std::unique_ptr<GUI> gui;

extern "C" HPR_EXPORT void initialize(lua_State* L)
{
    std::promise<void> ready;
    auto fut = ready.get_future();

    slint::invoke_from_event_loop([&ready]
    {
        gui = std::make_unique<GUI>();
        ready.set_value();
    });

    fut.wait();
}

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