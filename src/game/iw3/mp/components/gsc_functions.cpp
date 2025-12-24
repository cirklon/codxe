#include "pch.h"
#include "gsc_functions.h"
#include "g_scr_main.h"

namespace iw3
{
namespace mp
{
/**
 * Checks if a 3D point is contained within an axis-aligned bounding box
 */
bool IsPointInBounds(const float mins[3], const float maxs[3], const float point[3])
{
    return (point[0] >= mins[0] && point[0] <= maxs[0]) && (point[1] >= mins[1] && point[1] <= maxs[1]) &&
           (point[2] >= mins[2] && point[2] <= maxs[2]);
}

void GSCrGetPlayerclipBrushesContainingPoint()
{
    float point[3] = {0};
    Scr_GetVector(0, point);

    std::vector<int> brushIndices;
    for (int i = 0; i < cm->numBrushes; ++i)
    {
        auto &brush = cm->brushes[i];
        if (brush.contents & CONTENTS_PLAYERCLIP && IsPointInBounds(brush.mins, brush.maxs, point))
            brushIndices.push_back(i);
    }

    Scr_MakeArray();
    for (size_t i = 0; i < brushIndices.size(); ++i)
    {
        Scr_AddInt(brushIndices[i]);
        Scr_AddArray();
    }
}

void GScr_CbufAddText()
{
    if (Scr_GetNumParam() != 1)
    {
        Scr_Error("Usage: exec(<string>)\n");
    }
    // VM strings are null-terminated, so no need to manually terminate
    // the string here.
    const char *text = Scr_GetString(0);
    Cbuf_AddText(0, text);
}

gsc_functions::gsc_functions()
{
    Scr_AddFunction("exec", GScr_CbufAddText, 0);
    Scr_AddFunction("getplayerclipbrushescontainingpoint", GSCrGetPlayerclipBrushesContainingPoint, 0);
}

gsc_functions::~gsc_functions()
{
}
} // namespace mp
} // namespace iw3
