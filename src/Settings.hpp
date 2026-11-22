#pragma once

using namespace geode::prelude;

namespace overthere
{
    struct Settings
    {
        bool togglePointers = Mod::get()->getSavedValue<bool>("toggle-pointers", true);
        bool practiceOnly = Mod::get()->getSavedValue<bool>("practice-only", false);
        int pointerLimit = Mod::get()->getSavedValue<int>("pointer-limit", 24);
    };
    extern Settings settings;
}