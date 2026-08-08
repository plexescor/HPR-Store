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
}

void GUI::hide()
{
    ui->hide();
}