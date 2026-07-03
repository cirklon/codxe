#include "pch.h"
#include "patches.h"
#include "events.h"

namespace iw4
{
namespace mp_tu6
{

static bool s_arrowsSwapped = false;

void DisableFastfileAuth()
{
    // The game requires fastfiles to be signed in MP, but it has the code to load
    // unsigned fastfiles in the executable. Disable the auth check gate.
    *(volatile uint32_t *)0x821B0978 = 0x60000000;
}

void EnableBouncing()
{
    // Re-enable bouncing
    // https://xoxor4d.github.io/research/mw2-bounce/
    // PM_StepSlideMove
    // .text:8210CB70                 cmpwi     cr6, r23, 0
    // .text:8210CB74                 bne       cr6, loc_8210CB24
    *(volatile uint32_t *)0x8210CB70 = 0x60000000;
    *(volatile uint32_t *)0x8210CB74 = 0x60000000;
}

void DisableDvarProtection()
{
    // Read-only
    // .text:8230D680                 b         __restgprlr_27
    *(volatile uint32_t *)0x8230D680 = 0x60000000;

    // Write-protected
    // .text:8230D6A8                 b         __restgprlr_27
    *(volatile uint32_t *)0x8230D6A8 = 0x60000000;

    // Cheat-protected
    // .text:8230D6EC                 b         __restgprlr_27
    *(volatile uint32_t *)0x8230D6EC = 0x60000000;

    // default migration_dvarErrors off to prevent console spam
    *(volatile uint8_t *)0x822828E7 = 0x0;

    // default loc_warnings off to prevent console spam
    *(volatile uint8_t *)0x822CBDEB = 0x0;
}

void ReplaceGreenArrows()
{
    auto image = "compassping_green_mp";
    auto replacement = "compassping_blue_mp";
    auto imageXAsset = DB_FindXAssetHeader(XAssetType::ASSET_TYPE_IMAGE, image);
    auto replacementXAsset = DB_FindXAssetHeader(XAssetType::ASSET_TYPE_IMAGE, replacement);
    if (!imageXAsset.image || !replacementXAsset.image)
        return;
    char* originalName = imageXAsset.image->Name;
    *imageXAsset.image = *replacementXAsset.image;
    imageXAsset.image->Name = originalName;
}

void ReplaceGreenHollowArrows()
{
    auto image = "compassping_green_hollow_mp";
    auto replacement = "compassping_blue_hollow_mp";
    auto imageXAsset = DB_FindXAssetHeader(XAssetType::ASSET_TYPE_IMAGE, image);
    auto replacementXAsset = DB_FindXAssetHeader(XAssetType::ASSET_TYPE_IMAGE, replacement);
    if (!imageXAsset.image || !replacementXAsset.image)
        return;
    char* originalName = imageXAsset.image->Name;
    *imageXAsset.image = *replacementXAsset.image;
    imageXAsset.image->Name = originalName;
}

void TrySwapArrowsOnce()
{
    if (s_arrowsSwapped)
        return;

    // Confirm the DB is actually ready before touching it
    auto test = DB_FindXAssetHeader(XAssetType::ASSET_TYPE_IMAGE, "compassping_green_mp");
    if (!test.image)
        return; // not loaded yet, will retry next frame

    ReplaceGreenArrows();
    ReplaceGreenHollowArrows();
    s_arrowsSwapped = true;
}

patches::patches()
{
    DisableFastfileAuth();
    //EnableBouncing();
    DisableDvarProtection();
	Events::OnCG_DrawActive(TrySwapArrowsOnce);
}

patches::~patches()
{
}
} // namespace mp_tu6
} // namespace iw4
