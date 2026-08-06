#pragma once
#include "main.h"

class GUI
{
    public:
        GUI();
        ~GUI();

    public:
        void run();

    private:
        slint::ComponentHandle<MainWindow> ui;
};