#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Label.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/SliderNode.hpp>
#include <Geode/binding/CCMenuItemToggler.hpp>
#include "Settings.hpp"

using namespace geode::prelude;

/*
"practice-only": {
    "type": "bool",
    "name": "Toggle Pointers Only in Practice Mode",
    "default": false
},
"toggle-pointers": {
    "type": "bool",
    "name": "Toggle Pointers",
    "default": true
},
"pointer-limit": {
    "type": "int",
    "name": "Maximum Pointer Count",
    "default": 24,
    "description": "Some spam levels use pickup objects in an attempt to lag the game (and it works). This limit will help prevent excessive entity creation. Some levels have quite a few keys, so I've made the maximum limit fairly high, while still allowing for a custom threshold. Hopefully this will be useful for those with weaker devices.",
    "min": 6,
    "max": 48
}
old geode settings. might use descriptions later
*/


class OverTherePopup : public Popup {
protected:
    CCMenu* m_popupMenu;
    CCMenuItemToggler* m_togglerPointers;
    CCMenuItemToggler* m_togglerPractice;
    SliderNode* m_pointerLimitSlider;
    geode::Label* m_sliderLabel;
    geode::Label* m_statusLabel;

	bool init() {
		if (!Popup::init(230.f, 190.f, getThemeSpriteName())) return false;
		this->setTitle("Over There Settings"); 
		this->setID("over-there-popup");

		m_popupMenu = CCMenu::create();
		m_popupMenu->setPosition({0.f, 0.f});
		m_popupMenu->setID("over-there-menu");
		m_popupMenu->setContentSize({230.f, 190.f});
		m_mainLayer->addChild(m_popupMenu, 1);
		
		auto bgGlobal = CCScale9Sprite::create("square02b_001.png", {1, 1, 78, 78});
		bgGlobal->setPosition({115.f, 82.f});
		bgGlobal->setColor(getThemePrimaryColor());
		bgGlobal->setContentSize({200.f, 134.f});
		bgGlobal->setID("bg-nine-slice");
		m_mainLayer->addChild(bgGlobal);

        m_statusLabel = geode::Label::createRich("<cx ff0000>Mod disabled!", "bigFont.fnt");
        m_statusLabel->setPosition({115.f, 11.f});
		m_statusLabel->setScale(0.35f);
        m_statusLabel->setID("status-label");
        m_statusLabel->setAlignment(geode::Label::Alignment::Center);
        m_statusLabel->setOpacity(0);
        m_popupMenu->addChild(m_statusLabel, 2);
        if (overthere::settings.togglePointers == false) {
            m_statusLabel->setOpacity(255);
        }

        auto toggleLabel = geode::Label::create("Toggle\nPointers", "bigFont.fnt");
        toggleLabel->setPosition({75.f, 85.f});
		toggleLabel->setScale(0.3f);
		toggleLabel->setID("toggle-pointers-label");
        toggleLabel->setAlignment(geode::Label::Alignment::Center);
		m_popupMenu->addChild(toggleLabel);

		auto offSpr1 = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
		auto onSpr1 = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
		m_togglerPointers = CCMenuItemToggler::create(offSpr1, onSpr1, this, menu_selector(OverTherePopup::togglerPointersCallback));
		m_togglerPointers->setAnchorPoint({0.5f, 0.5f});
		m_togglerPointers->setPosition({75.f, 120.f});
		m_togglerPointers->setID("over-there-toggler-pointers");
		m_popupMenu->addChild(m_togglerPointers);
        bool togglePointers = overthere::settings.togglePointers;
		m_togglerPointers->toggle(togglePointers ? 1 : 0);

        auto practiceLabel = geode::Label::create("Pointers\nOnly In\nPractice", "bigFont.fnt");
        practiceLabel->setPosition({155.f, 80.f});
		practiceLabel->setScale(0.3f);
		practiceLabel->setID("practice-only-label");
        practiceLabel->setAlignment(geode::Label::Alignment::Center);
        m_popupMenu->addChild(practiceLabel);

		auto offSpr2 = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
		auto onSpr2 = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
		m_togglerPractice = CCMenuItemToggler::create(offSpr2, onSpr2, this, menu_selector(OverTherePopup::togglerPracticeCallback));
		m_togglerPractice->setAnchorPoint({0.5f, 0.5f});
		m_togglerPractice->setPosition({155.f, 120.f});
		m_togglerPractice->setID("over-there-toggler-practice");
		m_popupMenu->addChild(m_togglerPractice);
		bool practiceOnly = overthere::settings.practiceOnly;
		m_togglerPractice->toggle(practiceOnly ? 1 : 0);
		
		auto m_limitLabel = geode::Label::create("Pointer Limit", "goldFont.fnt");
		m_limitLabel->setPosition({90.f, 50.f});
		m_limitLabel->setScale(0.5f);
		m_limitLabel->setID("limit-label");
		m_limitLabel->setAlignment(geode::Label::Alignment::Center);
		m_popupMenu->addChild(m_limitLabel);

        m_sliderLabel = geode::Label::create(numToString(overthere::settings.pointerLimit, 1), "bigFont.fnt");
        m_sliderLabel->setPosition({165.f, 50.f});
        m_sliderLabel->setScale(0.45f);
        m_sliderLabel->setID("slider-label");
        m_sliderLabel->setAlignment(geode::Label::Alignment::Center);
        m_popupMenu->addChild(m_sliderLabel);

        m_pointerLimitSlider = SliderNode::create([this](SliderNode* slider, float value) {
            this->pointerLimitSliderCallback(value);
        }, 0);
        m_pointerLimitSlider->setMin(6.f);
        m_pointerLimitSlider->setMax(48.f);
        m_pointerLimitSlider->setValue((float)overthere::settings.pointerLimit);
        m_pointerLimitSlider->setPosition({115.f, 30.f});
        m_pointerLimitSlider->setID("pointer-limit-slider");
        m_pointerLimitSlider->setScale(0.8f);
        m_pointerLimitSlider->setSnapStep(1.f);
        m_popupMenu->addChild(m_pointerLimitSlider);
		
		return true;
	} // init


	/*----------------
    Callback Functions
    ----------------*/


    void pointerLimitSliderCallback(float value) {
        Mod::get()->setSavedValue<int>("pointer-limit", (int)value);
        m_sliderLabel->setText(numToString(value, 1).c_str());
    }

	void togglerPointersCallback(CCObject* sender) {
		auto toggle = static_cast<CCMenuItemToggler*>(sender);
		bool state = !toggle->isToggled();
		Mod::get()->setSavedValue<bool>("toggle-pointers", state);
		geode::log::info("Saved toggle state {}", state);
		if (state == false) {
			m_statusLabel->setOpacity(255);
		}
		else {
			m_statusLabel->setOpacity(0);
		}
		return;
	}

	void togglerPracticeCallback(CCObject* sender) {
		auto toggle = static_cast<CCMenuItemToggler*>(sender);
		bool state = !toggle->isToggled();
		Mod::get()->setSavedValue<bool>("practice-only", state);
		geode::log::info("Saved toggle state {}", state);
		return;
	}

	char const* getThemeSpriteName() {
		auto theme = Mod::get()->getSettingValue<std::string>("mod-theme");
		if (theme == "Brown") return "GJ_square01.png";
		else if (theme == "Blue") return "GJ_square02.png";
		else if (theme == "Green") return "GJ_square03.png";
		else if (theme == "Purple") return "GJ_square04.png";
		else if (theme == "Gray") return "GJ_square05.png";
		else if (theme == "White") return "GJ_square06.png";
		else return "GJ_square01.png";
	}

	ccColor3B getThemePrimaryColor() {
		auto theme = Mod::get()->getSettingValue<std::string>("mod-theme");
		if (theme == "Brown") return {111, 60, 24};
		else if (theme == "Blue") return {42, 52, 108};
		else if (theme == "Green") return {56, 115, 47};
		else if (theme == "Purple") return {58, 25, 96};
		else if (theme == "Gray") return {56, 56, 56};
		else if (theme == "White") return {152, 152, 152};
		else return {111, 60, 24};
	}

    void updateSettings() {
        overthere::settings.togglePointers = Mod::get()->getSavedValue<bool>("toggle-pointers");
        overthere::settings.practiceOnly = Mod::get()->getSavedValue<bool>("practice-only");
        overthere::settings.pointerLimit = Mod::get()->getSavedValue<int>("pointer-limit");
		geode::log::info("Updated settings!");
    }

	void onClose(CCObject* sender) {
		Popup::onClose(sender);
		updateSettings();
	}
public:
	static OverTherePopup* create() {
		auto ret = new OverTherePopup();
		if (ret && ret->init()) {
			ret->autorelease();
			return ret;
		}
		delete ret;
		return nullptr;
	} // create
};