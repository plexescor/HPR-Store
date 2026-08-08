#include "GUI.hpp"
#include "main.h"
#include <slint/slint.h>

GUI::GUI() 
    : ui(StoreWindow::create()),
      registryManager(std::make_shared<RegistryManager>()),
      eventBridge(std::make_unique<UIEventBridge>(ui, registryManager))
{
    eventBridge->setupEvents();
}

GUI::~GUI() = default;

void GUI::show()
{
    ui->show();
    if (eventBridge)
    {
        eventBridge->triggerRefresh();
    }
}

void GUI::hide()
{
    ui->hide();
}