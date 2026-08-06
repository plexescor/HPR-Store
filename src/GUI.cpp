#include "GUI.hpp"
#include "main.h"
#include <slint/slint.h>

GUI::GUI() : ui(StoreWindow::create())
{

}

GUI::~GUI()
{

}

void GUI::show()
{
    ui->show();
    std::cout << "TSHOW: hread: " << std::this_thread::get_id() << '\n';
}

void GUI::hide()
{
    ui->hide();
    std::cout << "HIDE: Thread: " << std::this_thread::get_id() << '\n';
}