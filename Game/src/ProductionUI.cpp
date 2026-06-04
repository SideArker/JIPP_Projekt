#include "ProductionUI.hpp"
#include "AnimationManager.hpp"
#include "UnitRegistry.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <string>

static const sf::Color BG_DARK(15, 25, 35);
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
  for (auto &preview : m_previews) {
    if (preview.frames.empty())
      continue;
    preview.timer -= dt;
    if (preview.timer <= 0.f) {
      preview.timer = 1.f;
      preview.frames[preview.index]->setVisible(false);
      preview.index =
          (preview.index + 1) % static_cast<int>(preview.frames.size());
      preview.frames[preview.index]->setVisible(true);
    }
  }
}

void ProductionUI::open(std::shared_ptr<Building> factory) {
  close();

  bool hasVehicleBase = false;
  bool hasAirport = false;
  bool hasPort = false;
  for (const auto &b : m_mapManager.getBuildings()) {
    if (b->getTeam() == factory->getTeam()) {
      if (b->getTypeName() == "VehicleBase")
        hasVehicleBase = true;
      if (b->getTypeName() == "Airport")
        hasAirport = true;
      if (b->getTypeName() == "Port")
        hasPort = true;
    }
  }

  Team team = factory->getTeam();
  int currentMoney = 0;
  auto moneyIt = m_mapManager.getTeams().find(team);
  if (moneyIt != m_mapManager.getTeams().end())
    currentMoney = moneyIt->second.money;

  struct Category {
    std::string name;
    std::vector<std::string> units;
    bool unlocked = false;
  };
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
  m_panel->setSize("100% - 250px", "100% - 152px");
  m_panel->setPosition(0, 0);
  m_panel->getRenderer()->setBackgroundColor(BG_DARK);
  m_panel->getRenderer()->setBorders(tgui::Borders(0, 0, 2, 2));
  m_panel->getRenderer()->setBorderColor(BORDER_COL);
  m_gui.add(m_panel);

  auto titleLabel = tgui::Label::create("UNIT PRODUCTION");
  titleLabel->setPosition(16, 10);
  titleLabel->setTextSize(18);
  titleLabel->getRenderer()->setTextColor(TEXT_NORMAL);
  m_panel->add(titleLabel);

  auto moneyLabel = tgui::Label::create("$" + std::to_string(currentMoney));
  moneyLabel->setPosition("100% - 140", 10);
  moneyLabel->setTextSize(16);
  moneyLabel->getRenderer()->setTextColor(sf::Color(80, 220, 80));
  m_panel->add(moneyLabel);

  auto closeBtn = makeStyledBtn("Close [Esc]");
  closeBtn->setSize(110, 28);
  closeBtn->setPosition("100% - 126", 8);
  closeBtn->onClick([this]() { close(); });
  m_panel->add(closeBtn);
  // Divider line
  auto divider = tgui::Panel::create();
  divider->setSize("100%", 2);
  divider->setPosition(0, 40);
  divider->getRenderer()->setBackgroundColor(BORDER_COL);
  m_panel->add(divider);

  const float CARD_W = 130.f;
  const float CARD_H = 150.f;
  const float CARD_PAD = 10.f;
  const float CAT_LABEL_H = 26.f;
  const float CAT_PAD = 8.f;
  const float START_Y = 50.f;

  float curY = START_Y;
  for (const auto &cat : categories) {
    auto catStrip = tgui::Panel::create();
    catStrip->setSize("100%", CAT_LABEL_H);
    catStrip->setPosition(0, curY);
    catStrip->getRenderer()->setBackgroundColor(BG_CAT);
    catStrip->getRenderer()->setBorders(tgui::Borders(0, 1, 0, 1));
    catStrip->getRenderer()->setBorderColor(BORDER_COL);
    m_panel->add(catStrip);

    std::string catTitle = cat.name + (cat.unlocked ? "" : "  [Locked]");
    auto catLabel = tgui::Label::create(catTitle);
    catLabel->setPosition(12, 4);
    catLabel->setTextSize(13);
    catLabel->getRenderer()->setTextColor(cat.unlocked ? TEXT_NORMAL
                                                       : TEXT_GREY);
    catStrip->add(catLabel);
    curY += CAT_LABEL_H;

    float curX = CARD_PAD;
    for (const auto &uName : cat.units) {
      const UnitData *data = UnitRegistry::getData(uName);
      auto dummy = UnitRegistry::create(uName, Team::Neutral);
      if (!data || !dummy)
        continue;

      bool canAfford = cat.unlocked && (currentMoney >= data->cost);

      auto card = tgui::Panel::create();
      card->setSize(CARD_W, CARD_H);
      card->setPosition(curX, CAT_PAD);
      card->getRenderer()->setBackgroundColor(BG_CARD);
      card->getRenderer()->setBorders(tgui::Borders(1));
      card->getRenderer()->setBorderColor(BORDER_COL);

      const float IMG_SIZE = 64.f;
      auto imgPanel = tgui::Panel::create();
      imgPanel->setSize(IMG_SIZE, IMG_SIZE);
      imgPanel->setPosition((CARD_W - IMG_SIZE) / 2.f, 6);
      imgPanel->getRenderer()->setBackgroundColor(sf::Color::Transparent);
      imgPanel->getRenderer()->setBorders(tgui::Borders(0));
      card->add(imgPanel);

      AnimatedPreview preview;
      const AnimationSet *animSet = AnimationManager::getSet(uName);
      if (animSet) {
        const std::vector<std::pair<std::string, bool>> dirs = {
            {"_left", false},
            {"_down", false},
            {"_right", false},
            {"_up", false},
        };
        for (const auto &[suffix, _] : dirs) {
          const AnimationClip *clip = animSet->getClip("idle" + suffix);
          if (!clip)
            clip = animSet->getClip("walk" + suffix);
          if (!clip || clip->frames.empty())
            continue;

          const sf::IntRect &fr = clip->frames[0];
          tgui::UIntRect part(static_cast<unsigned>(fr.position.x),
                              static_cast<unsigned>(fr.position.y),
                              static_cast<unsigned>(fr.size.x),
                              static_cast<unsigned>(fr.size.y));

          std::string artPath = clip->texturePath.empty() ? dummy->getArtPath()
                                                          : clip->texturePath;

          auto pic = tgui::Picture::create(tgui::Texture(artPath, part));
          pic->setSize(IMG_SIZE, IMG_SIZE);
          pic->setVisible(preview.frames.empty());
          imgPanel->add(pic);
          preview.frames.push_back(pic);
        }
      }
      if (!preview.frames.empty()) {
        preview.timer = 1.f;
        m_previews.push_back(preview);
      }

      auto nameLabel = tgui::Label::create(uName);
      nameLabel->setPosition(6, IMG_SIZE + 10);
      nameLabel->setTextSize(13);
      nameLabel->getRenderer()->setTextColor(TEXT_NORMAL);
      card->add(nameLabel);

      auto statsLabel =
          tgui::Label::create("HP " + std::to_string(data->maxHealth) +
                              "  \u2694 " + std::to_string(data->damage));
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
          auto unit = UnitRegistry::create(uName, team);
          if (!unit)
            return;
          m_mapManager.deductTeamMoney(team, data->cost);
          const sf::Vector2u ts = m_mapManager.getTileSize();
          sf::Vector2i bGrid(
              static_cast<int>(std::round(factory->getPosition().x /
                                          static_cast<float>(ts.x))),
              static_cast<int>(std::round(factory->getPosition().y /
                                          static_cast<float>(ts.y))));
          m_mapManager.spawnUnit(unit, bGrid.x, bGrid.y);
          m_turnController.markActed(*unit);
          close();
        });
      }
      card->add(buyBtn);

      card->setPosition(curX, curY + CAT_PAD);
      m_panel->add(card);

      curX += CARD_W + CARD_PAD;
    }

    curY += CARD_H + CAT_PAD * 2;
  }
}
