#include "RightPanelUI.hpp"

static constexpr float PANEL_W       = 160.f;
static constexpr float PANEL_H       = 360.f;
static constexpr float PADDING       =  12.f;
static constexpr float GAP_BOX       =   4.f;
static constexpr float GAP_BTN_H     =   8.f;
static constexpr float GAP_BTN_V     =   8.f;

static tgui::Button::Ptr makeImageButton(const std::string& texturePath) {
    auto btn = tgui::Button::create();
    btn->setText("");
    auto r = btn->getRenderer();
    tgui::Texture tex(texturePath);
    r->setTexture(tex);
    r->setTextureHover(tex);
    r->setTextureDown(tex);
    r->setBackgroundColor(sf::Color::Transparent);
    r->setBackgroundColorHover(sf::Color(255, 255, 255, 18));
    r->setBackgroundColorDown(sf::Color(0, 0, 0, 30));
    r->setBorderColor(sf::Color::Transparent);
    r->setBorderColorHover(sf::Color::Transparent);
    r->setBorderColorDown(sf::Color::Transparent);
    r->setBorders(tgui::Borders(0));
    return btn;
}

RightPanelWidgets buildRightPanelUI(
    tgui::Gui& gui,
    float panelOriginXGU,
    float panelOriginYGU,
    float scaleX,
    float scaleY,
    int   numBoxes,
    float boxHeightGU,
    float btnHeightGU,
    float endTurnHeightGU)
{
    RightPanelWidgets result;

    const float ox = panelOriginXGU * scaleX;
    const float oy = panelOriginYGU * scaleY;
    const float sx = scaleX;
    const float sy = scaleY;

    auto panelBg = tgui::Picture::create(tgui::Texture("Art/UI/right_panel.png"));
    panelBg->setPosition(ox, oy);
    panelBg->setSize(PANEL_W * sx, PANEL_H * sy);
    gui.add(panelBg);

    const float innerW   = PANEL_W - 2.f * PADDING;
    const float halfBtnW = (innerW - GAP_BTN_H) / 2.f;

    float curY = PADDING;

    for (int i = 0; i < numBoxes; ++i) {
        auto box = tgui::Panel::create();
        box->setPosition(ox + PADDING * sx, oy + curY * sy);
        box->setSize(innerW * sx, boxHeightGU * sy);
        box->getRenderer()->setBackgroundColor(sf::Color(8, 16, 28, 180));
        box->getRenderer()->setBorderColor(sf::Color(55, 85, 115, 200));
        box->getRenderer()->setBorders(tgui::Borders(1));
        result.infoBoxes.push_back(box);
        gui.add(box);
        curY += boxHeightGU + GAP_BOX;
    }
    if (numBoxes > 0)
        curY -= GAP_BOX;

    curY += PADDING;

    result.undoBtn = makeImageButton("Art/UI/undo_btn.png");
    result.undoBtn->setPosition(ox + PADDING * sx, oy + curY * sy);
    result.undoBtn->setSize(halfBtnW * sx, btnHeightGU * sy);
    gui.add(result.undoBtn);

    result.nextUnitBtn = makeImageButton("Art/UI/next_unit_btn.png");
    result.nextUnitBtn->setPosition(ox + (PADDING + halfBtnW + GAP_BTN_H) * sx, oy + curY * sy);
    result.nextUnitBtn->setSize(halfBtnW * sx, btnHeightGU * sy);
    gui.add(result.nextUnitBtn);

    curY += btnHeightGU + GAP_BTN_V;

    result.settingsBtn = makeImageButton("Art/UI/settings_btn.png");
    result.settingsBtn->setPosition(ox + PADDING * sx, oy + curY * sy);
    result.settingsBtn->setSize(innerW * sx, btnHeightGU * sy);
    gui.add(result.settingsBtn);
    const float endTurnY = PANEL_H - PADDING - endTurnHeightGU;
    result.endTurnBtn = makeImageButton("Art/UI/end_turn_btn.png");
    result.endTurnBtn->setPosition(ox + PADDING * sx, oy + endTurnY * sy);
    result.endTurnBtn->setSize(innerW * sx, endTurnHeightGU * sy);
    gui.add(result.endTurnBtn);

    return result;
}
