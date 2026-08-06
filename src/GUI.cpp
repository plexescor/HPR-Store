#include "GUI.hpp"
#include "main.h"
#include <slint/slint.h>

GUI::GUI() : ui(MainWindow::create())
{

}

GUI::~GUI()
{

}

void GUI::run()
{
    ui->run();
}