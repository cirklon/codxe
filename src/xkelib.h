/**
 * Xbox kernal library.
 *
 * Source https://github.com/jogolden/testdev/tree/master/xkelib
 */
#pragma once

#include "xtl.h"

#define EXPORTNUM(x) // Just for documentation, thx XBMC!

// flags used by ExCreateThread
#define EX_CREATE_FLAG_SUSPENDED 0x1    // thread created suspended
#define EX_CREATE_FLAG_SYSTEM 0x2       // create a system thread
#define EX_CREATE_FLAG_TITLE_EXEC 0x100 // title execution thread

typedef struct _STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING, *PSTRING;

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderLinks;           // 0x0 sz:0x8
    LIST_ENTRY InClosureOrderLinks;        // 0x8 sz:0x8
    LIST_ENTRY InInitializationOrderLinks; // 0x10 sz:0x8
    PVOID NtHeadersBase;                   // 0x18 sz:0x4
    PVOID ImageBase;                       // 0x1C sz:0x4
    DWORD SizeOfNtImage;                   // 0x20 sz:0x4
    UNICODE_STRING FullDllName;            // 0x24 sz:0x8
    UNICODE_STRING BaseDllName;            // 0x2C sz:0x8
    DWORD Flags;                           // 0x34 sz:0x4
    DWORD SizeOfFullImage;                 // 0x38 sz:0x4
    PVOID EntryPoint;                      // 0x3C sz:0x4
    WORD LoadCount;                        // 0x40 sz:0x2
    WORD ModuleIndex;                      // 0x42 sz:0x2
    PVOID DllBaseOriginal;                 // 0x44 sz:0x4
    DWORD CheckSum;                        // 0x48 sz:0x4
    DWORD ModuleLoadFlags;                 // 0x4C sz:0x4
    DWORD TimeDateStamp;                   // 0x50 sz:0x4
    PVOID LoadedImports;                   // 0x54 sz:0x4
    PVOID XexHeaderBase;                   // 0x58 sz:0x4
    union
    {
        STRING LoadFileName; // 0x5C sz:0x8
        struct
        {
            PVOID ClosureRoot;     // 0x5C sz:0x4 LDR_DATA_TABLE_ENTRY
            PVOID TraversalParent; // 0x60 sz:0x4 LDR_DATA_TABLE_ENTRY
        } asEntry;
    } inf;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY; // size 100
C_ASSERT(sizeof(LDR_DATA_TABLE_ENTRY) == 0x64);

extern "C"
{
    NTSYSAPI
    EXPORTNUM(3)
    VOID NTAPI DbgPrint(IN const char *s, ...);

    NTSYSAPI
    EXPORTNUM(463)
    DWORD NTAPI XamGetCurrentTitleId(VOID);

    NTSYSAPI
    EXPORTNUM(13)
    DWORD NTAPI ExCreateThread(IN PHANDLE pHandle, IN DWORD dwStackSize, IN LPDWORD lpThreadId,
                               IN PVOID apiThreadStartup, IN LPTHREAD_START_ROUTINE lpStartAddress,
                               IN LPVOID lpParameter, IN DWORD dwCreationFlagsMod);
}
