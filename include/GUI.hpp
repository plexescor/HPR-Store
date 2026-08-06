#pragma once
#include "main.h"

class GUI
{
    public:
        GUI();
        ~GUI();

    public:
        void show();
        void hide();

    private:
        slint::ComponentHandle<StoreWindow> ui;
};