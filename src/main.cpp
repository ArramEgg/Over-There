#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "Settings.hpp"
#include "SettingsMenu.hpp"

using namespace geode::prelude;

namespace overthere
{
	Settings settings;
}


class $modify(OTPauseLayer, PauseLayer) {
	void customSetup() {
		this->PauseLayer::customSetup();
		auto spr = CCSprite::create("OT_logoBtn_001.png"_spr);
		spr->setScale(0.575f);
		auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(OTPauseLayer::onLogoBtn));
		btn->setAnchorPoint({0.5f, 0.5f});
		btn->setID("over-there-button");
		if (auto menu = this->getChildByIDRecursive("right-button-menu")) {
			menu->addChild(btn);
			menu->updateLayout();
		}
	}

	void onEdit(CCObject* sender) {
		PauseLayer::onEdit(sender);
		auto playLayer = GameManager::sharedState()->getPlayLayer();
		if (auto lootNode = playLayer->m_objectParent->getChildByID("over-there-node")) {
			lootNode->removeAllChildren();
			playLayer->m_objectParent->removeChild(lootNode, 1);
		}
	}

	void onLogoBtn(CCObject *) {
		OverTherePopup::create()->show();
	}
};


class $modify(OTPlayLayer, PlayLayer) {
	struct Fields
	{
		Fields() {
			log::info("Created OTEntities");
		}
		std::unordered_map<CCSprite*, GameObject*> OTEntities;
		bool isInPractice;
		~Fields() {
			log::info("Removed OTEntities");
		}
	};


	/*--------------
	Hooked Functions
	--------------*/

	
	bool init(GJGameLevel* level, bool dontCreateObjects) {
		if (!PlayLayer::init(level, dontCreateObjects)) return false;
		m_fields.self();
		m_fields->isInPractice = GJBaseGameLayer::get()->m_isPracticeMode;
		log::info("Fields registered!");
		return true;
	}

	void onQuit() {
		PlayLayer::onQuit();
		cleanOverThere();
		if (auto lootNode = this->m_objectParent->getChildByID("over-there-node")) {
			this->m_objectParent->removeChild(lootNode, 1);
			log::info("Cleaning 3/3: Removed Over There CCNode!");
		}
	} // hopefully no memory incontinence

	void postUpdate(float dt) {
		PlayLayer::postUpdate(dt);
		bool togglePointers = overthere::settings.togglePointers;
		if (!togglePointers) return;
		checkForPracticeEntry();
		if (m_fields->OTEntities.empty()) {
			return;
		}
		updateOT();
	}

	void resetLevel() {
		PlayLayer::resetLevel();
		cleanOverThere();
		bool isPracticeMode = GJBaseGameLayer::get()->m_isPracticeMode;
		//bool isPracticeMode = reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(GJBaseGameLayer::get()) + 0x748); // GJBGL members are still CLAPPED
		if (overthere::settings.practiceOnly && !isPracticeMode) {
			log::info("Practice only enabled! Ignoring loot...");
			return;
		}
		if (!overthere::settings.togglePointers) {
			if (auto lootNode = this->m_objectParent->getChildByID("over-there-node")) {
				this->m_objectParent->removeChild(lootNode, 1);
				log::info("Pointers disabled! Deleting node...");
			}
			return;
		}
		setupOverThere();
	}


	/*--------------
	Custom Functions
	--------------*/


	void checkForPracticeEntry() {
		bool isPracticeMode = GJBaseGameLayer::get()->m_isPracticeMode;
		if (isPracticeMode != m_fields->isInPractice) {
			if (isPracticeMode) {
				m_fields->isInPractice = isPracticeMode;
				setupOverThere();
			}
		}
	} // prevents needing to restart a level when pointers only in practice is on

	void updateOT() {
		for (auto pair = m_fields->OTEntities.begin(); pair != m_fields->OTEntities.end(); ) {
			auto entity = pair->first;
			auto target = pair->second;
			if (target->m_isDisabled) {
				entity->removeAllChildren();
				auto lootNode = this->m_objectParent->getChildByID("over-there-node");
				lootNode->removeChild(entity, 1);
				pair = m_fields->OTEntities.erase(pair);
				continue;
			}
			updateEntityPosition(entity, target);
			++pair;
		}
	}

	void updateEntityPosition(CCSprite* entity, GameObject* target) {
		// wall of stuff
		auto arrow = static_cast<CCSprite*>(entity->getChildByID("arrow"));
		auto cross = static_cast<CCSprite*>(entity->getChildByID("cross"));
		float cameraZoom = GJBaseGameLayer::get()->m_gameState.m_cameraZoom;
		auto winSize = CCDirector::sharedDirector()->getWinSize();
		auto lootPos = target->getLastPosition();
		CCPoint cam = GJBaseGameLayer::get()->m_gameState.m_cameraPosition;
		float cameraFlip = GJBaseGameLayer::get()->m_cameraFlip;
		CCPoint screenPos = ccp((lootPos.x - cam.x) * cameraZoom * cameraFlip, (lootPos.y - cam.y) * cameraZoom);
		CCPoint clampScreenPos;
		float distanceSq;
		float onScreenMargin = -5.f;
		CCPoint distance = ccpSub(cam, lootPos);
		float minDistance = 1000.f;
		float maxDistance = 2000.f;
		float minDistanceSq = minDistance * minDistance;
		float maxDistanceSq = maxDistance * maxDistance;
		bool platformer = GJBaseGameLayer::get()->m_isPlatformer;
		bool dualMode = UILayer::get()->m_dualMode;

		if (cameraFlip < 0) {
			screenPos.x = screenPos.x + winSize.width;
		}

		// if in platformer mode, calculate distance of closest player instead of camera
		if (platformer) {
			CCPoint p1 = GJBaseGameLayer::get()->m_player1->getRealPosition();
			CCPoint p2 = GJBaseGameLayer::get()->m_player2->getRealPosition();
			CCPoint p1Distance = ccpSub(p1, lootPos);
			CCPoint p2Distance = ccpSub(p2, lootPos);
			float p1DistanceSq = p1Distance.x * p1Distance.x + p1Distance.y * p1Distance.y;
			float p2DistanceSq = p2Distance.x * p2Distance.x + p2Distance.y * p2Distance.y;
			if (dualMode) {
				if (p1DistanceSq < p2DistanceSq) {
					distanceSq = p1DistanceSq;
				} // if player 1 is closer, use p1 distance
				else {
					distanceSq = p2DistanceSq;
				} // if player 2 is closer, use p2 distance
			} // if dual mode, find closest player
			else {
				distanceSq = p1DistanceSq;
			} // use p1 distance if not dual mode
		}
		else {
			distanceSq = distance.x * distance.x + distance.y * distance.y;
		} // if not platformer, use camera distance for calculations

		bool onScreen = screenPos.x > onScreenMargin && screenPos.x < winSize.width - onScreenMargin && screenPos.y > onScreenMargin && screenPos.y < winSize.height - onScreenMargin;
		if (onScreen) {
			entity->setOpacity(0);
			arrow->setOpacity(0);
			cross->setOpacity(255);
		} // hide entity if collectible is on screen and show cross
		else {
			float alpha = 255.f;
			if (distanceSq > minDistanceSq) {
				float t = (distanceSq - minDistanceSq) / (maxDistanceSq - minDistanceSq);
				t = std::clamp(t, 0.f, 1.f);
				alpha = (1.f - t) * 255.f;
			} // son, i'm crine
			entity->setOpacity(static_cast<GLubyte>(alpha));
			arrow->setOpacity(static_cast<GLubyte>(alpha));
		} // if collectible is off screen, calculate opacity based on distance

		// clamp entity to screen
		float margin = 40.f;
		clampScreenPos.x = std::clamp(screenPos.x, margin, winSize.width - margin);
		clampScreenPos.y = std::clamp(screenPos.y, margin, winSize.height - margin);
		entity->setPosition(clampScreenPos);
		
		if (!onScreen) {
			updateArrowRotation(entity, target);
			cross->setOpacity(0);
		}
		else {
		cross->setPosition((screenPos-clampScreenPos) * 2);
		}
	}

	void updateArrowRotation(CCSprite* entity, GameObject* target) {
		auto arrow = entity->getChildByID("arrow");
		auto lootPos = target->getLastPosition();
		CCPoint cam = GJBaseGameLayer::get()->m_gameState.m_cameraPosition;
		float cameraZoom = GJBaseGameLayer::get()->m_gameState.m_cameraZoom;
		float cameraFlip = GJBaseGameLayer::get()->m_cameraFlip;
		CCPoint screenPos = ccp((lootPos.x - cam.x) * cameraZoom * cameraFlip, (lootPos.y - cam.y) * cameraZoom);
		auto winSize = CCDirector::sharedDirector()->getWinSize();

		if (cameraFlip < 0) {
			screenPos.x = screenPos.x + winSize.width;
		}

		CCPoint lootDirection = ccpSub(screenPos, entity->getPosition());
		lootDirection.y *= -1; // son im crine
		float angle = ccpToAngle(lootDirection);
		
		arrow->setRotation(CC_RADIANS_TO_DEGREES(angle));
		arrow->setPosition({std::cos(angle) * 40.f, std::sin(-angle) * 40.f});
		return;
	} // make arrow point towards the target collectible

	void cleanOverThere() {
		if (auto lootNode = this->m_objectParent->getChildByID("over-there-node")) {
			lootNode->removeAllChildren();
		}
		m_fields->OTEntities.clear();
	}

	void setupOverThere() {
		if (!this->m_objectParent->getChildByID("over-there-node")) {
			auto mainNode = this->m_objectParent;
			auto lootNode = CCNode::create();
			lootNode->setID("over-there-node");
			mainNode->addChild(lootNode, 51);
			log::info("Setup: Creating Over There CCNode...");
		} // if node doesn't exist, create it
		m_fields->OTEntities = std::unordered_map<CCSprite*, GameObject*>();
		for (int i = 0; i < this->m_collectibles->count(); i++) {
			auto loot = static_cast<GameObject*>(this->m_collectibles->objectAtIndex(i));
			createOverThereEntity(loot);
		} // create entities for all the collectibles in the level
	} // setup mod

	void createOverThereEntity(GameObject* target) {
		if (target->m_isDisabled) return;
		if (m_fields->OTEntities.size() == overthere::settings.pointerLimit) return;
		auto lootNode = this->m_objectParent->getChildByID("over-there-node");
		if (target->m_objectID == 142) {
			auto otEntity = CCSprite::createWithTexture(CCTextureCache::sharedTextureCache()->addImage("goldCoin_001.png"_spr, 0));
			otEntity->setScale(0.6f);
			otEntity->setAnchorPoint({0.5f, 0.5f});
			otEntity->setContentSize({50.f, 50.f});
			otEntity->refreshTextureRect();
			otEntity->setID("over-there-entity");
			lootNode->addChild(otEntity);
			m_fields->OTEntities[otEntity] = target;
			createArrowInstance(otEntity);
			createCrossInstance(otEntity);
		}
		else if (target->m_objectID == 1329) {
			auto otEntity = CCSprite::createWithTexture(CCTextureCache::sharedTextureCache()->addImage("silverCoin_001.png"_spr, 0));
			otEntity->setScale(0.6f);
			otEntity->setAnchorPoint({0.5f, 0.5f});
			otEntity->setContentSize({50.f, 50.f});
			otEntity->refreshTextureRect();
			otEntity->setID("over-there-entity");
			lootNode->addChild(otEntity);
			m_fields->OTEntities[otEntity] = target;
			createArrowInstance(otEntity);
			createCrossInstance(otEntity);
		}
		else {
			auto otEntity = CCSprite::createWithTexture(target->getTexture(), target->getTextureRect());
			otEntity->m_bRectRotated = target->isTextureRectRotated();
			otEntity->setScale(0.6f);
			otEntity->setAnchorPoint({0.5f, 0.5f});
			otEntity->setContentSize({50.f, 50.f});
			otEntity->refreshTextureRect();
			otEntity->setID("over-there-entity");
			lootNode->addChild(otEntity);
			m_fields->OTEntities[otEntity] = target;
			createArrowInstance(otEntity);
			createCrossInstance(otEntity);
		}
	}

	void createArrowInstance(CCSprite* entity) {
		auto arrow = CCSprite::createWithTexture(CCTextureCache::sharedTextureCache()->addImage("arrowRed_001.png"_spr, 0));
		arrow->setScale(0.5f);
		arrow->setAnchorPoint({0.4f, 0.5f}); // rectangle? more like dihhhh
		arrow->setID("arrow");
		entity->addChild(arrow);
		arrow->ignoreAnchorPointForPosition(true);
	}

	void createCrossInstance(CCSprite* entity) {
		auto cross = CCSprite::createWithTexture(CCTextureCache::sharedTextureCache()->addImage("crossRed_001.png"_spr, 0));
		cross->setScale(0.7f);
		cross->setAnchorPoint({0.5f, 0.5f});
		cross->setContentSize({50.f, 50.f});
		cross->refreshTextureRect(); // bruh x2
		cross->setID("cross");
		entity->addChild(cross);
		cross->ignoreAnchorPointForPosition(true);
	}
};