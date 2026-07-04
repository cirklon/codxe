#pragma once
#include "pch.h"

namespace iw4
{
namespace mp_tu6
{

enum MenuOptionType
{
    OPT_ACTION,
    OPT_TOGGLE,
    OPT_ARRAY,
    OPT_INCREMENT,
};

struct MenuOption
{
    const char *label;
    void (*callback)(int localClientNum);
    MenuOptionType type;

    bool *toggleValue;

    const char **arrayValues;
    int arrayCount;
    int *arrayIndex;

    float *incValue;
    float incMin;
    float incMax;
    float incStep;
};

enum MenuPage
{
    Main,
    Player,
    Weapons,
};

class menu : public Module
{
  public:
    menu();
    ~menu();

    static Detour s_drawDetour;
    static Detour s_cmdDetour;

    static void Draw(int localClientNum);
    static void ProcessInput(int localClientNum, usercmd_s &cmd);
    static bool IsOpen();

  private:
    static void BuildOptions(MenuPage page);
    static void Navigate(int delta);
    static void Adjust(int delta);
    static void Select(int localClientNum);
    static void Back();
    static void Toggle();

    static MenuOption MakeAction(const char *label, void (*cb)(int));
    static MenuOption MakeToggle(const char *label, bool *value, void (*cb)(int));
    static MenuOption MakeArray(const char *label, const char **values, int count, int *index, void (*cb)(int));
    static MenuOption MakeIncrement(const char *label, float *value, float min, float max, float step,
                                     void (*cb)(int));

    static void BackCallback(int localClientNum);

  private:
    static bool s_open;
    static int s_cursor;
    static MenuPage s_page;
    static std::vector<MenuOption> s_options;
    static std::vector<MenuPage> s_pageStack;
    static Material *s_whiteMaterial;
    static Font_s *s_font;

    static bool s_testToggle;
    static int s_testArrayIndex;
    static float s_testIncrement;
};

} // namespace mp_tu6
} // namespace iw4