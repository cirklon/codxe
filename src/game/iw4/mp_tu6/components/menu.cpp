#include "pch.h"
#include "menu.h"
#include <cstring>
#include <cstdio>

namespace iw4
{
namespace mp_tu6
{

bool menu::s_open = false;
int menu::s_cursor = 0;
MenuPage menu::s_page = Main;
std::vector<MenuOption> menu::s_options;
std::vector<MenuPage> menu::s_pageStack;
Material *menu::s_whiteMaterial = NULL;
Font_s *menu::s_font = NULL;
Detour menu::s_drawDetour;
Detour menu::s_cmdDetour;

bool menu::s_testToggle = false;
int menu::s_testArrayIndex = 0;
float menu::s_testIncrement = 50.0f;

static const char *kTestArrayValues[] = {"Low", "Medium", "High"};
static const char *kToggleValues[] = {"OFF", "ON"};

static const float kColorPanel[4] = {0.109803f, 0.129411f, 0.156862f, 0.8f};
static const float kColorPanelRight[4] = {0.109803f, 0.129411f, 0.156862f, 0.8f};
static const float kColorSelected[4] = {0.922f, 0.204f, 0.310f, 0.6f};
static const float kColorLine[4] = {1.0f, 1.0f, 1.0f, 0.10f};
static const float kColorBorder[4] = {0.922f, 0.204f, 0.310f, 0.6f};
static const float kColorText[4] = {0.82f, 0.82f, 0.82f, 1.0f};
static const float kColorTextSelected[4] = {1.0f, 1.0f, 1.0f, 1.0f};
static const float kColorTitle[4] = {1.0f, 1.0f, 1.0f, 1.0f};
static const float kColorSubtitle[4] = {0.55f, 0.55f, 0.55f, 1.0f};
static const float kColorLabelMuted[4] = {0.48f, 0.48f, 0.50f, 1.0f};
static const float kColorHighlight[4] = {0.922f, 0.204f, 0.310f, 0.6f}; // light red accent for current-value / selected list entry
static const float kColorSliderBg[4] = {1.0f, 1.0f, 1.0f, 0.12f};
static const float kColorSliderFill[4] = {1.0f, 1.0f, 1.0f, 0.85f};

static void DrawBorder(float x, float y, float w, float h, float t, const float *color, Material *mat)
{
    R_AddCmdDrawStretchPic(x, y, w, t, 0, 0, 1, 1, color, mat);
    R_AddCmdDrawStretchPic(x, y + h - t, w, t, 0, 0, 1, 1, color, mat);
    R_AddCmdDrawStretchPic(x, y, t, h, 0, 0, 1, 1, color, mat);
    R_AddCmdDrawStretchPic(x + w - t, y, t, h, 0, 0, 1, 1, color, mat);
}

// Draws a '\n'-delimited block of text, one R_AddCmdDrawText call per line.
static void DrawMultilineText(const char *text, Font_s *font, float x, float y, float scale, float lineHeight, const float *color)
{
    if (!text || !*text) return;

    char buffer[512];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    float curY = y;
    char *ctx = NULL;
    char *line = strtok_s(buffer, "\n", &ctx);
    while (line)
    {
        R_AddCmdDrawText(line, 0x7FFFFFFF, font, x, curY, scale, scale, 0.0f, color, 0);
        curY += lineHeight;
        line = strtok_s(NULL, "\n", &ctx);
    }
}

static void CG_DrawActive_Hook(int localClientNum)
{
    menu::s_drawDetour.GetOriginal<decltype(CG_DrawActive)>()(localClientNum);
    menu::Draw(localClientNum);
}

static void CL_CreateNewCommands_Hook(int localClientNum)
{
    menu::s_cmdDetour.GetOriginal<decltype(CL_CreateNewCommands)>()(localClientNum);

    if (localClientNum < 0) return;

    clientActive_t *cl = clients[localClientNum];
    if (!cl) return;

    usercmd_s &cmd = cl->cmds[cl->cmdNumber & 0x7F];
    menu::ProcessInput(localClientNum, cmd);

    if (menu::IsOpen())
    {
        cmd.buttons = 0;
        cmd.forwardmove = 0;
        cmd.rightmove = 0;
    }
}

menu::menu()
{
    s_page = Main;
    s_cursor = 0;
    s_open = false;
    s_pageStack.clear();
    BuildOptions(s_page);

    s_whiteMaterial = NULL;
    s_font = NULL;

    s_drawDetour = Detour(CG_DrawActive, CG_DrawActive_Hook);
    s_drawDetour.Install();

    s_cmdDetour = Detour(CL_CreateNewCommands, CL_CreateNewCommands_Hook);
    s_cmdDetour.Install();
}

menu::~menu()
{
    s_cmdDetour.Remove();
    s_drawDetour.Remove();
}

bool menu::IsOpen()
{
    return s_open;
}

MenuOption menu::MakeAction(const char *label, const char *description, void (*cb)(int))
{
    MenuOption opt;
    memset(&opt, 0, sizeof(opt));
    opt.label = label;
    opt.description = description;
    opt.callback = cb;
    opt.type = OPT_ACTION;
    return opt;
}

MenuOption menu::MakeSubmenu(const char *label, const char *description, MenuPage targetPage)
{
    MenuOption opt;
    memset(&opt, 0, sizeof(opt));
    opt.label = label;
    opt.description = description;
    opt.callback = OpenSubmenuCallback;
    opt.type = OPT_ACTION;
    opt.isSubmenu = true;
    opt.targetPage = targetPage;
    return opt;
}

MenuOption menu::MakeToggle(const char *label, const char *description, bool *value, void (*cb)(int))
{
    MenuOption opt;
    memset(&opt, 0, sizeof(opt));
    opt.label = label;
    opt.description = description;
    opt.callback = cb;
    opt.type = OPT_TOGGLE;
    opt.toggleValue = value;
    return opt;
}

MenuOption menu::MakeArray(const char *label, const char *description, const char *const *values, int count, int *index, void (*cb)(int))
{
    MenuOption opt;
    memset(&opt, 0, sizeof(opt));
    opt.label = label;
    opt.description = description;
    opt.callback = cb;
    opt.type = OPT_ARRAY;
    opt.arrayValues = values;
    opt.arrayCount = count;
    opt.arrayIndex = index;
    return opt;
}

MenuOption menu::MakeIncrement(const char *label, const char *description, float *value, float min, float max, float step, void (*cb)(int))
{
    MenuOption opt;
    memset(&opt, 0, sizeof(opt));
    opt.label = label;
    opt.description = description;
    opt.callback = cb;
    opt.type = OPT_INCREMENT;
    opt.incValue = value;
    opt.incMin = min;
    opt.incMax = max;
    opt.incStep = step;
    return opt;
}

void menu::OpenSubmenuCallback(int localClientNum)
{
    (void)localClientNum;

    if (s_cursor < 0 || s_cursor >= (int)s_options.size()) return;

    MenuOption &opt = s_options[s_cursor];
    if (!opt.isSubmenu) return;

    s_pageStack.push_back(s_page);
    s_page = opt.targetPage;
    BuildOptions(s_page);
}

void menu::BackCallback(int localClientNum)
{
    (void)localClientNum;
    Back();
}

void menu::BuildOptions(MenuPage page)
{
    s_options.clear();
    s_cursor = 0;

    switch (page)
    {
    case Main:
        s_options.push_back(MakeAction("Test Action", "Runs a placeholder action with no persistent state.", NULL));
        s_options.push_back(MakeToggle("Test Toggle", "Flips a boolean test value on or off.", &s_testToggle, NULL));
        s_options.push_back(MakeArray("Test Array", "Changes the quality level.", kTestArrayValues, 3, &s_testArrayIndex, NULL));
        s_options.push_back(MakeIncrement("Test Increment", "Adjusts a float value between 0 and 100.", &s_testIncrement, 0.0f, 100.0f, 5.0f, NULL));
        s_options.push_back(MakeSubmenu("Player", "Options that affect your own player.", Player));
        s_options.push_back(MakeSubmenu("Weapons", "Options related to weapons and ammo.", Weapons));
        break;

    case Player:
        s_options.push_back(MakeAction("Back", "Return to the previous menu.", BackCallback));
        break;

    case Weapons:
        s_options.push_back(MakeAction("Back", "Return to the previous menu.", BackCallback));
        break;
    }
}

void menu::Navigate(int delta)
{
    if (s_options.empty()) return;

    int count = (int)s_options.size();
    s_cursor = (s_cursor + delta + count) % count;
}

void menu::Adjust(int delta)
{
    if (s_cursor < 0 || s_cursor >= (int)s_options.size()) return;

    MenuOption &opt = s_options[s_cursor];

    if (opt.type == OPT_INCREMENT && opt.incValue)
    {
        *opt.incValue += opt.incStep * (float)delta;
        if (*opt.incValue < opt.incMin) *opt.incValue = opt.incMin;
        if (*opt.incValue > opt.incMax) *opt.incValue = opt.incMax;
        if (opt.callback) opt.callback(0);
    }
    else if (opt.type == OPT_ARRAY && opt.arrayIndex && opt.arrayCount > 0)
    {
        int idx = *opt.arrayIndex;
        idx = (idx + delta) % opt.arrayCount;
        if (idx < 0) idx += opt.arrayCount;
        *opt.arrayIndex = idx;
        if (opt.callback) opt.callback(0);
    }
    else if (opt.type == OPT_TOGGLE && opt.toggleValue && delta != 0)
    {
        // L/R can flip a toggle too, matching how it cycles array/increment values.
        *opt.toggleValue = !(*opt.toggleValue);
        if (opt.callback) opt.callback(0);
    }
}

void menu::Select(int localClientNum)
{
    if (s_cursor < 0 || s_cursor >= (int)s_options.size()) return;

    MenuOption &opt = s_options[s_cursor];

    if (opt.type == OPT_TOGGLE && opt.toggleValue)
    {
        *opt.toggleValue = !(*opt.toggleValue);
        if (opt.callback) opt.callback(localClientNum);
        return;
    }

    if (opt.type == OPT_ARRAY && opt.arrayIndex && opt.arrayCount > 0)
    {
        // A/Select also advances the array by one, same convenience as toggle.
        Adjust(1);
        return;
    }

    if (opt.callback) opt.callback(localClientNum);
}

void menu::Back()
{
    if (s_pageStack.empty())
    {
        s_open = false;
        return;
    }

    s_page = s_pageStack.back();
    s_pageStack.pop_back();
    BuildOptions(s_page);
}

void menu::Toggle()
{
    s_open = !s_open;
    if (s_open) s_cursor = 0;
}

void menu::ProcessInput(int localClientNum, usercmd_s &cmd)
{
    static int prevButtons = 0;
    static int navCooldown = 0;
    static int adjustCooldown = 0;

    int buttons = cmd.buttons;
    int pressed = buttons & ~prevButtons;
    prevButtons = buttons;

    if ((buttons & CMD_BUTTON_ADS) && (pressed & CMD_BUTTON_USE_RELOAD))
    {
        Toggle();
        return;
    }

    if (!s_open) return;

    if (pressed & CMD_BUTTON_UP) Select(localClientNum);
    if (pressed & CMD_BUTTON_MELEE) Back();

    if (navCooldown > 0)
        navCooldown--;
    else if (cmd.forwardmove > 40)
    {
        Navigate(-1);
        navCooldown = 8;
    }
    else if (cmd.forwardmove < -40)
    {
        Navigate(1);
        navCooldown = 8;
    }

    if (adjustCooldown > 0)
        adjustCooldown--;
    else if (cmd.rightmove > 40)
    {
        Adjust(1);
        adjustCooldown = 8;
    }
    else if (cmd.rightmove < -40)
    {
        Adjust(-1);
        adjustCooldown = 8;
    }
}

void menu::Draw(int localClientNum)
{
    (void)localClientNum;
    if (!s_open) return;
    if (!s_whiteMaterial) s_whiteMaterial = Material_RegisterHandle("white");
    if (!s_font) s_font = R_RegisterFont("fonts/boldfont");
    if (!s_whiteMaterial || !s_font) return;

    // ---- layout constants (compact pass - no footer, tighter everywhere) ----
    const float kLeftW = 300.0f;
    const float kRightW = 205.0f;
    const float kMenuOffsetX = 230.0f;

    const float kHeaderH = 32.0f;
    const float kRowH = 22.0f;
    const float kBottomPad = 8.0f; // small breathing room under the last row, no controls bar
    const float kPadL = 12.0f;
    const float kPadR = 12.0f;
    const float kBorder = 0.0f;

    const float kTextScale = 0.32f;
    const float kTitleScale = 0.40f;
    const float kSubScale = 0.24f;

    const int kOptionLimit = 6;

    // right-panel typography
	const float kRLabelScale = 0.28f;   // section headers
	const float kRValueScale = 0.50f;   // current value
	const float kRListScale = 0.36f;    // available values / range
	const float kRDescScale = 0.33f;    // description
	const float kRLineH = 15.0f;
	const float kRListRowH = 21.0f;
	const float kRSliderH = 4.0f;

    const ScreenPlacement *scrPlace = ScrPlace_GetFullPlacement();
    float screenMinX = scrPlace->virtualViewableMin[0];
    float screenMaxX = scrPlace->virtualViewableMax[0];
    float screenMinY = scrPlace->virtualViewableMin[1];
    float screenMaxY = scrPlace->virtualViewableMax[1];
    float screenW = screenMaxX - screenMinX;
    float screenH = screenMaxY - screenMinY;

    int total = (int)s_options.size();
    int limit = total < kOptionLimit ? total : kOptionLimit;
    int start = 0;

    if (total > limit)
    {
        start = s_cursor - (limit - 1) / 2;
        if (start < 0) start = 0;
        if (start > total - limit) start = total - limit;
    }

    float bodyH = kRowH * (float)limit;
    float panelH = kHeaderH + bodyH + kBottomPad;
    float totalW = kLeftW + kRightW;

    float x = screenMinX + ((screenW - totalW) * 0.5f) + kMenuOffsetX;
    float y = screenMinY + ((screenH - panelH) * 0.5f);

    float leftX = x;
    float rightX = x + kLeftW;
    float labelX = leftX + kPadL;
    float rowRight = leftX + kLeftW - kPadR;

    // ---- left panel: background + header -----------------------------------
    R_AddCmdDrawStretchPic(leftX, y, kLeftW, panelH, 0, 0, 1, 1, kColorPanel, s_whiteMaterial);
    DrawBorder(leftX, y, kLeftW, panelH, kBorder, kColorBorder, s_whiteMaterial);

    R_AddCmdDrawText("\\\\o//", 0x7FFFFFFF, s_font, labelX, y + 15.0f, kTitleScale, kTitleScale, 0.0f, kColorTitle, 0);
    R_AddCmdDrawText("Main Menu", 0x7FFFFFFF, s_font, labelX, y + 26.0f, kSubScale, kSubScale, 0.0f, kColorSubtitle, 0);

    // ---- right panel: background --------------------------------------------
    R_AddCmdDrawStretchPic(rightX, y, kRightW, panelH, 0, 0, 1, 1, kColorPanelRight, s_whiteMaterial);
    DrawBorder(rightX, y, kRightW, panelH, kBorder, kColorBorder, s_whiteMaterial);
    R_AddCmdDrawStretchPic(rightX, y, kBorder, panelH, 0, 0, 1, 1, kColorBorder, s_whiteMaterial);

    // ---- left panel: option rows ---------------------------------------------
    for (int i = 0; i < limit; ++i)
    {
        int index = start + i;
        MenuOption &opt = s_options[index];
        bool selected = index == s_cursor;
        float rowY = y + kHeaderH + ((float)i * kRowH);
        float textY = rowY + kRowH - 6.0f;
        const float *tc = selected ? kColorTextSelected : kColorText;

        if (selected)
            R_AddCmdDrawStretchPic(leftX, rowY, kLeftW, kRowH, 0, 0, 1, 1, kColorSelected, s_whiteMaterial);

        R_AddCmdDrawText(opt.label, 0x7FFFFFFF, s_font, labelX, textY, kTextScale, kTextScale, 0.0f, tc, 0);

        // Value display (ON/OFF, array value, slider) lives entirely in the
        // right-hand detail panel - the row itself only ever shows the label
        // plus a submenu arrow.
        if (opt.type == OPT_ACTION && opt.isSubmenu)
            R_AddCmdDrawText(">", 0x7FFFFFFF, s_font, rowRight - 5.0f, textY, kTextScale, kTextScale, 0.0f, tc, 0);
    }

    // ---- right panel: detail content for the highlighted row ----------------
    if (s_cursor >= 0 && s_cursor < (int)s_options.size())
    {
        MenuOption &opt = s_options[s_cursor];
        float rx = rightX + 12.0f;
        float rw = kRightW - 24.0f;
        float cy = y + 14.0f;

        bool hasValue = opt.type == OPT_TOGGLE || opt.type == OPT_ARRAY || opt.type == OPT_INCREMENT;

        if (hasValue)
        {
            R_AddCmdDrawText("CURRENT VALUE", 0x7FFFFFFF, s_font, rx, cy, kRLabelScale, kRLabelScale, 0.0f, kColorLabelMuted, 0);
            cy += kRLineH + 4.0f;

            char valueBuf[32];
            const char *valueStr = valueBuf;

            if (opt.type == OPT_TOGGLE)
                valueStr = (opt.toggleValue && *opt.toggleValue) ? "ON" : "OFF";
            else if (opt.type == OPT_ARRAY)
            {
                int idx = opt.arrayIndex ? *opt.arrayIndex : 0;
                if (idx < 0) idx = 0;
                if (opt.arrayCount > 0 && idx >= opt.arrayCount) idx = opt.arrayCount - 1;
                valueStr = (opt.arrayValues && opt.arrayCount > 0 && opt.arrayValues[idx]) ? opt.arrayValues[idx] : "NULL";
            }
            else // OPT_INCREMENT
            {
                sprintf(valueBuf, "%.0f", opt.incValue ? *opt.incValue : 0.0f);
            }

            R_AddCmdDrawText(valueStr, 0x7FFFFFFF, s_font, rx, cy + 8.0f, kRValueScale, kRValueScale, 0.0f, kColorHighlight, 0);
            cy += 22.0f;

            if (opt.type == OPT_INCREMENT)
            {
                float t = 0.0f;
                if (opt.incValue && (opt.incMax - opt.incMin) != 0.0f)
                    t = (*opt.incValue - opt.incMin) / (opt.incMax - opt.incMin);
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;

                R_AddCmdDrawStretchPic(rx, cy, rw, kRSliderH, 0, 0, 1, 1, kColorSliderBg, s_whiteMaterial);
                R_AddCmdDrawStretchPic(rx, cy, rw * t, kRSliderH, 0, 0, 1, 1, kColorSliderFill, s_whiteMaterial);
                cy += kRSliderH + 8.0f;
            }

            R_AddCmdDrawStretchPic(rx, cy, rw, 1.0f, 0, 0, 1, 1, kColorLine, s_whiteMaterial);
            cy += 11.0f;

            if (opt.type == OPT_TOGGLE)
            {
                R_AddCmdDrawText("AVAILABLE VALUES", 0x7FFFFFFF, s_font, rx, cy, kRLabelScale, kRLabelScale, 0.0f, kColorLabelMuted, 0);
                cy += kRLineH + 4.0f;

                bool on = opt.toggleValue && *opt.toggleValue;
                for (int i = 0; i < 2; ++i)
                {
                    bool sel = (i == 1) == on;
                    const float *c = sel ? kColorHighlight : kColorText;
                    if (sel)
                        R_AddCmdDrawText(">", 0x7FFFFFFF, s_font, rx, cy, kRListScale, kRListScale, 0.0f, c, 0);
                    R_AddCmdDrawText(kToggleValues[i], 0x7FFFFFFF, s_font, rx + 12.0f, cy, kRListScale, kRListScale, 0.0f, c, 0);
                    cy += kRListRowH;
                }

                cy += 4.0f;
                R_AddCmdDrawStretchPic(rx, cy, rw, 1.0f, 0, 0, 1, 1, kColorLine, s_whiteMaterial);
                cy += 11.0f;
            }
            else if (opt.type == OPT_ARRAY && opt.arrayValues && opt.arrayCount > 0)
            {
                R_AddCmdDrawText("AVAILABLE VALUES", 0x7FFFFFFF, s_font, rx, cy, kRLabelScale, kRLabelScale, 0.0f, kColorLabelMuted, 0);
                cy += kRLineH + 4.0f;

                int curIdx = opt.arrayIndex ? *opt.arrayIndex : 0;
                for (int i = 0; i < opt.arrayCount; ++i)
                {
                    bool sel = i == curIdx;
                    const float *c = sel ? kColorHighlight : kColorText;
                    if (sel)
                        R_AddCmdDrawText(">", 0x7FFFFFFF, s_font, rx, cy, kRListScale, kRListScale, 0.0f, c, 0);
                    R_AddCmdDrawText(opt.arrayValues[i], 0x7FFFFFFF, s_font, rx + 12.0f, cy, kRListScale, kRListScale, 0.0f, c, 0);
                    cy += kRListRowH;
                }

                cy += 4.0f;
                R_AddCmdDrawStretchPic(rx, cy, rw, 1.0f, 0, 0, 1, 1, kColorLine, s_whiteMaterial);
                cy += 11.0f;
            }
            else if (opt.type == OPT_INCREMENT)
            {
                R_AddCmdDrawText("RANGE", 0x7FFFFFFF, s_font, rx, cy, kRLabelScale, kRLabelScale, 0.0f, kColorLabelMuted, 0);
                cy += kRLineH + 4.0f;

                char rangeBuf[48];
                sprintf(rangeBuf, "%.0f - %.0f (step %.0f)", opt.incMin, opt.incMax, opt.incStep);
                R_AddCmdDrawText(rangeBuf, 0x7FFFFFFF, s_font, rx, cy, kRListScale, kRListScale, 0.0f, kColorText, 0);
                cy += kRListRowH;

                cy += 4.0f;
                R_AddCmdDrawStretchPic(rx, cy, rw, 1.0f, 0, 0, 1, 1, kColorLine, s_whiteMaterial);
                cy += 11.0f;
            }
        }

        R_AddCmdDrawText("DESCRIPTION", 0x7FFFFFFF, s_font, rx, cy, kRLabelScale, kRLabelScale, 0.0f, kColorLabelMuted, 0);
        cy += kRLineH + 4.0f;
        DrawMultilineText(opt.description, s_font, rx, cy, kRDescScale, kRLineH, kColorText);
    }
}

} // namespace mp_tu6
} // namespace iw4