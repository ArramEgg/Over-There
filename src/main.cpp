#include <Geode/Geode.hpp>
#include <Geode/binding/CCMenuItemToggler.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(OTPauseLayer, PauseLayer) {
	void customSetup() {
		this->PauseLayer::customSetup();

		// create item node (this is for the label)
		auto otItem = CCNode::create();
		otItem->setContentSize({50.f, 26.f});

		// create toggle box
		auto offSpr = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
		auto onSpr = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
		auto otToggle = CCMenuItemToggler::create(offSpr, onSpr, this, menu_selector(OTPauseLayer::toggleOT));
		otToggle->setAnchorPoint({0.5f, 0.5f});
		otToggle->setID("over-there-toggle");

		// apply setting value to toggle box
		bool togglePointers = Mod::get()->getSettingValue<bool>("toggle-pointers");
		otToggle->toggle(togglePointers ? 1 : 0);
		
		// create toggle box label
		auto otLabel = CCLabelBMFont::create("Toggle\nPointers", "bigFont.fnt", 0.f, kCCTextAlignmentCenter);
		otLabel->setPosition({25.f, 13.f});
		otLabel->setScale(0.3f);
		otLabel->setID("over-there-label");

		// add label to its node (if i don't do this, weird stuff happens idk... menu don't like labels ig)
		otItem->addChild(otLabel);
		
		if (auto menu = this->getChildByID("right-button-menu")) {
			menu->addChild(otToggle);
			menu->addChild(otItem);
			menu->updateLayout();
		} // obligatory comment
	} // hook customSetup to create stuff
	// callback function
	void toggleOT(CCObject* sender) {
		auto toggle = static_cast<CCMenuItemToggler*>(sender);
		bool state = !toggle->isToggled();
		Mod::get()->setSettingValue<bool>("toggle-pointers", state);
		return;
	} // make toggle box do stuff
}; // UI creation is butt-ugly


class $modify(OTPlayLayer, PlayLayer) {
	struct Fields
	{
		std::unordered_map<CCSprite*, GameObject*> OTEntities;
	};


	/*--------------
	Hooked Functions
	--------------*/


	void onQuit() {
		PlayLayer::onQuit();
		cleanOverThere();
		if (auto lootNode = this->m_objectParent->getChildByID("over-there-node")) {
			this->m_objectParent->removeChild(lootNode, 1);
		}
	} // hopefully no memory incontinence


	void postUpdate(float dt) {
		PlayLayer::postUpdate(dt);
		bool togglePointers = Mod::get()->getSettingValue<bool>("toggle-pointers");
		if (!togglePointers) return;
		if (m_fields->OTEntities.empty()) {
			return;
		}
		updateOT();
	} // update stuff per frame here


	void resetLevel() {
		PlayLayer::resetLevel();
		cleanOverThere();
		if (!Mod::get()->getSettingValue<bool>("toggle-pointers")) {
			if (auto lootNode = this->m_objectParent->getChildByID("over-there-node")) {
				this->m_objectParent->removeChild(lootNode, 1);
			}
			return;
		}
		setupOverThere();
	} // yeah (prettiest function in the whole mod)


	/*--------------
	Custom Functions
	--------------*/


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
	} // the "main" update function


	void updateEntityPosition(CCSprite* entity, GameObject* target) {
		// wall of stuff
		auto arrow = static_cast<CCSprite*>(entity->getChildByID("arrow"));
		auto cross = static_cast<CCSprite*>(entity->getChildByID("cross"));
		float cameraZoom = GJBaseGameLayer::get()->m_gameState.m_cameraZoom;
		CCPoint cameraOffset = GJBaseGameLayer::get()->m_gameState.m_cameraOffset;
		auto winSize = CCDirector::sharedDirector()->getWinSize();
		auto lootPos = target->getLastPosition();
		CCPoint cam = GJBaseGameLayer::get()->m_gameState.m_cameraPosition;
		float cameraFlip = GJBaseGameLayer::get()->m_cameraFlip;
		CCPoint screenPos = ccp((lootPos.x - cam.x) * cameraZoom * cameraFlip + cameraOffset.x, (lootPos.y - cam.y) * cameraZoom + cameraOffset.y);
		float distanceSq;
		float onScreenMargin = 20.f;
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

		// opacity calculation
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
		screenPos.x = std::clamp(screenPos.x, margin, winSize.width - margin);
		screenPos.y = std::clamp(screenPos.y, margin, winSize.height - margin);
		entity->setPosition(screenPos);

		if (!onScreen) {
			updateArrowRotation(entity, target);
			cross->setOpacity(0);
		} // update arrows if target is offscreen and hide cross
	} // place entity on screen


	void updateArrowRotation(CCSprite* entity, GameObject* target) {
		auto arrow = entity->getChildByID("arrow");
		auto lootPos = target->getLastPosition();
		CCPoint cam = GJBaseGameLayer::get()->m_gameState.m_cameraPosition;
		float cameraZoom = GJBaseGameLayer::get()->m_gameState.m_cameraZoom;
		CCPoint cameraOffset = GJBaseGameLayer::get()->m_gameState.m_cameraOffset;
		float cameraFlip = GJBaseGameLayer::get()->m_cameraFlip;
		CCPoint screenPos = ccp((lootPos.x - cam.x) * cameraZoom * cameraFlip + cameraOffset.x, (lootPos.y - cam.y) * cameraZoom + cameraOffset.y);
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
	} // second prettiest function in the whole mod


	void setupOverThere() {
		if (!Mod::get()->getSettingValue<bool>("toggle-pointers")) return;
		if (!this->m_objectParent->getChildByID("over-there-node")) {
			auto mainNode = this->m_objectParent;
			auto lootNode = CCNode::create();
			lootNode->setID("over-there-node");
			mainNode->addChild(lootNode, 51);
		} // if node doesn't exist, create it
		m_fields->OTEntities = std::unordered_map<CCSprite*, GameObject*>();
		for (int i = 0; i < this->m_collectibles->count(); i++) {
			auto loot = static_cast<GameObject*>(this->m_collectibles->objectAtIndex(i));
			createOverThereEntity(loot);
		} // create entities for all the collectibles in the level
	} // setup mod


	void createOverThereEntity(GameObject* target) {
		if (target->m_isDisabled) return;
		if (m_fields->OTEntities.size() == Mod::get()->getSettingValue<int>("pointer-limit")) return;
		auto lootNode = this->m_objectParent->getChildByID("over-there-node");
		auto otEntity = CCSprite::createWithTexture(target->getTexture(), target->getTextureRect());
		otEntity->m_bRectRotated = target->isTextureRectRotated();
		otEntity->setScale(0.6f);
		otEntity->setAnchorPoint({0.5f, 0.5f});
		otEntity->setContentSize({50.f, 50.f});
		otEntity->refreshTextureRect(); // bruh
		otEntity->setID("over-there-entity");
		lootNode->addChild(otEntity);
		m_fields->OTEntities[otEntity] = target;
		createArrowInstance(otEntity);
		createCrossInstance(otEntity);
	} // create entity for the targeted collectible and add it to the map


	void createArrowInstance(CCSprite* entity) {
		auto arrow = CCSprite::createWithTexture(CCTextureCache::sharedTextureCache()->addImage("arrowRed_001.png"_spr, 0));
		arrow->setScale(0.5f);
		arrow->setAnchorPoint({0.4f, 0.5f}); // rectangle? more like dihhhh
		arrow->setID("arrow");
		entity->addChild(arrow);
		arrow->ignoreAnchorPointForPosition(true);
	} // create arrow child for entity to point with

	void createCrossInstance(CCSprite* entity) {
		auto cross = CCSprite::createWithTexture(CCTextureCache::sharedTextureCache()->addImage("crossRed_001.png"_spr, 0));
		cross->setScale(0.7f);
		cross->setAnchorPoint({0.5f, 0.5f});
		cross->setContentSize({50.f, 50.f});
		cross->refreshTextureRect(); // bruh x2
		cross->setID("cross");
		entity->addChild(cross);
		cross->ignoreAnchorPointForPosition(true);
	} // create X for marking loot position
};