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
        void updateStoreItemsUI();
        void triggerRefresh();

        void goToPage(int page);
        void goNextPage();
        void goPrevPage();
        void setSortMode(int mode);    // 0=none, 1=stars, 2=downloads
        void updatePaginationUI();

    private:
        slint::ComponentHandle<StoreWindow> ui;
        std::shared_ptr<RegistryManager> registryManager;
        int currentPage = 0;
        int totalPages = 1;
};
