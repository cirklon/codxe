#include "pch.h"
#include "g_scr_main.h"

namespace qos
{
namespace mp
{
std::vector<BuiltinFunctionDef *> scr_functions;
std::vector<BuiltinMethodDef *> scr_methods;

int CL_IsKeyPressed(const int localClientNum, const char *keyName)
{
    const int keynum = Key_StringToKeynum(keyName);
    if (keynum >= 0)
        return playerKeys[0].keys[keynum].down;
    else
        return 0;
}

void PlayerCmd_ButtonPressed(scr_entref_t entref)
{
    GetPlayerEntity(entref);

    const char *button = Scr_GetString(0);
    if (!button || !*button)
        Scr_Error("usage: <client> buttonPressed(<button name>)");

    const int keypressed = CL_IsKeyPressed(0, button);
    return Scr_AddInt(keypressed);
}

void PlayerCmd_ADSButtonPressed(scr_entref_t entref)
{
    const gentity_s *ent = GetPlayerEntity(entref);
    Scr_AddInt(((ent->client->buttonsSinceLastFrame | ent->client->buttons) & CMD_BUTTON_ADS) != 0);
}

void PlayerCmd_JumpButtonPressed(scr_entref_t entref)
{
    const gentity_s *ent = GetPlayerEntity(entref);
    Scr_AddInt(((ent->client->buttonsSinceLastFrame | ent->client->buttons) & CMD_BUTTON_JUMP) != 0);
}

void PlayerCmd_NextFireTypeButtonPressed(scr_entref_t entref)
{
    const gentity_s *ent = GetPlayerEntity(entref);
    Scr_AddInt(((ent->client->buttonsSinceLastFrame | ent->client->buttons) & CMD_BUTTON_NEXTFIRETYPE) != 0);
}

void PlayerCmd_SprintButtonPressed(scr_entref_t entref)
{
    const gentity_s *ent = GetPlayerEntity(entref);
    Scr_AddInt(((ent->client->buttonsSinceLastFrame | ent->client->buttons) & CMD_BUTTON_SPRINT) != 0);
}

void PlayerCmd_SetPlayerVelocity(scr_entref_t entref)
{
    gentity_s *ent = GetPlayerEntity(entref);

    if (Scr_GetNumParam() != 1)
        Scr_Error("Usage: <client> SetVelocity( vec3 )\n");

    float velocity[3] = {0};

    Scr_GetVector(0, velocity);

    ent->client->ps.velocity[0] = velocity[0];
    ent->client->ps.velocity[1] = velocity[1];
    ent->client->ps.velocity[2] = velocity[2];
}

void GScr_CbufAddText()
{
    if (Scr_GetNumParam() != 1)
        Scr_Error("Usage: exec(<string>)\n");

    const char *text = Scr_GetString(0);
    Cbuf_AddText(0, text);
}

void Scr_AddFunction(const char *name, BuiltinFunction func, scr_builtin_type_t type)
{
    BuiltinFunctionDef *newFunc = new BuiltinFunctionDef;
    newFunc->actionString = name;
    newFunc->actionFunc = func;
    newFunc->type = type;
    scr_functions.push_back(newFunc);
}

void Scr_AddMethod(const char *name, BuiltinMethod func, scr_builtin_type_t type)
{
    BuiltinMethodDef *newMethod = new BuiltinMethodDef;
    newMethod->actionString = name;
    newMethod->actionFunc = func;
    newMethod->type = type;
    scr_methods.push_back(newMethod);
}

Detour Scr_GetFunction_Detour;

BuiltinFunction Scr_GetFunction_Hook(const char **pName, int *type)
{
    if (pName != nullptr)
    {
        for (size_t i = 0; i < scr_functions.size(); ++i)
        {
            if (std::strcmp(*pName, scr_functions[i]->actionString) == 0)
            {
                *type = scr_functions[i]->type;
                return scr_functions[i]->actionFunc;
            }
        }
    }

    const BuiltinFunction pFunction = Scr_GetFunction_Detour.GetOriginal<decltype(Scr_GetFunction)>()(pName, type);
    DbgPrint("QOS MP: Scr_GetFunction called for %s, type %d pFunction %p\n", *pName, *type, pFunction);
    return pFunction;
}

Detour Scr_GetMethod_Detour;

BuiltinMethod Scr_GetMethod_Hook(const char **pName, int *type)
{
    if (pName != nullptr)
    {
        for (size_t i = 0; i < scr_methods.size(); ++i)
        {
            if (std::strcmp(*pName, scr_methods[i]->actionString) == 0)
            {
                *type = scr_methods[i]->type;
                return scr_methods[i]->actionFunc;
            }
        }
    }

    const BuiltinMethod pMethod = Scr_GetMethod_Detour.GetOriginal<decltype(Scr_GetMethod)>()(pName, type);
    DbgPrint("QOS MP: Scr_GetMethod called for %s, type %d pMethod %p\n", *pName, *type, pMethod);
    return pMethod;
}

g_scr_main::g_scr_main()
{
    Scr_GetFunction_Detour = Detour(Scr_GetFunction, Scr_GetFunction_Hook);
    Scr_GetFunction_Detour.Install();

    Scr_GetMethod_Detour = Detour(Scr_GetMethod, Scr_GetMethod_Hook);
    Scr_GetMethod_Detour.Install();

    Scr_AddFunction("exec", GScr_CbufAddText, BUILTIN_ANY);

    Scr_AddMethod("buttonpressed", PlayerCmd_ButtonPressed, BUILTIN_ANY); // Only works for host buttons

    Scr_AddMethod("adsbuttonpressed", PlayerCmd_ADSButtonPressed, BUILTIN_ANY);
    Scr_AddMethod("jumpbuttonpressed", PlayerCmd_JumpButtonPressed, BUILTIN_ANY);
    Scr_AddMethod("nextfiretypebuttonpressed", PlayerCmd_NextFireTypeButtonPressed, BUILTIN_ANY);
    Scr_AddMethod("sprintbuttonpressed", PlayerCmd_SprintButtonPressed, BUILTIN_ANY);

    Scr_AddMethod("setplayervelocity", PlayerCmd_SetPlayerVelocity, BUILTIN_ANY);
}

g_scr_main::~g_scr_main()
{
    Scr_GetFunction_Detour.Remove();

    Scr_GetMethod_Detour.Remove();
}
} // namespace mp
} // namespace qos
