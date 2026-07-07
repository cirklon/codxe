#pragma once
#include "pch.h"

namespace iw4
{
namespace mp_tu6
{

// Intercepts DB_FindXAssetHeader for ASSET_TYPE_IMAGE and serves a raw
// override file from <mod base path>\images\<name>.dds when one exists,
// falling back to the normal fastfile-backed asset otherwise. Mirrors
// scr_parser's override-then-fallback pattern for scripts.
class image_loader : public Module
{
  public:
    image_loader();
    ~image_loader();
};

} // namespace mp_tu6
} // namespace iw4