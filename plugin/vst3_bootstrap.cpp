#include <windows.h>
#include <string>

// Define VST3 factory function signature
typedef void* ( *GetFactoryProc )();

// Global handle to the real DLL
static HMODULE g_hRealPlugin = nullptr;

// Helper to get the directory of this module (the bootstrap shim)
static std::wstring GetModuleDirectory()
{
    HMODULE hModule = nullptr;
    if ( GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              (LPCWSTR) &GetModuleDirectory,
                              &hModule ) )
    {
        wchar_t path[MAX_PATH];
        if ( GetModuleFileNameW ( hModule, path, MAX_PATH ) > 0 )
        {
            std::wstring fullPath ( path );
            size_t       lastSlash = fullPath.find_last_of ( L"\\/" );
            if ( lastSlash != std::wstring::npos )
            {
                return fullPath.substr ( 0, lastSlash );
            }
        }
    }
    return L"";
}

// Exported VST3 Factory Function
extern "C" __declspec ( dllexport ) void* GetPluginFactory()
{
    // 1. If already loaded, just forward
    if ( g_hRealPlugin )
    {
        GetFactoryProc realProc = (GetFactoryProc) GetProcAddress ( g_hRealPlugin, "GetPluginFactory" );
        if ( realProc )
        {
            return realProc();
        }
        return nullptr;
    }

    // 2. Determine path to this module
    std::wstring moduleDir = GetModuleDirectory();
    if ( moduleDir.empty() ) {
        return nullptr;
    }

    // 3. Set DLL Directory to this folder so implicit dependencies are found
    // We use AddDllDirectory if available (Win8+) or SetDllDirectory as fallback.
    // However, SetDllDirectory is simpler and usually sufficient for VST plugins.
    // We need to ensure that when JamulusCore.dll loads,
    // it can find its dependencies in the SAME folder.
    SetDllDirectoryW ( moduleDir.c_str() );

    std::wstring corePathVst3 = moduleDir + L"\\JamulusCore.vst3";
    std::wstring corePathDll  = moduleDir + L"\\JamulusCore.dll";

    g_hRealPlugin = LoadLibraryExW ( corePathVst3.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
    if ( !g_hRealPlugin )
        g_hRealPlugin = LoadLibraryExW ( corePathDll.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
    if ( !g_hRealPlugin )
        g_hRealPlugin = LoadLibraryW ( corePathVst3.c_str() );
    if ( !g_hRealPlugin )
        g_hRealPlugin = LoadLibraryW ( corePathDll.c_str() );

    // 6. Restore DLL Directory (optional, but good practice to clear if we don't want to pollute)
    // Actually, for VSTs, keeping it might be necessary if the Core DLL lazy loads things later.
    // But usually LoadLibrary resolves static imports immediately.
    // Let's leave it set for this thread/process context or reset?
    // Resetting it might break lazy loads. Leaving it is safer for the plugin.
    // SetDllDirectoryW(NULL);

    if ( g_hRealPlugin )
    {
        GetFactoryProc realProc = (GetFactoryProc) GetProcAddress ( g_hRealPlugin, "GetPluginFactory" );
        if ( realProc )
        {
            return realProc();
        }
    }

    return nullptr;
}

// Optional: DllMain to handle attach/detach if needed
BOOL WINAPI DllMain ( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
    if ( fdwReason == DLL_PROCESS_DETACH )
    {
        if ( g_hRealPlugin )
        {
            FreeLibrary ( g_hRealPlugin );
            g_hRealPlugin = nullptr;
        }
    }
    return TRUE;
}
// touch
