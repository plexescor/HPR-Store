#include "uiEventBridge.hpp"
#include <iostream>

UIEventBridge::UIEventBridge(slint::ComponentHandle<StoreWindow> uiHandle, std::shared_ptr<RegistryManager> regManager)
    : ui(std::move(uiHandle)), registryManager(std::move(regManager))
{
}

UIEventBridge::~UIEventBridge() = default;

void UIEventBridge::setupEvents()
{
    if (!registryManager) return;

    ui->on_refresh_clicked([this]() {
        std::cout << "[UIEventBridge] Refresh clicked, fetching registry..." << std::endl;
        registryManager->fetchRegistry();
    });
}
