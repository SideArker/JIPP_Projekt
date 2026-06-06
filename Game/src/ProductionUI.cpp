#include "ProductionUI.hpp"
#include "AnimationManager.hpp"
#include "Unit.hpp"
#include "UnitRegistry.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <map>
#include <string>

struct Category {
  std::string name;
  std::vector<std::string> units;
  bool unlocked = false;
};

static const sf::Color BG_DARK(15, 25, 35, 210);
static const sf::Color BG_CARD(8, 16, 28);
static const sf::Color BG_CAT(20, 32, 48);
static const sf::Color BORDER_COL(60, 85, 115);
static const sf::Color TEXT_NORMAL(200, 220, 255);
static const sf::Color TEXT_GREY(120, 135, 155);
static const sf::Color BTN_BG(30, 45, 60);
static const sf::Color BTN_HOVER(50, 75, 100);
static const sf::Color BTN_DOWN(20, 30, 40);
static const sf::Color BTN_DISABLED(20, 25, 30);
static const sf::Color BTN_TEXT(200, 220, 255);
static const sf::Color BTN_TEXT_DIS(100, 110, 130);

tgui::Button::Ptr ProductionUI::makeStyledBtn(const std::string &text) {
  auto btn = tgui::Button::create(text);
  btn->getRenderer()->setBackgroundColor(BTN_BG);
  btn->getRenderer()->setBackgroundColorHover(BTN_HOVER);
  btn->getRenderer()->setBackgroundColorDown(BTN_DOWN);
  btn->getRenderer()->setBackgroundColorDisabled(BTN_DISABLED);
  btn->getRenderer()->setTextColor(BTN_TEXT);
  btn->getRenderer()->setTextColorDisabled(BTN_TEXT_DIS);
  btn->getRenderer()->setBorders(tgui::Borders(1));
  btn->getRenderer()->setBorderColor(BORDER_COL);
  return btn;
}

ProductionUI::ProductionUI(tgui::Gui &gui, MapManager &mapManager,
                           TurnController &turnController)
    : m_gui(gui), m_mapManager(mapManager), m_turnController(turnController) {}

void ProductionUI::close() {
  if (m_panel) {
    m_gui.remove(m_panel);
    m_panel = nullptr;
  }
  m_previews.clear();
}

void ProductionUI::update(float dt) {
  const std::vector<MoveDirection> dirs = {
      MoveDirection::Left, MoveDirection::Down, MoveDirection::Right,
      MoveDirection::Up};

  for (auto &preview : m_previews) {
    if (!preview.unit || !preview.canvas)
      continue;

    preview.timer -= dt;
    if (preview.timer <= 0.f) {
      preview.timer = 1.f;
      preview.dirIndex = (preview.dirIndex + 1) % static_cast<int>(dirs.size());
      preview.unit->setDirection(dirs[preview.dirIndex]);
    }

    // Redraw to the canvas
    preview.canvas->clear(sf::Color::Transparent);

    sf::Sprite sprite(preview.unit->getCurrentTexture());
    
    sf::IntRect rect = preview.unit->getCurrentRect();
    bool flipX = preview.unit->shouldFlipX();
    
    const AnimationSet *animSet = AnimationManager::getSet(preview.unit->getName());
    if (animSet) {
      std::string dirStr = "";
      switch (dirs[preview.dirIndex]) {
        case MoveDirection::Left:  dirStr = "_left"; break;
        case MoveDirection::Right: dirStr = "_right"; break;
        case MoveDirection::Down:  dirStr = "_down"; break;
        case MoveDirection::Up:    dirStr = "_up"; break;
      }
      
      const AnimationClip* clip = animSet->getClip("idle" + dirStr);
      if (!clip) clip = animSet->getClip("idle");
      if (!clip) clip = animSet->getClip("walk" + dirStr);
      if (!clip) clip = animSet->getClip("walk");
      
      if (clip && !clip->frames.empty()) {
        rect = clip->frames[0];
        flipX = clip->flipX;
      }
    }

    sprite.setTextureRect(rect);

    sf::Vector2f centerOffset((64.f - rect.size.x * 2.f) / 2.f,
                              (64.f - rect.size.y * 2.f) / 2.f);
    sprite.setPosition(centerOffset);

    if (flipX) {
      sprite.setScale({-2.f, 2.f});
      sprite.setOrigin({static_cast<float>(rect.size.x), 0.f});
    } else {
      sprite.setScale({2.f, 2.f});
      sprite.setOrigin({0.f, 0.f});
    }

    preview.canvas->draw(sprite);
    preview.canvas->display();
  }
}

void ProductionUI::open(std::shared_ptr<Building> factory) {
  close();

  bool hasVehicleBase = false;
  bool hasAirport = false;
  bool hasPort = false;

  bool sitsOnWater =
      m_mapManager.getTerrainAt(sf::Vector2i(factory->getPosition().x / 32,
                                             factory->getPosition().y / 32)) ==
      TerrainType::Water;

  for (const auto &b : m_mapManager.getBuildings()) {
    if (b->getTeam() == factory->getTeam()) {
      if (b->getTypeName() == "VehicleBase")
        hasVehicleBase = true;
      if (b->getTypeName() == "Airport")
        hasAirport = true;
      if (b->getTypeName() == "Port" && sitsOnWater)
        hasPort = true;
    }
  }

  Team team = factory->getTeam();
  int currentMoney = 0;
  auto moneyIt = m_mapManager.getTeams().find(team);
  if (moneyIt != m_mapManager.getTeams().end())
    currentMoney = moneyIt->second.money;

  std::vector<Category> categories = {{"Infantry", {}, true},
                                      {"Ground", {}, hasVehicleBase},
                                      {"Flying", {}, hasAirport},
                                      {"Naval", {}, hasPort}};

  auto unitNames = UnitRegistry::getRegisteredUnitNames();
  std::sort(unitNames.begin(), unitNames.end());
  for (const auto &uName : unitNames) {
    const UnitData *d = UnitRegistry::getData(uName);
    if (!d)
      continue;
    switch (d->movementCategory) {
    case MovementCategory::Infantry:
      categories[0].units.push_back(uName);
      break;
    case MovementCategory::Ground:
      categories[1].units.push_back(uName);
      break;
    case MovementCategory::Flying:
      categories[2].units.push_back(uName);
      break;
    case MovementCategory::Naval:
      categories[3].units.push_back(uName);
      break;
    }
  }
  categories.erase(
      std::remove_if(categories.begin(), categories.end(),
                     [](const Category &c) { return c.units.empty(); }),
      categories.end());

  m_panel = tgui::Panel::create();
  m_panel->setSize("80% - 200", "80% - 120");
  m_panel->setPosition("10%", "10%");
  m_panel->getRenderer()->setBackgroundColor(BG_DARK);
  m_panel->getRenderer()->setBorders(tgui::Borders(2));
  m_panel->getRenderer()->setBorderColor(BORDER_COL);
  m_gui.add(m_panel);

  auto titleLabel = tgui::Label::create("UNIT PRODUCTION");
  titleLabel->setPosition(16, 10);
  titleLabel->setTextSize(18);
  titleLabel->getRenderer()->setTextColor(TEXT_NORMAL);
  m_panel->add(titleLabel);

  auto moneyLabel =
      tgui::Label::create("Funds: $" + std::to_string(currentMoney));
  moneyLabel->setPosition(260, 12);
  moneyLabel->setTextSize(14);
  moneyLabel->getRenderer()->setTextColor(sf::Color(80, 220, 80));
  m_panel->add(moneyLabel);

  auto closeBtn = makeStyledBtn("Close [Esc]");
  closeBtn->setSize(135, 28);
  closeBtn->setPosition("100% - 145", 8);
  closeBtn->onClick([this]() { close(); });
  m_panel->add(closeBtn);

  auto divider = tgui::Panel::create();
  divider->setSize("100%", 2);
  divider->setPosition(0, 42);
  divider->getRenderer()->setBackgroundColor(BORDER_COL);
  m_panel->add(divider);

  auto scrollable = tgui::ScrollablePanel::create();
  scrollable->setPosition(0, 46);
  scrollable->setSize("100%", "100% - 50");
  scrollable->getRenderer()->setBackgroundColor(sf::Color::Transparent);
  scrollable->getRenderer()->setBorders(tgui::Borders(0));
  scrollable->getRenderer()->setScrollbarWidth(8);
  m_panel->add(scrollable);

  const float CARD_W = 130.f;
  const float CARD_H = 160.f;
  const float CARD_PAD = 10.f;
  const float CAT_LABEL_H = 26.f;
  const float CAT_PAD = 8.f;

  float curY = 4.f;
  for (const auto &cat : categories) {
    auto catStrip = tgui::Panel::create();
    catStrip->setSize("100%", CAT_LABEL_H);
    catStrip->setPosition(0, curY);
    catStrip->getRenderer()->setBackgroundColor(BG_CAT);
    catStrip->getRenderer()->setBorders(tgui::Borders(0, 1, 0, 1));
    catStrip->getRenderer()->setBorderColor(BORDER_COL);
    scrollable->add(catStrip);

    auto catLabel =
        tgui::Label::create(cat.name + (cat.unlocked ? "" : "  [Locked]"));
    catLabel->setPosition(12, 4);
    catLabel->setTextSize(13);
    catLabel->getRenderer()->setTextColor(cat.unlocked ? TEXT_NORMAL
                                                       : TEXT_GREY);
    catStrip->add(catLabel);
    curY += CAT_LABEL_H;

    float curX = CARD_PAD;
    for (const auto &uName : cat.units) {
      const UnitData *data = UnitRegistry::getData(uName);
      auto dummy = UnitRegistry::create(uName, Team::Ally);
      if (!data || !dummy)
        continue;

      bool canAfford = cat.unlocked && (currentMoney >= data->cost);

      auto card = tgui::Panel::create();
      card->setSize(CARD_W, CARD_H);
      card->setPosition(curX, curY + CAT_PAD);
      card->getRenderer()->setBackgroundColor(BG_CARD);
      card->getRenderer()->setBorders(tgui::Borders(1));
      card->getRenderer()->setBorderColor(BORDER_COL);
      scrollable->add(card);

      const float IMG_SIZE = 64.f;
      auto imgPanel = tgui::Panel::create();
      imgPanel->setSize(IMG_SIZE, IMG_SIZE);
      imgPanel->setPosition((CARD_W - IMG_SIZE) / 2.f, 6);
      imgPanel->getRenderer()->setBackgroundColor(sf::Color::Transparent);
      imgPanel->getRenderer()->setBorders(tgui::Borders(0));
      card->add(imgPanel);

      AnimatedPreview preview;
      const AnimationSet *animSet = AnimationManager::getSet(uName);
      if (animSet && dummy) {
        dummy->setDirection(MoveDirection::Left);

        auto canvas = tgui::CanvasSFML::create({IMG_SIZE, IMG_SIZE});
        canvas->clear(sf::Color::Transparent);
        canvas->display();
        imgPanel->add(canvas);

        preview.unit = dummy;
        preview.canvas = canvas;
        preview.timer = 1.f;
        preview.dirIndex = 0;
        m_previews.push_back(std::move(preview));
      }

      auto nameLabel = tgui::Label::create(uName);
      nameLabel->setPosition(6, IMG_SIZE + 10);
      nameLabel->setTextSize(13);
      nameLabel->getRenderer()->setTextColor(TEXT_NORMAL);
      card->add(nameLabel);

      auto statsLabel =
          tgui::Label::create("HP:" + std::to_string(data->maxHealth) +
                              " Atk:" + std::to_string(data->damage));
      statsLabel->setPosition(6, IMG_SIZE + 28);
      statsLabel->setTextSize(11);
      statsLabel->getRenderer()->setTextColor(TEXT_GREY);
      card->add(statsLabel);

      auto buyBtn = makeStyledBtn("$" + std::to_string(data->cost));
      buyBtn->setSize(CARD_W - 12, 26);
      buyBtn->setPosition(6, CARD_H - 32);
      if (!canAfford) {
        buyBtn->setEnabled(false);
      } else {
        buyBtn->onClick([this, uName, data, team, factory]() {
          auto newUnit = UnitRegistry::create(uName, team);
          if (newUnit) {
            newUnit->setActed(true);
            newUnit->setFadeIn(0.3f);
            m_mapManager.deductTeamMoney(team, data->cost);
            const sf::Vector2u ts = m_mapManager.getTileSize();
            sf::Vector2i bGrid(
                static_cast<int>(std::round(factory->getPosition().x / static_cast<float>(ts.x))),
                static_cast<int>(std::round(factory->getPosition().y / static_cast<float>(ts.y))));
            m_mapManager.spawnUnit(newUnit, bGrid.x, bGrid.y);
            m_turnController.markActed(*newUnit);
          }
          close();
        });
      }
      card->add(buyBtn);

      curX += CARD_W + CARD_PAD;
    }

    curY += CARD_H + CAT_PAD * 2;
  }
}
