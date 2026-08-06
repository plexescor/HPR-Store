#include "GUI.hpp"
#include "lua.hpp"
#include "sol.hpp"

#ifdef _WIN32
#define HPR_EXPORT __declspec(dllexport)
#else
#define HPR_EXPORT
#endif

static std::unique_ptr<GUI> gui;

extern "C" HPR_EXPORT void initialize(lua_State* L)
{
    slint::invoke_from_event_loop([] 
    {
        gui = std::make_unique<GUI>();
        gui->show();
    });
}

extern "C" HPR_EXPORT void destroy(lua_State* L)
{
    slint::invoke_from_event_loop([] 
    {
        if (!gui)
        {
            std::cout << "gui is null\n";
            return;
        }

        std::cout << "about to hide\n";
        gui->hide();
        std::cout << "hide returned\n";
    });
}