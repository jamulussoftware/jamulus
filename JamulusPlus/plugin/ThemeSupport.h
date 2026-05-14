#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#else
#include <dlfcn.h>
#endif

struct JamulusTheme
{
    juce::String name = "Default";
    juce::File   directory;

    juce::Image backgroundImage;
    juce::File  backgroundVideoFile;
    juce::Image faderKnobImage;
    juce::Image rotaryKnobImage;

    juce::Colour windowColour   = juce::Colour ( 0xffd7dde2 );
    juce::Colour headerColour   = juce::Colour ( 0xffc3ccd4 );
    juce::Colour mixerColour    = juce::Colour ( 0xffdce1e6 );
    juce::Colour panelColour    = juce::Colour ( 0xffd7dde2 );
    juce::Colour panelAltColour = juce::Colour ( 0xffe7ebee );
    juce::Colour outlineColour  = juce::Colour ( 0xff8c959e );
    juce::Colour textColour     = juce::Colour ( 0xff111417 );
    juce::Colour subTextColour  = juce::Colour ( 0xff4d5660 );
    juce::Colour buttonColour   = juce::Colour ( 0xffcfd6dc );
    juce::Colour accentColour   = juce::Colour ( 0xff00cc66 );
    juce::Colour faderColour    = juce::Colour ( 0xffd8d8d8 );
    juce::Colour knobColour     = juce::Colour ( 0xff8a9299 );
};

#if JUCE_WINDOWS
struct WinVideoReader : public juce::Thread
{
    WinVideoReader() : juce::Thread ( "JamulusThemeVideoDecoder" ) {}

    ~WinVideoReader() override
    {
        signalThreadShouldExit();
        stopThread ( 3000 );
        if ( reader != nullptr )
        {
            reader->Release();
            reader = nullptr;
        }
        if ( mfStarted )
        {
            MFShutdown();
            mfStarted = false;
        }
    }

    bool open ( const juce::File& file )
    {
        if ( FAILED ( MFStartup ( MF_VERSION ) ) )
            return false;
        mfStarted = true;

        IMFAttributes* attrs = nullptr;
        MFCreateAttributes ( &attrs, 1 );
        attrs->SetUINT32 ( MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE );
        const HRESULT hr = MFCreateSourceReaderFromURL ( file.getFullPathName().toWideCharPointer(), attrs, &reader );
        attrs->Release();
        if ( FAILED ( hr ) || reader == nullptr )
            return false;

        IMFMediaType* type = nullptr;
        MFCreateMediaType ( &type );
        type->SetGUID ( MF_MT_MAJOR_TYPE, MFMediaType_Video );
        type->SetGUID ( MF_MT_SUBTYPE, MFVideoFormat_RGB32 );
        reader->SetCurrentMediaType ( (DWORD) MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, type );
        type->Release();

        IMFMediaType* outType = nullptr;
        if ( SUCCEEDED ( reader->GetCurrentMediaType ( (DWORD) MF_SOURCE_READER_FIRST_VIDEO_STREAM, &outType ) ) )
        {
            UINT32 w = 0, h = 0;
            MFGetAttributeSize ( outType, MF_MT_FRAME_SIZE, &w, &h );
            frameWidth  = (int) w;
            frameHeight = (int) h;

            UINT32 num = 0, den = 0;
            MFGetAttributeRatio ( outType, MF_MT_FRAME_RATE, &num, &den );
            if ( num > 0 && den > 0 )
                framePeriodMs = juce::jlimit ( 10, 200, (int) ( 1000.0 * den / num ) );

            outType->Release();
        }

        if ( frameWidth <= 0 || frameHeight <= 0 )
            return false;

        startThread ( juce::Thread::Priority::low );
        return true;
    }

    juce::Image getLatestFrame()
    {
        juce::ScopedLock sl ( frameLock );
        auto frame = pendingFrame;
        pendingFrame = {};
        return frame;
    }

    void run() override
    {
        CoInitializeEx ( nullptr, COINIT_MULTITHREADED );

        while ( !threadShouldExit() )
        {
            if ( eof )
            {
                seekToStart();
                continue;
            }

            auto image = decodeNextFrame();
            if ( image.isValid() )
            {
                juce::ScopedLock sl ( frameLock );
                pendingFrame = std::move ( image );
            }

            wait ( framePeriodMs );
        }

        CoUninitialize();
    }

private:
    IMFSourceReader* reader       = nullptr;
    bool             mfStarted    = false;
    int              frameWidth   = 0;
    int              frameHeight  = 0;
    int              framePeriodMs = 33;
    bool             eof          = false;
    juce::CriticalSection frameLock;
    juce::Image           pendingFrame;

    void seekToStart()
    {
        if ( reader == nullptr )
            return;

        PROPVARIANT pv;
        PropVariantInit ( &pv );
        pv.vt            = VT_I8;
        pv.hVal.QuadPart = 0;
        reader->SetCurrentPosition ( GUID_NULL, pv );
        PropVariantClear ( &pv );
        eof = false;
    }

    juce::Image decodeNextFrame()
    {
        if ( reader == nullptr )
            return {};

        DWORD      streamIdx = 0, flags = 0;
        LONGLONG   ts        = 0;
        IMFSample* sample    = nullptr;

        const HRESULT hr = reader->ReadSample ( (DWORD) MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &streamIdx, &flags, &ts, &sample );
        if ( FAILED ( hr ) || ( flags & MF_SOURCE_READERF_ENDOFSTREAM ) )
        {
            if ( sample != nullptr )
                sample->Release();
            eof = true;
            return {};
        }

        if ( sample == nullptr )
            return {};

        IMFMediaBuffer* buf = nullptr;
        sample->ConvertToContiguousBuffer ( &buf );
        sample->Release();
        if ( buf == nullptr )
            return {};

        BYTE* data = nullptr;
        DWORD maxLen = 0, curLen = 0;
        buf->Lock ( &data, &maxLen, &curLen );

        juce::Image image ( juce::Image::ARGB, frameWidth, frameHeight, false );
        {
            juce::Image::BitmapData bd ( image, juce::Image::BitmapData::writeOnly );
            const size_t srcRowBytes = (size_t) frameWidth * 4;
            for ( int y = 0; y < frameHeight; ++y )
            {
                auto* src = reinterpret_cast<const uint32_t*> ( data + y * srcRowBytes );
                auto* dst = reinterpret_cast<uint32_t*> ( bd.getLinePointer ( y ) );
                for ( int x = 0; x < frameWidth; ++x )
                    dst[x] = src[x] | 0xFF000000u;
            }
        }

        buf->Unlock();
        buf->Release();
        return image;
    }
};
#endif

static juce::File getThisPluginModuleFile()
{
#if JUCE_WINDOWS
    HMODULE hm = nullptr;
    if ( !GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCWSTR> ( &getThisPluginModuleFile ),
                               &hm ) )
        return {};

    wchar_t path[MAX_PATH] = {};
    if ( GetModuleFileNameW ( hm, path, MAX_PATH ) == 0 )
        return {};

    return juce::File ( juce::String ( path ) );
#else
    Dl_info info {};
    if ( dladdr ( reinterpret_cast<void*> ( &getThisPluginModuleFile ), &info ) == 0 || info.dli_fname == nullptr )
        return {};

    return juce::File ( juce::String::fromUTF8 ( info.dli_fname ) );
#endif
}

static juce::File findJamulusThemesDirectory()
{
    juce::Array<juce::File> roots;
    roots.add ( juce::File::getSpecialLocation ( juce::File::currentExecutableFile ).getParentDirectory() );

    const auto moduleFile = getThisPluginModuleFile();
    if ( moduleFile.existsAsFile() )
        roots.addIfNotAlreadyThere ( moduleFile.getParentDirectory() );

    for ( const auto& root : roots )
    {
        auto probe = root;
        for ( int i = 0; i < 8; ++i )
        {
            const auto a = probe.getChildFile ( "themes" );
            const auto b = probe.getChildFile ( "Resources" ).getChildFile ( "themes" );
            const auto c = probe.getParentDirectory().getChildFile ( "Resources" ).getChildFile ( "themes" );

            if ( a.isDirectory() )
                return a;
            if ( b.isDirectory() )
                return b;
            if ( c.isDirectory() )
                return c;

            const auto parent = probe.getParentDirectory();
            if ( parent == probe )
                break;
            probe = parent;
        }
    }

    return {};
}

static juce::StringArray findJamulusThemeNames()
{
    juce::StringArray names;
    names.add ( "Default" );

    const auto themesDir = findJamulusThemesDirectory();
    if ( !themesDir.isDirectory() )
        return names;

    auto dirs = themesDir.findChildFiles ( juce::File::findDirectories, false );
    dirs.sort();

    for ( const auto& dir : dirs )
    {
        if ( dir.getFileName().equalsIgnoreCase ( "Skin Template" ) )
            continue;

        names.addIfNotAlreadyThere ( dir.getFileName() );
    }

    return names;
}

static juce::File findJamulusThemeDirectoryByName ( const juce::String& themeName )
{
    if ( themeName.isEmpty() )
        return {};

    const auto themesDir = findJamulusThemesDirectory();
    if ( !themesDir.isDirectory() )
        return {};

    const auto dir = themesDir.getChildFile ( themeName );
    return dir.isDirectory() ? dir : juce::File();
}

static bool parseJamulusThemeColour ( const juce::String& value, juce::Colour& out )
{
    auto s = value.trim().trimCharactersAtStart ( "#" );
    if ( s.length() == 6 && s.containsOnly ( "0123456789abcdefABCDEF" ) )
    {
        out = juce::Colour::fromString ( "ff" + s );
        return true;
    }

    if ( s.length() == 8 && s.containsOnly ( "0123456789abcdefABCDEF" ) )
    {
        out = juce::Colour::fromString ( s );
        return true;
    }

    return false;
}

static JamulusTheme loadJamulusTheme ( const juce::String& themeName )
{
    JamulusTheme theme;
    theme.name = themeName.isNotEmpty() ? themeName : "Default";

    const auto themeDir = findJamulusThemeDirectoryByName ( theme.name );
    if ( !themeDir.isDirectory() )
        return theme;

    theme.directory = themeDir;

    const auto bgFiles = themeDir.findChildFiles ( juce::File::findFiles, false, "bg.*" );
    if ( !bgFiles.isEmpty() )
        theme.backgroundImage = juce::ImageFileFormat::loadFrom ( bgFiles[0] );

    const auto videoFile = themeDir.getChildFile ( "bg.mp4" );
    if ( videoFile.existsAsFile() )
        theme.backgroundVideoFile = videoFile;

    theme.faderKnobImage  = juce::ImageFileFormat::loadFrom ( themeDir.getChildFile ( "fknob.png" ) );
    theme.rotaryKnobImage = juce::ImageFileFormat::loadFrom ( themeDir.getChildFile ( "rknob.png" ) );

    const auto cfgFile = themeDir.getChildFile ( "skin.cfg" );
    if ( !cfgFile.existsAsFile() )
        return theme;

    const auto lines = juce::StringArray::fromLines ( cfgFile.loadFileAsString() );
    for ( const auto& line : lines )
    {
        const auto trimmed = line.trim();
        if ( trimmed.isEmpty() || trimmed.startsWith ( "#" ) || trimmed.startsWith ( ";" ) )
            continue;

        const auto value = trimmed.fromFirstOccurrenceOf ( ":", false, false ).trim();

        if ( trimmed.startsWithIgnoreCase ( "Window Colour:" ) )
            parseJamulusThemeColour ( value, theme.windowColour );
        else if ( trimmed.startsWithIgnoreCase ( "Header Colour:" ) || trimmed.startsWithIgnoreCase ( "MenuBar Colour:" ) )
            parseJamulusThemeColour ( value, theme.headerColour );
        else if ( trimmed.startsWithIgnoreCase ( "Mixer Colour:" ) )
            parseJamulusThemeColour ( value, theme.mixerColour );
        else if ( trimmed.startsWithIgnoreCase ( "Panel Colour:" ) )
            parseJamulusThemeColour ( value, theme.panelColour );
        else if ( trimmed.startsWithIgnoreCase ( "Panel Alt Colour:" ) )
            parseJamulusThemeColour ( value, theme.panelAltColour );
        else if ( trimmed.startsWithIgnoreCase ( "Outline Colour:" ) )
            parseJamulusThemeColour ( value, theme.outlineColour );
        else if ( trimmed.startsWithIgnoreCase ( "Text Colour:" ) )
            parseJamulusThemeColour ( value, theme.textColour );
        else if ( trimmed.startsWithIgnoreCase ( "Subtext Colour:" ) )
            parseJamulusThemeColour ( value, theme.subTextColour );
        else if ( trimmed.startsWithIgnoreCase ( "Button Colour:" ) )
            parseJamulusThemeColour ( value, theme.buttonColour );
        else if ( trimmed.startsWithIgnoreCase ( "Accent Colour:" ) || trimmed.startsWithIgnoreCase ( "Metronome Colour:" ) )
            parseJamulusThemeColour ( value, theme.accentColour );
        else if ( trimmed.startsWithIgnoreCase ( "Fader Colour:" ) || trimmed.startsWithIgnoreCase ( "Faders:" ) )
            parseJamulusThemeColour ( value, theme.faderColour );
        else if ( trimmed.startsWithIgnoreCase ( "Knob Colour:" ) || trimmed.startsWithIgnoreCase ( "Knobs:" ) )
            parseJamulusThemeColour ( value, theme.knobColour );
    }

    return theme;
}
