#include "uiEventBridge.hpp"
#include "installer.hpp"
#include <slint/slint.h>
#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <sstream>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <ixwebsocket/IXHttpClient.h>

static std::string sanitizeMarkdownForSlint(const std::string& input)
{
    std::string result;
    std::istringstream stream(input);
    std::string line;

    while (std::getline(stream, line))
    {
        size_t firstChar = line.find_first_not_of(" \t");
        if (firstChar != std::string::npos)
        {
            // Strip or convert blockquote prefix '>'
            if (line[firstChar] == '>')
            {
                line.erase(0, firstChar + 1);
                // strip leading spaces after >
                size_t afterBtn = line.find_first_not_of(" \t");
                if (afterBtn != std::string::npos) {
                    line = line.substr(afterBtn);
                } else {
                    line.clear();
                }
                firstChar = line.find_first_not_of(" \t");
            }
        }

        if (firstChar != std::string::npos && line[firstChar] == '#')
        {
            size_t hashEnd = line.find_first_not_of("#", firstChar);
            if (hashEnd != std::string::npos)
            {
                size_t level = hashEnd - firstChar;
                std::string title = line.substr(hashEnd);
                size_t titleStart = title.find_first_not_of(" \t");
                if (titleStart != std::string::npos) title = title.substr(titleStart);

                if (level == 1) {
                    line = "<font color='#38bdf8'>**" + title + "**</font>";
                } else if (level == 2) {
                    line = "<font color='#c084fc'>**" + title + "**</font>";
                } else {
                    line = "<font color='#38bdf8'>**" + title + "**</font>";
                }
            }
        }
        result += line + "\n";
    }

    return result;
}

static StoreItem_S toSlintStoreItem(const StoreItem& item, const std::unordered_map<std::string, std::string>& installedItems)
{
    StoreItem_S s;
    s.id = slint::SharedString(item.id);
    s.name = slint::SharedString(item.name);
    s.author = slint::SharedString(item.author);
    s.author_github = slint::SharedString(item.authorGithub);
    s.description = slint::SharedString(item.description);
    s.long_description = slint::private_api::parse_markdown(slint::SharedString(sanitizeMarkdownForSlint(item.longDescription)), {});
    s.version = slint::SharedString(item.version);
    s.download_url = slint::SharedString(item.downloadUrl);
    s.source_url = slint::SharedString(item.sourceUrl);
    s.item_type = slint::SharedString(item.type == StoreItemType::EXTENSION ? "EXTENSION" : "THEME");

    auto tagsModel = std::make_shared<slint::VectorModel<slint::SharedString>>();
    for (const auto& tag : item.tags)
        tagsModel->push_back(slint::SharedString(tag));
    s.tags = tagsModel;

    auto previewsModel = std::make_shared<slint::VectorModel<slint::SharedString>>();
    for (const auto& img : item.previewImages)
        previewsModel->push_back(slint::SharedString(img));
    s.preview_images = previewsModel;

    auto versionsModel = std::make_shared<slint::VectorModel<slint::SharedString>>();
    for (const auto& v : item.supportedHPRVersions)
        versionsModel->push_back(slint::SharedString(v));
    s.supported_hpr_versions = versionsModel;

    s.download_count = item.downloadCount;
    s.star_count = item.starCount;
    s.last_updated = slint::SharedString(item.lastUpdated);
    s.installed = (installedItems.find(item.id) != installedItems.end());

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

    auto installedItems = Installer::loadInstalledItems();

    auto pageItems = registryManager->getPage(currentPage);
    auto itemsModel = std::make_shared<slint::VectorModel<StoreItem_S>>();
    for (const auto& item : pageItems)
        itemsModel->push_back(toSlintStoreItem(item, installedItems));

    ui->set_storeItems(itemsModel);
    updatePaginationUI();
}

void UIEventBridge::updatePaginationUI()
{
    int total = registryManager->totalItemCount;
    totalPages = (total == 0) ? 1 : ((total + RegistryManager::PAGE_SIZE - 1) / RegistryManager::PAGE_SIZE);

    ui->set_totalItems(total);
    ui->set_currentPage(currentPage);
    ui->set_totalPages(totalPages);
    ui->set_currentSort(static_cast<int>(registryManager->currentSort));
    ui->set_activeTypeFilter(registryManager->activeTypeFilter);
}

void UIEventBridge::goToPage(int page)
{
    if (page < 0) page = 0;
    if (page >= totalPages) page = totalPages - 1;
    currentPage = page;
    updateStoreItemsUI();
}

void UIEventBridge::goNextPage()
{
    goToPage(currentPage + 1);
}

void UIEventBridge::goPrevPage()
{
    goToPage(currentPage - 1);
}

void UIEventBridge::setSortMode(int mode)
{
    if (!registryManager) return;

    std::thread([this, mode]()
    {
        if (mode == 0)
        {
            std::cout << "[UIEventBridge] Sort reset — reloading local database." << std::endl;
            registryManager->currentSort = SortMode::NONE;
            registryManager->readLocalRegistry();
        }
        else
        {
            // Sort mode: sort currently parsed in-memory allItems
            SortMode sortMode = (mode == 1) ? SortMode::STARS_DESC : SortMode::DOWNLOADS_DESC;
            registryManager->sortItems(sortMode);
        }

        currentPage = 0;

        slint::invoke_from_event_loop([this]()
        {
            updateStoreItemsUI();
        });
    }).detach();
}

void UIEventBridge::triggerRefresh()
{
    if (!registryManager) return;

    std::thread([this]()
    {
        std::cout << "[UIEventBridge] showUi triggered: reading local database..." << std::endl;
        registryManager->readLocalRegistry();
        currentPage = 0;

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
        std::cout << "[UIEventBridge] Update Database clicked. Fetching remote updates..." << std::endl;
        ui->set_databaseUpdating(true);
        std::thread([this]()
        {
            registryManager->updateDatabase();
            currentPage = 0;
            slint::invoke_from_event_loop([this]()
            {
                updateStoreItemsUI();
                ui->set_databaseUpdating(false);
            });
        }).detach();
    });

    ui->on_next_page_clicked([this]()
    {
        goNextPage();
    });

    ui->on_prev_page_clicked([this]()
    {
        goPrevPage();
    });

    ui->on_go_to_page_clicked([this](slint::SharedString pageStr)
    {
        // Parse 1-indexed string from user, convert to 0-indexed page
        try
        {
            std::string s = std::string(pageStr);
            // Strip any non-digit characters
            s.erase(std::remove_if(s.begin(), s.end(), [](char c){ return !std::isdigit(c); }), s.end());

            if (s.empty()) return;

            int page1based = std::stoi(s);
            int page0based = page1based - 1;  // Convert to 0-indexed
            goToPage(page0based);
        }
        catch (...)
        {
            std::cerr << "[UIEventBridge] Invalid page number: " << std::string(pageStr) << std::endl;
        }
    });

    ui->on_sort_changed([this](int mode)
    {
        setSortMode(mode);
    });

    ui->on_type_filter_changed([this](int filter)
    {
        if (!registryManager) return;
        std::thread([this, filter]()
        {
            std::cout << "[UIEventBridge] Type filter changed: " << filter << std::endl;
            registryManager->activeTypeFilter = filter;
            registryManager->rebuildPageOrder();
            currentPage = 0;

            slint::invoke_from_event_loop([this, filter]()
            {
                ui->set_activeTypeFilter(filter);
                updateStoreItemsUI();
            });
        }).detach();
    });

    ui->on_installed_filter_changed([this](int filter)
    {
        if (!registryManager) return;
        std::thread([this, filter]()
        {
            std::cout << "[UIEventBridge] Installed status filter changed: " << filter << std::endl;
            registryManager->activeInstalledFilter = filter;
            registryManager->rebuildPageOrder();
            currentPage = 0;

            slint::invoke_from_event_loop([this, filter]()
            {
                ui->set_activeInstalledFilter(filter);
                updateStoreItemsUI();
            });
        }).detach();
    });

    ui->on_item_selected([this](slint::SharedString itemIdStr)
    {
        if (!registryManager) return;

        std::string itemId = std::string(itemIdStr);

        // Find the item first to get its type and previewUrls
        StoreItem selectedItem;
        bool found = false;
        for (const auto& item : registryManager->allItems)
        {
            if (item.id == itemId)
            {
                selectedItem = item;
                found = true;
                break;
            }
        }

        // If not found in allItems, check items
        if (!found)
        {
            for (const auto& item : registryManager->items)
            {
                if (item.id == itemId)
                {
                    selectedItem = item;
                    found = true;
                    break;
                }
            }
        }

        if (!found || selectedItem.type != StoreItemType::THEME || selectedItem.previewImages.empty())
        {
            ui->set_selectedPreviewImages(std::make_shared<slint::VectorModel<slint::Image>>());
            ui->set_selectedPreviewImageCount(0);
            ui->set_previewLoading(false);
            return;
        }

        ui->set_previewLoading(true);

        std::thread([this, selectedItem]()
        {
            auto previewUrls = selectedItem.previewImages;
            auto tempDir = std::filesystem::temp_directory_path() / "hpr-store-previews" / selectedItem.id;
            std::filesystem::create_directories(tempDir);

            std::vector<std::string> localPaths(previewUrls.size());
            std::vector<std::thread> downloadThreads;
            downloadThreads.reserve(previewUrls.size());

            for (size_t i = 0; i < previewUrls.size(); ++i)
            {
                downloadThreads.emplace_back([i, url = previewUrls[i], tempDir, &localPaths]()
                {
                    // Compute filename from URL hash or name
                    size_t lastSlash = url.find_last_of('/');
                    std::string filename = (lastSlash == std::string::npos) ? std::to_string(i) : url.substr(lastSlash + 1);
                    auto destPath = tempDir / filename;

                    ix::HttpClient httpClient;
                    auto args = httpClient.createRequest();
                    args->extraHeaders["User-Agent"] = "HPR";

                    auto response = httpClient.get(url, args);
                    if (response->statusCode == 200 && !response->body.empty())
                    {
                        std::ofstream out(destPath, std::ios::binary | std::ios::trunc);
                        if (out.is_open())
                        {
                            out << response->body;
                            out.close();
                            localPaths[i] = destPath.string();
                            std::cout << "[UIEventBridge] Previews: Downloaded " << url << " -> " << destPath << std::endl;
                            return;
                        }
                    }
                    std::cerr << "[UIEventBridge] Previews: Failed downloading " << url << std::endl;
                });
            }

            for (auto& t : downloadThreads) t.join();

            // Pushing results back to UI
            slint::invoke_from_event_loop([this, localPaths]()
            {
                auto imageModel = std::make_shared<slint::VectorModel<slint::Image>>();
                for (const auto& path : localPaths)
                {
                    if (!path.empty())
                    {
                        auto maybeImg = slint::Image::load_from_path(slint::SharedString(path));
                        imageModel->push_back(maybeImg);
                    }
                }

                ui->set_selectedPreviewImages(imageModel);
                ui->set_selectedPreviewImageCount(static_cast<int>(imageModel->row_count()));
                ui->set_previewLoading(false);
            });
        }).detach();
    });

    ui->on_install_item_clicked([this](slint::SharedString itemId)
    {
        if (!registryManager) return;

        auto maybeItem = registryManager->getItemById(std::string(itemId));
        if (!maybeItem)
        {
            std::cerr << "[UIEventBridge] Install: item not found: " << itemId.data() << std::endl;
            return;
        }

        std::cout << "[UIEventBridge] Install requested for: " << itemId.data() << std::endl;
        ui->set_installing(true);
        ui->set_isUninstall(false);
        ui->set_installProgressText("STARTING INSTALLATION...");
        ui->set_installResultVisible(false);

        std::thread([this, item = *maybeItem]()
        {
            InstallResult result = Installer::install(item, [this](std::string progress) {
                slint::invoke_from_event_loop([this, progress]() {
                    ui->set_installProgressText(slint::SharedString(progress));
                });
            });

            slint::invoke_from_event_loop([this, result, item]()
            {
                ui->set_installing(false);
                ui->set_installSuccess(result.success);
                ui->set_installErrorMessage(slint::SharedString(result.errorMessage));
                ui->set_installResultVisible(true);
                
                // Instantly update the list items to show "INSTALLED" if successful
                if (result.success)
                {
                    updateStoreItemsUI();
                    
                    // Also update the currently open detail view selectedItem copy's installed state
                    auto currentSelected = ui->get_selectedItem();
                    if (std::string(currentSelected.id) == item.id)
                    {
                        currentSelected.installed = true;
                        ui->set_selectedItem(currentSelected);
                    }
                }
            });
        }).detach();
    });

    ui->on_uninstall_item_clicked([this](slint::SharedString itemId)
    {
        if (!registryManager) return;

        auto maybeItem = registryManager->getItemById(std::string(itemId));
        if (!maybeItem)
        {
            std::cerr << "[UIEventBridge] Uninstall: item not found: " << itemId.data() << std::endl;
            return;
        }

        std::cout << "[UIEventBridge] Uninstall requested for: " << itemId.data() << std::endl;
        ui->set_installing(true);
        ui->set_isUninstall(true);
        ui->set_installProgressText("UNINSTALLING...");
        ui->set_installResultVisible(false);

        std::thread([this, item = *maybeItem]()
        {
            bool success = Installer::uninstall(item.id, item.type);

            slint::invoke_from_event_loop([this, success, item]()
            {
                ui->set_installing(false);
                ui->set_installSuccess(success);
                if (success) {
                    ui->set_installErrorMessage(slint::SharedString("Item uninstalled successfully."));
                } else {
                    ui->set_installErrorMessage(slint::SharedString("Failed to uninstall the item."));
                }
                ui->set_installResultVisible(true);
                
                // Instantly update UI list
                updateStoreItemsUI();

                // If currently open detail view is the uninstalled item, update its state
                auto currentSelected = ui->get_selectedItem();
                if (std::string(currentSelected.id) == item.id)
                {
                    currentSelected.installed = false;
                    ui->set_selectedItem(currentSelected);
                }
            });
        }).detach();
    });
}
