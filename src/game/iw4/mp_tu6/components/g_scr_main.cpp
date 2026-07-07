#include "pch.h"
#include "g_scr_main.h"
#include "events.h"

namespace iw4
{
namespace mp_tu6
{
std::vector<BuiltinFunctionDef *> scr_functions;
std::vector<BuiltinMethodDef *> scr_methods;
static FILE *open_script_io_file_handle;
const int MAX_SCRIPT_STRING_BYTES = 65535;

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

BuiltinFunction Scr_GetFunction_Hook(const char **pName, scr_builtin_type_t *type)
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
    else
    {
        for (size_t i = 0; i < scr_functions.size(); ++i)
        {
            Scr_RegisterFunction(reinterpret_cast<int>(scr_functions[i]->actionFunc), scr_functions[i]->actionString);
        }
    }

    return Scr_GetFunction_Detour.GetOriginal<decltype(Scr_GetFunction)>()(pName, type);
}

Detour Scr_GetMethod_Detour;

BuiltinMethod Scr_GetMethod_Hook(const char **pName, scr_builtin_type_t *type)
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
    else
    {
        for (size_t i = 0; i < scr_methods.size(); ++i)
        {
            Scr_RegisterFunction(reinterpret_cast<int>(scr_methods[i]->actionFunc), scr_methods[i]->actionString);
        }
    }

    return Scr_GetMethod_Detour.GetOriginal<decltype(Scr_GetMethod)>()(pName, type);
}

static std::string BuildScriptFilePath(const char *filename)
{
    if ((filename[0] && filename[1] == ':') || strncmp(filename, "game:\\", 6) == 0)
        return filename;

    std::string rel(filename);
    for (size_t i = 0; i < rel.size(); ++i)
    {
        if (rel[i] == '/')
            rel[i] = '\\';
    }

    const std::string base = Config::GetModBasePath();
    if (base.empty())
        return rel;

    return base + "\\" + rel;
}

static void EnsureParentDirectory(const std::string &path)
{
    char dirpath[MAX_PATH];
    strncpy(dirpath, path.c_str(), sizeof(dirpath) - 1);
    dirpath[sizeof(dirpath) - 1] = '\0';

    char *last_slash = strrchr(dirpath, '\\');
    if (last_slash)
    {
        *last_slash = '\0';
        filesystem::create_nested_dirs(dirpath);
    }
}

void GScr_CbufAddText()
{
    if (Scr_GetNumParam() != 1)
        Scr_Error("Usage: exec(<string>)\n");

    // VM strings are null-terminated, so no need to manually terminate
    // the string here.
    const char *text = Scr_GetString(0);
    Cbuf_AddText(0, text);
}

static void GScr_PrintConsole()
{
    if (Scr_GetNumParam() != 1)
        Scr_Error("Usage: printconsole(<string>)");

    const char *text = Scr_GetString(0);
    Com_Printf(0, "%s", text);
}

static void GScr_FileWrite()
{
    if (Scr_GetNumParam() != 3)
        Scr_Error("Usage: filewrite(<file>, <contents>, <mode>)");

    const char *filename = Scr_GetString(0);
    const char *contents = Scr_GetString(1);
    const char *mode = Scr_GetString(2);

    const char *file_mode = nullptr;
    if (!_stricmp(mode, "write"))
        file_mode = "wb";
    else if (!_stricmp(mode, "append"))
        file_mode = "ab";
    else
        Scr_ParamError(2, "filewrite: mode must be \"write\" or \"append\"");

    const std::string path = BuildScriptFilePath(filename);
    EnsureParentDirectory(path);

    FILE *file = fopen(path.c_str(), file_mode);
    if (!file)
    {
        Scr_AddInt(0);
        return;
    }

    const bool ok = fwrite(contents, 1, strlen(contents), file) == strlen(contents);
    fclose(file);

    Scr_AddInt(ok ? 1 : 0);
}

static void GScr_FileRead()
{
    if (Scr_GetNumParam() != 1)
        Scr_Error("Usage: fileread(<file>)");

    const std::string path = BuildScriptFilePath(Scr_GetString(0));
    FILE *file = fopen(path.c_str(), "rb");
    if (!file)
    {
        Scr_AddUndefined();
        return;
    }

    fseek(file, 0, SEEK_END);
    const long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size < 0)
    {
        fclose(file);
        Scr_AddUndefined();
        return;
    }

    const size_t file_size_bytes = static_cast<size_t>(file_size);
    const size_t bytes_to_read = file_size_bytes < MAX_SCRIPT_STRING_BYTES ? file_size_bytes : MAX_SCRIPT_STRING_BYTES;
    std::vector<char> buffer(bytes_to_read + 1);

    const size_t bytes_read = fread(buffer.data(), 1, bytes_to_read, file);
    fclose(file);

    if (bytes_read != bytes_to_read)
    {
        Scr_AddUndefined();
        return;
    }

    buffer[bytes_read] = '\0';
    Scr_AddString(buffer.data());
}

static void GScr_FileExists()
{
    if (Scr_GetNumParam() != 1)
        Scr_Error("Usage: fileexists(<file>)");

    const std::string path = BuildScriptFilePath(Scr_GetString(0));
    FILE *file = fopen(path.c_str(), "rb");
    if (!file)
    {
        Scr_AddInt(0);
        return;
    }

    fclose(file);
    Scr_AddInt(1);
}

static void GScr_OpenFile()
{
    if (Scr_GetNumParam() != 2)
        Scr_Error("Usage: openfile(<file>, <mode>)");

    const char *filename = Scr_GetString(0);
    const char *mode = Scr_GetString(1);

    if (_stricmp(mode, "read"))
    {
        Scr_AddInt(-1);
        return;
    }

    if (open_script_io_file_handle)
    {
        Scr_AddInt(-1);
        return;
    }

    const std::string path = BuildScriptFilePath(filename);
    open_script_io_file_handle = fopen(path.c_str(), "r");
    if (!open_script_io_file_handle)
    {
        Scr_AddInt(-1);
        return;
    }

    Scr_AddInt(1);
}

static void GScr_ReadStream()
{
    if (Scr_GetNumParam() != 0)
        Scr_Error("Usage: readstream()");

    if (!open_script_io_file_handle)
    {
        Scr_AddUndefined();
        return;
    }

    char line[4096];
    if (!fgets(line, sizeof(line), open_script_io_file_handle))
    {
        Scr_AddUndefined();
        return;
    }

    Scr_AddString(line);
}

static void CloseScriptIOFile()
{
    if (open_script_io_file_handle)
    {
        fclose(open_script_io_file_handle);
        open_script_io_file_handle = nullptr;
    }
}

static void GScr_CloseFile()
{
    if (Scr_GetNumParam() != 0)
        Scr_Error("Usage: closefile()");

    if (!open_script_io_file_handle)
    {
        Scr_AddInt(-1);
        return;
    }

    const int result = fclose(open_script_io_file_handle);
    open_script_io_file_handle = nullptr;
    Scr_AddInt(result);
}

static void GScr_setGrenadeTimeLeft(scr_entref_t entref)
{
	auto ent = GetPlayerEntity(entref);
	ent->client->ps.grenadeTimeLeft = Scr_GetInt(0);
}

static void GScr_doInstashoots(scr_entref_t entref)
{
	auto ent = GetPlayerEntity(entref);
	ent->client->ps.weapState[0].weaponDelay = 0;
	ent->client->ps.weapState[0].weaponTime = 0;
	ent->client->ps.weapState[0].weaponRestrictKickTime = 1;
	ent->client->ps.weapState[0].weaponState = 0x0;
	ent->client->ps.weapState[1].weaponDelay = 0;
	ent->client->ps.weapState[1].weaponTime = 0;
	ent->client->ps.weapState[1].weaponRestrictKickTime = 1;
	ent->client->ps.weapState[1].weaponState = 0x0;
}

static void GScr_setWeapAnim(scr_entref_t entref)
{
	auto ent = GetPlayerEntity(entref);
	ent->client->ps.weapState[0].weapAnim = Scr_GetInt(0);
	ent->client->ps.weapState[1].weapAnim = Scr_GetInt(0);
}

static void GScr_smoothAction(scr_entref_t entref)
{
	auto ent = GetPlayerEntity(entref);
	ent->client->ps.weapState[0].weapAnim = 0x1;
	ent->client->ps.weapState[1].weapAnim = 0x1;

	ent->client->ps.weapState[0].weaponDelay = 0;
	ent->client->ps.weapState[0].weaponTime = 0;
	ent->client->ps.weapState[0].weaponRestrictKickTime = 0;
	ent->client->ps.weapState[1].weaponDelay = 0;
	ent->client->ps.weapState[1].weaponTime = 0;
	ent->client->ps.weapState[1].weaponRestrictKickTime = 0;
}

static void GScr_Float()
{
	Scr_AddFloat(1.0f * Scr_GetInt(0));
}

static void GScr_SetMemoryInt()
{
    uint32_t addr = ((uint32_t)Scr_GetInt(1) << 16) | ((uint32_t)Scr_GetInt(2) & 0xFFFF);
    uint32_t val  = ((uint32_t)Scr_GetInt(3) << 16) | ((uint32_t)Scr_GetInt(4) & 0xFFFF);

    switch (Scr_GetInt(0))
    {
    case 0:
        *(volatile uint32_t *)addr = val;
        break;
    case 1:
        *(volatile uint8_t *)addr = (uint8_t)val;
        break;
    }
}

static void GScr_SetMemoryString()
{
	auto address = (uintptr_t)Scr_GetInt(0);
    auto str = Scr_GetString(1);
    std::string xstr(str);
    size_t length = xstr.length();

    for (size_t i = 0; i < length; i++)
    {
        *(volatile uint8_t *)address = (uint8_t)str[i];
        address++;
    }

    int space = 32 - (int)length;
    for (int i = 0; i < space; i++)
    {
        *(volatile uint8_t *)address = 0x0;
        address++;
    }
}

static void GScr_depatchElevator()
{
    switch (Scr_GetInt(0))
    {
    case 0:
        *(volatile uint16_t *)0x8210A2F0 = 0x419A;
        break;
    case 1:
        *(volatile uint16_t *)0x8210A2F0 = 0x4800;
        break;
    }
}

static void GScr_ReplaceImage()
{
	auto image = Scr_GetString(0);
	auto replacement = Scr_GetString(1);

	auto imageXAsset = DB_FindXAssetHeader(XAssetType::ASSET_TYPE_IMAGE, image);
	auto replacementXAsset = DB_FindXAssetHeader(XAssetType::ASSET_TYPE_IMAGE, replacement);

	if (!imageXAsset.image || !replacementXAsset.image)
        return;

    char* originalName = imageXAsset.image->Name;

    *imageXAsset.image = *replacementXAsset.image;

    imageXAsset.image->Name = originalName;
}

static bool IsMappedPointer(uint32_t ptr)
{
    return ((ptr >= 0x82000000 && ptr <= 0x83FFFFFF) ||
            (ptr >= 0xA0000000 && ptr <= 0xBFFFFFFF));
}

static void DumpMemory(const char* title, const void* address, size_t size)
{
    const unsigned char* raw = (const unsigned char*)address;

    DbgPrint("----- %s @ %08X -----",
        title,
        (unsigned int)(uintptr_t)address);

    for (size_t row = 0; row < size; row += 16)
    {
        const uint32_t* d = (const uint32_t*)(raw + row);

        DbgPrint(
            "+%02X | %08X %08X %08X %08X | "
            "%02X %02X %02X %02X "
            "%02X %02X %02X %02X "
            "%02X %02X %02X %02X "
            "%02X %02X %02X %02X",

            (unsigned int)row,

            d[0], d[1], d[2], d[3],

            raw[row+0], raw[row+1], raw[row+2], raw[row+3],
            raw[row+4], raw[row+5], raw[row+6], raw[row+7],
            raw[row+8], raw[row+9], raw[row+10], raw[row+11],
            raw[row+12], raw[row+13], raw[row+14], raw[row+15]);
    }
}

static void DumpAssetPointer(uint32_t ptr)
{
    DbgPrint("");
    DbgPrint("Pointer @ +0x6C = %08X", ptr);

    if (!IsMappedPointer(ptr))
    {
        DbgPrint("Pointer is outside mapped memory.");
        return;
    }

    const char* str = (const char*)ptr;

    DbgPrint("String: \"%s\"", str);

    size_t len = strlen(str);

    DumpMemory(
        "Asset Header",
        (const void*)ptr,
        len + 0x80);
}

static void DumpImageRaw(const char* name)
{
    auto asset = DB_FindXAssetHeader(
        XAssetType::ASSET_TYPE_IMAGE,
        name);

    if (!asset.image)
    {
        DbgPrint("Image '%s' not found.", name);
        return;
    }

    GfxImage* img = asset.image;
    unsigned char* raw = (unsigned char*)img;

    DbgPrint("");
    DbgPrint("========================================");
    DbgPrint("Image '%s' @ %08X",
        name,
        (unsigned int)(uintptr_t)img);

    DumpMemory("GfxImage", raw, 0x70);

    uint32_t assetPtr =
        *(uint32_t*)(raw + 0x6C);

    DumpAssetPointer(assetPtr);

    DbgPrint("");
    DumpMemory("Next Record (+0x70)", raw + 0x70, 0x70);
}

static void CompareImages(
    const char* first,
    const char* second)
{
    auto a =
        DB_FindXAssetHeader(
            XAssetType::ASSET_TYPE_IMAGE,
            first);

    auto b =
        DB_FindXAssetHeader(
            XAssetType::ASSET_TYPE_IMAGE,
            second);

    if (!a.image || !b.image)
    {
        DbgPrint("compareimage: image missing.");
        return;
    }

    unsigned char* A = (unsigned char*)a.image;
    unsigned char* B = (unsigned char*)b.image;

    DbgPrint("");
    DbgPrint("===== Comparing '%s' -> '%s' =====",
        first,
        second);

    for (int off = 0; off < 0x70; off += 4)
    {
        uint32_t av =
            *(uint32_t*)(A + off);

        uint32_t bv =
            *(uint32_t*)(B + off);

        if (av != bv)
        {
            DbgPrint(
                "+0x%02X : %08X -> %08X",
                off,
                av,
                bv);
        }
    }
}

static void GScr_DumpImage()
{
    if (Scr_GetNumParam() != 1)
        Scr_Error("Usage: dumpimage(<image>)");

    DumpImageRaw(Scr_GetString(0));
}

static void GScr_CompareImage()
{
    if (Scr_GetNumParam() != 2)
        Scr_Error("Usage: compareimage(<image1>, <image2>)");

    CompareImages(
        Scr_GetString(0),
        Scr_GetString(1));
}

static void DumpImageAsset(const char* name)
{
    auto asset = DB_FindXAssetHeader(
        XAssetType::ASSET_TYPE_IMAGE,
        name);

    if (!asset.image)
    {
        DbgPrint("Image '%s' not found.", name);
        return;
    }

    uint32_t ptr = *(uint32_t*)((char*)asset.image + 0x6C);

    DumpAssetPointer(ptr);
}

static void GScr_DumpImageAsset()
{
    if (Scr_GetNumParam() != 1)
        Scr_Error("Usage: dumpimageasset(<image>)");

    DumpImageAsset(Scr_GetString(0));
}

g_scr_main::g_scr_main()
{
    Scr_GetFunction_Detour = Detour(Scr_GetFunction, Scr_GetFunction_Hook);
    Scr_GetFunction_Detour.Install();

    Scr_GetMethod_Detour = Detour(Scr_GetMethod, Scr_GetMethod_Hook);
    Scr_GetMethod_Detour.Install();

    Scr_AddFunction("exec", GScr_CbufAddText, BUILTIN_ANY);
    Scr_AddFunction("printconsole", GScr_PrintConsole, BUILTIN_ANY);
    Scr_AddFunction("filewrite", GScr_FileWrite, BUILTIN_ANY);
    Scr_AddFunction("fileread", GScr_FileRead, BUILTIN_ANY);
    Scr_AddFunction("fileexists", GScr_FileExists, BUILTIN_ANY);
    Scr_AddFunction("openfile", GScr_OpenFile, BUILTIN_ANY);
    Scr_AddFunction("readstream", GScr_ReadStream, BUILTIN_ANY);
    Scr_AddFunction("closefile", GScr_CloseFile, BUILTIN_ANY);
	Scr_AddFunction("setmemoryint", GScr_SetMemoryInt, BUILTIN_ANY);
	Scr_AddFunction("setmemorystring", GScr_SetMemoryString, BUILTIN_ANY);
	Scr_AddFunction("depatchelevator", GScr_depatchElevator, BUILTIN_ANY);
	Scr_AddFunction("replaceimage", GScr_ReplaceImage, BUILTIN_ANY);
	Scr_AddFunction("float", GScr_Float, BUILTIN_ANY);
	Scr_AddFunction("dumpimage", GScr_DumpImage, BUILTIN_ANY);
	Scr_AddFunction("dumpimageasset", GScr_DumpImageAsset, BUILTIN_ANY);
	Scr_AddFunction("compareimages", GScr_CompareImage, BUILTIN_ANY);

	Scr_AddMethod("setgrenadetimeleft", GScr_setGrenadeTimeLeft, BUILTIN_ANY);
	Scr_AddMethod("doinstashoots", GScr_doInstashoots, BUILTIN_ANY);
	Scr_AddMethod("setweapanim", GScr_setWeapAnim, BUILTIN_ANY);
	Scr_AddMethod("smoothaction", GScr_smoothAction, BUILTIN_ANY);

    Events::OnVMShutdown(CloseScriptIOFile);
}

g_scr_main::~g_scr_main()
{
    CloseScriptIOFile();

    Scr_GetFunction_Detour.Remove();

    Scr_GetMethod_Detour.Remove();
}
} // namespace mp_tu6
} // namespace iw4
