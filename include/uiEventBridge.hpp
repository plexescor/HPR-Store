#pragma once
#include "main.h"
#include "registryManager.hpp"
#include <memory>

class UIEventBridge
{
    public:
        UIEventBridge(slint::ComponentHandle<StoreWindow> uiHandle, std::shared_ptr<RegistryManager> regManager);
        ~UIEventBridge();

        void setupEvents();

    private:
        slint::ComponentHandle<StoreWindow> ui;
        std::shared_ptr<RegistryManager> registryManager;
};
