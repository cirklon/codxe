#include "pch.h"
#include "scr_parser.h"

namespace t6
{
namespace mp
{

Detour Load_ScriptParseTreeAsset_Detour;

void Load_ScriptParseTreeAsset_Hook(ScriptParseTree **a1)
{
    ScriptParseTree *scriptParseTree = *a1;

    if (scriptParseTree && scriptParseTree->name)
    {
        std::string modBasePath = Config::GetModBasePath();
        if (!modBasePath.empty())
        {
            std::string file_path = modBasePath + "\\" + scriptParseTree->name + ".bin";
            std::replace(file_path.begin(), file_path.end(), '/', '\\');

            DbgPrint("GSCLoader: Checking %s\n", file_path.c_str());

            HANDLE file = CreateFileA(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            if (file != INVALID_HANDLE_VALUE)
            {
                DWORD file_size = GetFileSize(file, NULL);
                DbgPrint("GSCLoader: Found file, size=%u\n", file_size);

                if (file_size != INVALID_FILE_SIZE && file_size > 0)
                {
                    char *buffer = new char[file_size];
                    DWORD bytes_read = 0;
                    if (ReadFile(file, buffer, file_size, &bytes_read, NULL) && bytes_read == file_size)
                    {
                        scriptParseTree->buffer = buffer;
                        scriptParseTree->len = file_size;
                        DbgPrint("GSCLoader: Successfully loaded %s (%u bytes)\n",
                                 scriptParseTree->name, file_size);
                    }
                    else
                    {
                        delete[] buffer;
                        DbgPrint("GSCLoader: ReadFile failed, bytes_read=%u expected=%u error=0x%08X\n",
                                 bytes_read, file_size, GetLastError());
                    }
                }
                else
                {
                    DbgPrint("GSCLoader: Invalid file size: %u\n", file_size);
                }

                CloseHandle(file);
            }
            else
            {
                DbgPrint("GSCLoader: Not found (error=0x%08X): %s\n", GetLastError(), file_path.c_str());
            }
        }
        else
        {
            DbgPrint("GSCLoader: No active mod, skipping %s\n", scriptParseTree->name);
        }
    }

    Load_ScriptParseTreeAsset_Detour.GetOriginal<decltype(Load_ScriptParseTreeAsset)>()(a1);
}

scr_parser::scr_parser()
{
    Load_ScriptParseTreeAsset_Detour = Detour(Load_ScriptParseTreeAsset, Load_ScriptParseTreeAsset_Hook);
    Load_ScriptParseTreeAsset_Detour.Install();
    DbgPrint("GSCLoader: hooked Load_ScriptParseTreeAsset.\n");
}

scr_parser::~scr_parser()
{
    Load_ScriptParseTreeAsset_Detour.Remove();
}

} // namespace mp
} // namespace t6