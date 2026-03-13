#pragma once

#include "structs.h"

namespace qos
{
namespace mp
{
static auto cm = reinterpret_cast<clipMap_t *>(0x839CB3D0);
static auto g_entities = reinterpret_cast<gentity_s *>(0x838B15A0);
static const PlayerKeyState *playerKeys = reinterpret_cast<PlayerKeyState *>(0x82AAA2B0);

static auto Hunk_AllocateTempMemoryHighInternal = reinterpret_cast<void *(*)(int size)>(0x822B89E8);

static auto Scr_AddInt = reinterpret_cast<void (*)(int value)>(0x8227FBC8);
static auto Scr_AddSourceBuffer =
    reinterpret_cast<char *(*)(const char *filename, const char *extFilename, const char *codePos, bool archive)>(
        0x82275DD8);
static auto Scr_GetFunction = reinterpret_cast<BuiltinFunction (*)(const char **pName, int *type)>(0x8223F788);
static auto Scr_GetMethod = reinterpret_cast<BuiltinMethod (*)(const char **pName, int *type)>(0x8223F838);
static auto Scr_GetVector = reinterpret_cast<void (*)(unsigned int index, float *vectorValue)>(0x82284618);
static auto Scr_GetString = reinterpret_cast<char *(*)(unsigned int index)>(0x82284430);

static auto Scr_Error = reinterpret_cast<void (*)(const char *error)>(0x82280180);
static auto Scr_ObjectError = reinterpret_cast<void (*)(const char *error)>(0x822802B8);

static auto va = reinterpret_cast<char *(*)(char *format, ...)>(0x822C1848);

typedef int (*Key_StringToKeynum_t)(const char *str);
static Key_StringToKeynum_t Key_StringToKeynum = reinterpret_cast<Key_StringToKeynum_t>(0x821C2AD0);

} // namespace mp
} // namespace qos
