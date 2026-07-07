#include "pch.h"
#include "image_loader.h"
#include <unordered_map>
#include <string>

namespace iw4
{
namespace mp_tu6
{

Detour DB_FindXAssetHeader_Detour;

// Cache of images we've already built from an override file, keyed by the
// asset name the engine asked for. DB_FindXAssetHeader gets called very
// frequently (every material bind, potentially every frame) so we do not
// want to touch disk or rebuild a texture on every lookup - only on the
// first request for a given name.
static std::unordered_map<std::string, GfxImage *> g_overrideImageCache;

static std::string BuildImageOverridePath(const char *name)
{
    std::string modBasePath = Config::GetModBasePath();
    if (modBasePath.empty())
        return std::string();

    std::string safeName(name);
    for (size_t i = 0; i < safeName.size(); ++i)
    {
        if (safeName[i] == '/')
            safeName[i] = '\\';
    }

    // Convention: game:\_codxe\mods\<mod>\images\<name>.dds
    return modBasePath + "\\images\\" + safeName + ".dds";
}

// ---------------------------------------------------------------------------
// THE MISSING PIECE.
//
// GScr_ReplaceImage works by copying one *already-loaded* GfxImage struct
// over another - both sides were already built by the fastfile loader at
// startup, complete with a live Xenos GPU texture behind them. What we need
// here is different: turning raw DDS bytes read off disk, at runtime, into
// a real sampleable GfxImage that didn't come from a fastfile at all.
//
// On Xbox 360 that means either:
//
//   (a) finding and calling the engine's own "build image from parsed
//       data" routine - the same internal function the fastfile loader
//       calls when it deserializes an IMAGE asset. This is the safe path:
//       let the engine do the Xenos tiling and GPU resource creation the
//       way it already knows how to, and we just hand it bytes. We don't
//       have this function's address yet.
//
//   (b) hand-rolling it ourselves: swizzling the DXT data into Xenos tile
//       order and calling the raw D3D CreateTexture-equivalent directly.
//       This needs the live D3D device pointer plus the exact GfxImage
//       field layout (width/height/format/mip pointers/texture handle)
//       from structs.h, and is much easier to get subtly wrong in a way
//       that only shows up as a corrupted or crashing texture later.
//
// Returning nullptr for now rather than guessing at either - see below.
// ---------------------------------------------------------------------------
static GfxImage *BuildImageFromDDS(const std::string &path)
{
    (void)path;
    return nullptr;
}

XAssetHeader DB_FindXAssetHeader_Hook(XAssetType type, const char *name)
{
    if (type == XAssetType::ASSET_TYPE_IMAGE && name)
    {
        auto cached = g_overrideImageCache.find(name);
        if (cached != g_overrideImageCache.end())
        {
            XAssetHeader header;
            header.image = cached->second;
            return header;
        }

        std::string path = BuildImageOverridePath(name);
        if (!path.empty())
        {
            FILE *file = fopen(path.c_str(), "rb");
            if (file)
            {
                fclose(file);

                GfxImage *built = BuildImageFromDDS(path);
                if (built)
                {
                    g_overrideImageCache[name] = built;
                    DbgPrint("image_loader: Loaded override image: %s\n", path.c_str());

                    XAssetHeader header;
                    header.image = built;
                    return header;
                }
            }
        }
    }

    return DB_FindXAssetHeader_Detour.GetOriginal<decltype(DB_FindXAssetHeader)>()(type, name);
}

image_loader::image_loader()
{
    DB_FindXAssetHeader_Detour = Detour(DB_FindXAssetHeader, DB_FindXAssetHeader_Hook);
    DB_FindXAssetHeader_Detour.Install();
}

image_loader::~image_loader()
{
    DB_FindXAssetHeader_Detour.Remove();
}

} // namespace mp_tu6
} // namespace iw4