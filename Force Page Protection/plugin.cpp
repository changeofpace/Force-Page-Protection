#include "plugin.h"
#include "memory.h"

static const char* authorName = "changeofpace";
static const char* githubSourceURL = R"(https://github.com/changeofpace/Force-Page-Protection)";
static const char* cmdForcePageProtection = "ForcePageProtection";

static bool cbForcePageProtection(int argc, char* argv[])
{
    if (argc != 3)
    {
        PluginLog("usage: %s address, page_protection.\n", cmdForcePageProtection);
        return false;
    }
    if (DbgIsRunning())
    {
        PluginLog("Error: debuggee must be paused.\n");
        return false;
    }
    const HANDLE hProcess = DbgGetProcessHandle();
    if (!hProcess)
    {
        PluginLog("Error: DbgGetProcessHandle failed.\n");
        return false;
    }

    // Validate page protection arg.
    const DWORD newProtection = DWORD(DbgValFromString(argv[2]));
    if (!memory::IsValidPageProtection(newProtection))
    {
        PluginLog("Error: 0x%X is not a valid page protection.\n", newProtection);
        return false;
    }
    
    // Get the memory region's base address and size.
    const duint requestedAddress = DbgValFromString(argv[1]);
    duint regionSize = 0;
    const duint regionBase = DbgMemFindBaseAddr(requestedAddress, &regionSize);
    if (!(regionBase && regionSize))
    {
        PluginLog("Error: %p is not in a valid memory region.\n", requestedAddress);
        return false;
    }

    bool remapped = false;

    // Try standard virtual protect.
    DWORD oldProtection = 0;
    if (!VirtualProtectEx(hProcess, PVOID(regionBase), regionSize, newProtection, &oldProtection))
    {
        if (!memory::RegionIsMappedView(hProcess, PVOID(regionBase), regionSize))
        {
            PluginLog("Error: %p must be in a memory mapped view.\n", requestedAddress);
            return false;
        }

        // Check if the view was created with SEC_NO_CHANGE allocation type or if the requested
        // protection isn't compatible with the view's initial protection.
        if (!memory::ViewHasProtectedProtection(hProcess, PVOID(regionBase), regionSize, newProtection))
        {
            PluginLog("Error: Unhandled protection scenario. Open an issue on github :): %s.\n", githubSourceURL);
            return false;
        }

        // Attempt to remap the view using the requested protection.
        if (!memory::RemapViewOfSection(hProcess, PVOID(regionBase), regionSize, newProtection))
        {
            PluginLog("Error: failed to remap the view at %p.\n", regionBase);
            return false;
        }
        remapped = true;
    }

    GuiUpdateMemoryView();

    PluginLog("%p (%IX) protection set to 0x%X%s.\n",
              regionBase,
              regionSize,
              newProtection,
              remapped ? " (remapped)" : "");

    return true;
}

void PluginLog(const char* Format, ...)
{
    va_list valist;
    char buf[MAX_STRING_SIZE];
    RtlZeroMemory(buf, MAX_STRING_SIZE);

    _snprintf_s(buf, MAX_STRING_SIZE, _TRUNCATE, "[%s] ", PLUGIN_NAME);

    va_start(valist, Format);
    _vsnprintf_s(buf + strlen(buf), sizeof(buf) - strlen(buf), _TRUNCATE, Format, valist);
    va_end(valist);

    _plugin_logputs(buf);
}

///////////////////////////////////////////////////////////////////////////////
// x64dbg

enum { PLUGIN_MENU_ABOUT };

PLUG_EXPORT void CBMENUENTRY(CBTYPE cbType, PLUG_CB_MENUENTRY* info)
{
    switch (info->hEntry)
    {
    case PLUGIN_MENU_ABOUT:
    {
        const int maxMessageBoxStringSize = 1024;
        char buf[maxMessageBoxStringSize] = "";

        _snprintf_s(buf, maxMessageBoxStringSize, _TRUNCATE, "Author:  %s.\n\nsource code:  %s.",
                    authorName,
                    githubSourceURL);

        MessageBoxA(hwndDlg, buf, "About", 0);
    }
    break;
    }
}

bool pluginInit(PLUG_INITSTRUCT* initStruct)
{
    if (!_plugin_registercommand(pluginHandle, cmdForcePageProtection, cbForcePageProtection, true))
    {
        PluginLog("Failed to register command %s.\n", cmdForcePageProtection);
        return false;
    }
    return true;
}

bool pluginStop()
{
    _plugin_menuclear(hMenu);
    _plugin_unregistercommand(pluginHandle, cmdForcePageProtection);
    return true;
}

void pluginSetup()
{
    _plugin_menuaddentry(hMenu, PLUGIN_MENU_ABOUT, "&About");
}

