#pragma once
#include "structs.h"

namespace t6
{
namespace mp
{
	static auto Load_ScriptParseTreeAsset = reinterpret_cast<void (*)(ScriptParseTree **a1)>(0x8226CD58);
    static auto Hunk_AllocateTempMemoryHigh = reinterpret_cast<void *(*)(int size)>(0x8241AB68);
}
}
