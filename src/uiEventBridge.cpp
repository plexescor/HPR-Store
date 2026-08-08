#include "uiEventBridge.hpp"
#include <slint/slint.h>
#include <iostream>
#include <vector>
#include <memory>
#include <thread>

static StoreItem_S toSlintStoreItem(const StoreItem& item)
{
    StoreItem_S s;
    s.id = slint::SharedString(item.id);
    s.name = slint::SharedString(item.name);
    s.author = slint::SharedString(item.author);
    s.author_github = slint::SharedString(item.authorGithub);
    s.description = slint::SharedString(item.description);
    s.long_description = slint::SharedString(item.longDescription);
    s.version = slint::SharedString(item.version);
    s.download_url = slint::SharedString(item.downloadUrl);
    s.source_url = slint::SharedString(item.sourceUrl);
    s.item_type = slint::SharedString(item.type == StoreItemType::EXTENSION ? "EXTENSION" : "THEME");

    auto tagsModel = std::make_shared<slint::VectorModel<slint::SharedString>>();
    for (const auto& tag : item.tags) 
    {
        tagsModel->push_back(slint::SharedString(tag));
    }
    s.tags = tagsModel;

    auto previewsModel = std::make_shared<slint::VectorModel<slint::SharedString>>();
    for (const auto& img : item.previewImages) 
    {
        previewsModel->push_back(slint::SharedString(img));
    }
    s.preview_images = previewsModel;

    auto versionsModel = std::make_shared<slint::VectorModel<slint::SharedString>>();
    for (const auto& v : item.supportedHPRVersions) 
    {
        versionsModel->push_back(slint::SharedString(v));
    }
    s.supported_hpr_versions = versionsModel;

    s.download_count = item.downloadCount;
    s.star_count = item.starCount;
    s.last_updated = slint::SharedString(item.lastUpdated);

    return s;
}

UIEventBridge::UIEventBridge(slint::ComponentHandle<StoreWindow> uiHandle, std::shared_ptr<RegistryManager> regManager)
    : ui(std::move(uiHandle)), registryManager(std::move(regManager))
{
}

UIEventBridge::~UIEventBridge() = default;

void UIEventBridge::updateStoreItemsUI()
{
    if (!registryManager) return;

    auto itemsModel = std::make_shared<slint::VectorModel<StoreItem_S>>();
    for (const auto& item : registryManager->items) 
    {
        itemsModel->push_back(toSlintStoreItem(item));
    }
    ui->set_storeItems(itemsModel);
}

void UIEventBridge::triggerRefresh()
{
    if (!registryManager) return;

    std::thread([this]() 
    {
        std::cout << "[UIEventBridge] showUi triggered: fetching and parsing registry asynchronously..." << std::endl;
        registryManager->fetchAndParseRegistry();

        slint::invoke_from_event_loop([this]() 
        {
            updateStoreItemsUI();
        });
    }).detach();
}

void UIEventBridge::setupEvents()
{
    if (!registryManager) return;

    ui->on_refresh_clicked([this]() 
    {
        std::cout << "[UIEventBridge] Refresh clicked, fetching and parsing registry asynchronously..." << std::endl;
        triggerRefresh();
    });

    ui->on_install_item_clicked([](slint::SharedString itemId) 
    {
        std::cout << "[UIEventBridge] Install requested for store item: " << itemId.data() << std::endl;
    });
}
