#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#if JUCE_WINDOWS
#    define NOMINMAX
#    include <Windows.h>
#    include <objbase.h>
#endif

using namespace juce;

static void printDesc ( const PluginDescription& d )
{
    std::cout << "Name: " << d.name << "\n"
              << "DescriptiveName: " << d.descriptiveName << "\n"
              << "Manufacturer: " << d.manufacturerName << "\n"
              << "Category: " << d.category << "\n"
              << "Format: " << d.pluginFormatName << "\n"
              << "Identifier: " << d.fileOrIdentifier << "\n"
              << "UniqueId: " << d.uniqueId << "\n"
              << "IsInstrument: " << ( d.isInstrument ? "true" : "false" ) << "\n"
              << "NumInputs: " << d.numInputChannels << "\n"
              << "NumOutputs: " << d.numOutputChannels << "\n";
}

static String formatWindowsErrorMessage ( unsigned long error )
{
#if JUCE_WINDOWS
    LPWSTR      messageBuffer = nullptr;
    const DWORD flags         = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD langId        = MAKELANGID ( LANG_NEUTRAL, SUBLANG_DEFAULT );

    const auto size = FormatMessageW ( flags, nullptr, (DWORD) error, langId, (LPWSTR) &messageBuffer, 0, nullptr );

    String message;

    if ( size != 0 && messageBuffer != nullptr )
        message = String ( messageBuffer ).trim();
    else
        message = "(no message)";

    if ( messageBuffer != nullptr )
        LocalFree ( messageBuffer );

    return message;
#else
    (void) error;
    return {};
#endif
}

static void tryLoadLibraryAndPrintError ( const String& modulePath )
{
#if JUCE_WINDOWS
    // Try loading WITHOUT setting DLL directory - the embedded manifest should handle it
    SetLastError ( 0 );
    HMODULE h = LoadLibraryW ( modulePath.toWideCharPointer() );

    if ( h == nullptr )
    {
        const auto err = GetLastError();
        std::cout << "LoadLibraryW (no DLL directory setup): FAILED\n";
        std::cout << "Win32Error: " << (unsigned long) err << "\n";
        std::cout << "Win32Message: " << formatWindowsErrorMessage ( err ) << "\n";
        return;
    }

    std::cout << "LoadLibraryW (no DLL directory setup): OK\n";

    const auto* proc = GetProcAddress ( h, "GetPluginFactory" );
    std::cout << "GetProcAddress(GetPluginFactory): " << ( proc != nullptr ? "OK" : "FAILED" ) << "\n";

    FreeLibrary ( h );
#else
    (void) modulePath;
#endif
}

static StringArray resolveTargets ( const String& input )
{
    const File f = File::createFileWithoutCheckingPath ( input );

    if ( f.existsAsFile() )
        return { f.getFullPathName() };

    if ( !f.isDirectory() )
        return {};

    // If a bundle directory was passed, collect candidate module files under it.
    StringArray       modules;
    DirectoryIterator iter ( f, true, "*.vst3", File::findFiles );
    while ( iter.next() )
        modules.add ( iter.getFile().getFullPathName() );

    return modules;
}

int main ( int argc, char* argv[] )
{
    if ( argc < 2 )
    {
        std::cerr << "Usage: vst3_load_probe <path-to-plugin.vst3>\n";
        return 2;
    }

#if JUCE_WINDOWS
    // Some hosts initialise COM. It shouldn't be required for VST3 in general,
    // but doing this makes the probe's runtime closer to common hosts.
    CoInitializeEx ( nullptr, COINIT_APARTMENTTHREADED );
#endif

    // Ensure JUCE's message manager exists (some plugin creation paths require it).
    MessageManager::getInstance();

    const String target = CharPointer_UTF8 ( argv[1] );
    std::cout << "Target: " << target << "\n";

    const auto resolved = resolveTargets ( target );
    std::cout << "Resolved module candidates: " << resolved.size() << "\n";
    for ( const auto& m : resolved )
        std::cout << "  - " << m << "\n";

    if ( resolved.isEmpty() )
    {
        std::cerr << "No .vst3 module files found at target.\n";
        return 2;
    }

    VST3PluginFormatHeadless format;

    OwnedArray<PluginDescription> found;

    for ( const auto& modulePath : resolved )
    {
        std::cout << "\n=== Scanning module: " << modulePath << " ===\n";
        std::cout << "fileMightContainThisPluginType: " << ( format.fileMightContainThisPluginType ( modulePath ) ? "true" : "false" ) << "\n";

        tryLoadLibraryAndPrintError ( modulePath );

        // NOTE: We're NOT setting SetDllDirectory here anymore.
        // The plugin should have an embedded manifest that enables DLL redirection
        // so Windows can find Qt DLLs in the same directory as the plugin.

        try
        {
            format.findAllTypesForFile ( found, modulePath );
        }
        catch ( ... )
        {
            std::cerr << "Exception while scanning plugin types.\n";
            return 3;
        }

        if ( !found.isEmpty() )
            break;
    }

    std::cout << "\nFound plugin types: " << found.size() << "\n";

    if ( found.isEmpty() )
        return 4;

    AudioPluginFormatManager manager;
    manager.addFormat ( std::make_unique<VST3PluginFormatHeadless>() );

    for ( int i = 0; i < found.size(); ++i )
    {
        std::cout << "\n=== Type " << i << " ===\n";
        printDesc ( *found.getUnchecked ( i ) );

        String                               error;
        std::unique_ptr<AudioPluginInstance> instance;

        try
        {
            instance = manager.createPluginInstance ( *found.getUnchecked ( i ), 48000.0, 512, error );
        }
        catch ( ... )
        {
            std::cout << "CreatePluginInstance: threw exception\n";
            continue;
        }

        if ( instance == nullptr )
        {
            std::cout << "CreatePluginInstance: FAILED\n";
            std::cout << "Error: " << error << "\n";
            continue;
        }

        std::cout << "CreatePluginInstance: OK\n";
        auto desc = instance->getPluginDescription();
        std::cout << "Resolved description:\n";
        printDesc ( desc );

        // Simulate what AudioPluginHost does when you open a plugin
        std::cout << "\n--- Simulating host workflow ---\n";

        // 1. Check if plugin has editor
        std::cout << "hasEditor: " << ( instance->hasEditor() ? "true" : "false" ) << "\n";

        // 2. Prepare to play (this might initialize Qt/Jamulus stuff)
        std::cout << "Calling prepareToPlay(48000, 512)...\n";
        try
        {
            instance->prepareToPlay ( 48000.0, 512 );
            std::cout << "prepareToPlay: OK\n";
        }
        catch ( ... )
        {
            std::cout << "prepareToPlay: threw exception!\n";
        }

        // 3. Try processing a block of audio
        std::cout << "Calling processBlock...\n";
        try
        {
            AudioBuffer<float> buffer ( 2, 512 );
            buffer.clear();
            MidiBuffer midi;
            instance->processBlock ( buffer, midi );
            std::cout << "processBlock: OK\n";
        }
        catch ( ... )
        {
            std::cout << "processBlock: threw exception!\n";
        }

        // 4. Check hasEditor - we can't actually create editors in headless mode
        std::cout << "hasEditor (queried again): " << ( instance->hasEditor() ? "true" : "false" ) << "\n";

        // 5. Process a few more blocks to ensure stability
        std::cout << "Processing 10 more audio blocks...\n";
        try
        {
            AudioBuffer<float> buffer ( 2, 512 );
            MidiBuffer         midi;
            for ( int b = 0; b < 10; ++b )
            {
                buffer.clear();
                instance->processBlock ( buffer, midi );
            }
            std::cout << "Multi-block processing: OK\n";
        }
        catch ( ... )
        {
            std::cout << "Multi-block processing: threw exception!\n";
        }

        instance->releaseResources();
    }

    return 0;
}
