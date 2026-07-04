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

static const float kColorRowBg[4]        = {0.109803f, 0.129411f, 0.156862f, 0.80f};
static const float kColorSelectedBg[4]   = {0.9216f, 0.2039f, 0.3098f, 0.6f};
static const float kColorDivider[4]      = {1.0f, 1.0f, 1.0f, 0.15f};
static const float kColorTextNormal[4]   = {0.85f, 0.85f, 0.85f, 1.0f};
static const float kColorTextSelected[4] = {1.0f, 1.0f, 1.0f, 1.0f};
static const float kColorTitle[4]        = {1.0f, 1.0f, 1.0f, 1.0f};
static const float kColorScrollBar[4]    = {0.9216f, 0.2039f, 0.3098f, 1.0f};
static const float kColorScrollTrack[4]  = {1.0f, 1.0f, 1.0f, 0.08f};
static const float kColorToggleOn[4]     = {0.9216f, 0.2039f, 0.3098f, 1.0f};
static const float kColorToggleOff[4]    = {0.443f, 0.455f, 0.467f, 1.0f};
static const float kColorSliderTrack[4] = {0.109803f, 0.129411f, 0.156862f, 1.0f};
static const float kColorSliderKnob[4]  = {0.9216f, 0.2039f, 0.3098f, 1.0f};

static void CG_DrawActive_Hook(int localClientNum)
{
    menu::s_drawDetour.GetOriginal<decltype(CG_DrawActive)>()(localClientNum);
    menu::Draw(localClientNum);
}

static void CL_CreateNewCommands_Hook(int localClientNum)
{
    menu::s_cmdDetour.GetOriginal<decltype(CL_CreateNewCommands)>()(localClientNum);

    if (localClientNum < 0)
        return;

    clientActive_t *cl = clients[localClientNum];
    if (!cl)
        return;

    usercmd_s &cmd = cl->cmds[cl->cmdNumber & 0x7F];

    static int frame = 0;
    if ((frame++ % 60) == 0)
    {
        char buf[64];
        sprintf(buf, "buttons=%X fwd=%d", cmd.buttons, (int)cmd.forwardmove);
        CG_GameMessage(localClientNum, buf);
    }

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

MenuOption menu::MakeAction(const char *label, void (*cb)(int))
{
    MenuOption opt = {};
    opt.label = label;
    opt.callback = cb;
    opt.type = OPT_ACTION;
    return opt;
}

MenuOption menu::MakeToggle(const char *label, bool *value, void (*cb)(int))
{
    MenuOption opt = {};
    opt.label = label;
    opt.callback = cb;
    opt.type = OPT_TOGGLE;
    opt.toggleValue = value;
    return opt;
}

MenuOption menu::MakeArray(const char *label, const char **values, int count, int *index, void (*cb)(int))
{
    MenuOption opt = {};
    opt.label = label;
    opt.callback = cb;
    opt.type = OPT_ARRAY;
    opt.arrayValues = values;
    opt.arrayCount = count;
    opt.arrayIndex = index;
    return opt;
}

MenuOption menu::MakeIncrement(const char *label, float *value, float min, float max, float step, void (*cb)(int))
{
    MenuOption opt = {};
    opt.label = label;
    opt.callback = cb;
    opt.type = OPT_INCREMENT;
    opt.incValue = value;
    opt.incMin = min;
    opt.incMax = max;
    opt.incStep = step;
    return opt;
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
        s_options.push_back(MakeAction("Test Action", NULL));
        s_options.push_back(MakeToggle("Test Toggle", &s_testToggle, NULL));
        s_options.push_back(MakeArray("Test Array", kTestArrayValues, 3, &s_testArrayIndex, NULL));
        s_options.push_back(MakeIncrement("Test Increment", &s_testIncrement, 0.0f, 100.0f, 5.0f, NULL));
        break;
    }
}

void menu::Navigate(int delta)
{
    if (s_options.empty())
        return;

    int count = (int)s_options.size();
    s_cursor = (s_cursor + delta + count) % count;
}

void menu::Adjust(int delta)
{
    if (s_cursor < 0 || s_cursor >= (int)s_options.size())
        return;

    MenuOption &opt = s_options[s_cursor];

    if (opt.type == OPT_ARRAY && opt.arrayIndex && opt.arrayCount > 0)
    {
        *opt.arrayIndex = (*opt.arrayIndex + delta + opt.arrayCount) % opt.arrayCount;
        if (opt.callback)
            opt.callback(0);
    }
    else if (opt.type == OPT_INCREMENT && opt.incValue)
    {
        *opt.incValue += opt.incStep * (float)delta;
        if (*opt.incValue < opt.incMin)
            *opt.incValue = opt.incMin;
        if (*opt.incValue > opt.incMax)
            *opt.incValue = opt.incMax;
        if (opt.callback)
            opt.callback(0);
    }
}

void menu::Select(int localClientNum)
{
    if (s_cursor < 0 || s_cursor >= (int)s_options.size())
        return;

    MenuOption &opt = s_options[s_cursor];

    if (opt.type == OPT_TOGGLE && opt.toggleValue)
    {
        *opt.toggleValue = !(*opt.toggleValue);
        if (opt.callback)
            opt.callback(localClientNum);
        return;
    }

    if (opt.callback)
        opt.callback(localClientNum);
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

    if (s_open)
        s_cursor = 0;
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

    if (!s_open)
        return;

    if (pressed & CMD_BUTTON_UP)
        Select(localClientNum);

    if (pressed & CMD_BUTTON_MELEE)
        Back();

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
    if (!s_open)
        return;
    if (!s_whiteMaterial)
        s_whiteMaterial = Material_RegisterHandle("white");
    if (!s_font)
        s_font = R_RegisterFont("fonts/boldfont");
    if (!s_whiteMaterial || !s_font)
        return;

    const float kMenuMarginRight = -400.0f;
    const float kMenuMarginTop = 175.0f;
    const float kMenuWidth = 350.0f;
    const float kTextScale = 0.4f;
    const float headerHeight = 26.0f;
    const float rowHeight = 26.0f;
    const float paddingX = 6.0f;
    const float chevronWidth = 14.0f;
    const float kScrollBarWidth = 3.0f;
    const float kScrollBarGap = 4.0f;
    const int kOptionLimit = 6;

    const float toggleBoxSize = 10.0f;
    const float sliderWidth = 50.0f;
    const float sliderKnobSize = 8.0f;

    float width = kMenuWidth;
    const ScreenPlacement *scrPlace = ScrPlace_GetFullPlacement();
    float x = scrPlace->virtualViewableMax[0] - width - kMenuMarginRight;
    float y = scrPlace->virtualViewableMin[1] + kMenuMarginTop;

    R_AddCmdDrawStretchPic(x, y, width, headerHeight, 0, 0, 1, 1, kColorRowBg, s_whiteMaterial);
    R_AddCmdDrawText("\\o/", 0x7FFFFFFF, s_font, x + paddingX, y + headerHeight - 8.0f, kTextScale, kTextScale, 0.0f,
                      kColorTitle, 0);

    R_AddCmdDrawStretchPic(x, y + headerHeight, width, 1.5f, 0, 0, 1, 1, kColorDivider, s_whiteMaterial);

    int total = (int)s_options.size();
    int limit = (total < kOptionLimit) ? total : kOptionLimit;

    int start = 0;
    if (total > kOptionLimit)
    {
        start = s_cursor - (kOptionLimit - 1) / 2;
        if (start < 0)
            start = 0;
        if (start > total - kOptionLimit)
            start = total - kOptionLimit;
    }

    float bodyHeight = rowHeight * (float)limit;

    for (int i = 0; i < limit; ++i)
    {
        int index = start + i;
        MenuOption &opt = s_options[index];
        float rowY = y + headerHeight + (rowHeight * (float)i);
        bool selected = (index == s_cursor);

        R_AddCmdDrawStretchPic(x, rowY, width, rowHeight, 0, 0, 1, 1, kColorRowBg, s_whiteMaterial);
        if (selected)
            R_AddCmdDrawStretchPic(x, rowY, width, rowHeight, 0, 0, 1, 1, kColorSelectedBg, s_whiteMaterial);

        const float *textColor = selected ? kColorTextSelected : kColorTextNormal;
        float textY = rowY + rowHeight - 8.0f;

        R_AddCmdDrawText(opt.label, 0x7FFFFFFF, s_font, x + paddingX, textY, kTextScale, kTextScale, 0.0f, textColor,
                          0);

        if (opt.type == OPT_ACTION)
        {
            R_AddCmdDrawText(">", 0x7FFFFFFF, s_font, x + width - chevronWidth, textY, kTextScale, kTextScale, 0.0f,
                              textColor, 0);
        }
        else if (opt.type == OPT_TOGGLE)
		{
			bool on = opt.toggleValue && *opt.toggleValue;

			const float outerSize = 14.0f;
			const float innerSize = 8.0f;
			static const float kColorToggleBorder[4] = {1.0f, 1.0f, 1.0f, 0.35f};

			float outerX = x + width - paddingX - outerSize;
			float outerY = rowY + (rowHeight - outerSize) * 0.5f;

			R_AddCmdDrawStretchPic(outerX, outerY, outerSize, outerSize, 0, 0, 1, 1, kColorToggleBorder, s_whiteMaterial);

			float innerX = outerX + (outerSize - innerSize) * 0.5f;
			float innerY = outerY + (outerSize - innerSize) * 0.5f;

			R_AddCmdDrawStretchPic(innerX, innerY, innerSize, innerSize, 0, 0, 1, 1,
									on ? kColorToggleOn : kColorToggleOff, s_whiteMaterial);
		}
		else if (opt.type == OPT_ARRAY)
		{
			char buf[64];
			const char *value = (opt.arrayIndex && opt.arrayValues && opt.arrayCount > 0)
									 ? opt.arrayValues[*opt.arrayIndex]
									 : "NULL";
			sprintf(buf, "[%s]", value);
			int len = (int)strlen(buf);
			int textW = R_TextWidth(buf, len, s_font);
			R_AddCmdDrawText(buf, len, s_font, x + width - paddingX - (float)textW, textY, kTextScale, kTextScale,
							  0.0f, textColor, 0);
		}
		else if (opt.type == OPT_INCREMENT)
		{
			const float trackHeight = 14.0f;
			const float tickWidth = 2.0f;
			const float tickHeight = 12.0f;

			float barX = x + width - paddingX - sliderWidth;
			float barY = rowY + (rowHeight - trackHeight) * 0.5f;

			R_AddCmdDrawStretchPic(barX, barY, sliderWidth, trackHeight, 0, 0, 1, 1, kColorSliderTrack, s_whiteMaterial);

			float t = 0.0f;
			if (opt.incValue && (opt.incMax - opt.incMin) != 0.0f)
				t = (*opt.incValue - opt.incMin) / (opt.incMax - opt.incMin);
			if (t < 0.0f)
				t = 0.0f;
			if (t > 1.0f)
				t = 1.0f;

			float tickX = barX + (sliderWidth - tickWidth) * t;
			float tickY = rowY + (rowHeight - tickHeight) * 0.5f;

			R_AddCmdDrawStretchPic(tickX, tickY, tickWidth, tickHeight, 0, 0, 1, 1, kColorSliderKnob, s_whiteMaterial);
		}
    }

    if (total > limit)
    {
        float thumbHeight = bodyHeight * ((float)limit / (float)total);
        if (thumbHeight < 4.0f)
            thumbHeight = 4.0f;

        float trackTravel = bodyHeight - thumbHeight;
        float scrollFraction = (total > 1) ? ((float)s_cursor / (float)(total - 1)) : 0.0f;
        float thumbOffset = trackTravel * scrollFraction;

        float scrollBarX = x + width + kScrollBarGap;

        R_AddCmdDrawStretchPic(scrollBarX, y + headerHeight, kScrollBarWidth, bodyHeight, 0, 0, 1, 1,
                                kColorScrollTrack, s_whiteMaterial);
        R_AddCmdDrawStretchPic(scrollBarX, y + headerHeight + thumbOffset, kScrollBarWidth, thumbHeight, 0, 0, 1, 1,
                                kColorScrollBar, s_whiteMaterial);
    }
}

}
}