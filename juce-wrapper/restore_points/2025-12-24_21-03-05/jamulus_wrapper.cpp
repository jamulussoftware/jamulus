#include "jamulus_wrapper.h"
#include <QString>
#include "../libjamulus/../src/client.h"
#include "../libjamulus/../src/settings.h"
#include "../libjamulus/../src/clientdlg.h"
#include <QApplication>
#include <QLibraryInfo>
#include <QTimer>
#include <new>
#include "audio_fifo.h"
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

#ifdef _WIN32
#include <windows.h>

// Get the directory containing the current DLL (jamulus_wrapper or the plugin)
static QString getPluginDirectory() {
    HMODULE hModule = nullptr;
    // Get handle to this DLL by using the address of this function
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&getPluginDirectory),
        &hModule);
    
    if (hModule) {
        wchar_t path[MAX_PATH];
        if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0) {
            QString fullPath = QString::fromWCharArray(path);
            int lastSlash = fullPath.lastIndexOf('\\');
            if (lastSlash > 0) {
                return fullPath.left(lastSlash);
            }
        }
    }
    return QString();
}

// Set up Qt library paths before QApplication is created
static void setupQtPaths() {
    QString pluginDir = getPluginDirectory();
    if (!pluginDir.isEmpty()) {
        // Tell Qt to look for plugins in the same directory as this DLL
        qputenv("QT_PLUGIN_PATH", pluginDir.toUtf8());
    }
}
#else
static void setupQtPaths() {}
#endif

// Global Qt application instance and event pump timer
static QApplication* g_qtApp = nullptr;
static QTimer* g_eventPumpTimer = nullptr;

// Global GUI instances - forward declare
static CClientSettings* g_settings = nullptr;
static CClientDlg* g_clientDlg = nullptr;

// Ensure QApplication exists and event processing is set up
static void ensureQtApp() {
    static bool s_pathsSetup = false;
    if (!s_pathsSetup) {
        setupQtPaths();
        s_pathsSetup = true;
    }

    if (!QApplication::instance()) {
        static int fakeArgc = 1;
        static char fakeArg0[] = "jamulus_vst3";
        static char* fakeArgv[] = { fakeArg0, nullptr };
        g_qtApp = new QApplication(fakeArgc, fakeArgv);
        
        // Create a timer to pump Qt events since we're not running exec()
        g_eventPumpTimer = new QTimer();
        QObject::connect(g_eventPumpTimer, &QTimer::timeout, []() {
            QApplication::processEvents(QEventLoop::AllEvents, 10);
        });
        g_eventPumpTimer->start(16); // ~60fps event processing
    }
}

// WrapperClient: exposes a bridge to call CClient::ProcessSndCrdAudioData
// from the plugin/audio thread by converting to/from the Jamulus internal
// integer buffer format.
class WrapperClient : public CClient
{
public:
    WrapperClient(const quint16 iPortNumber,
                  const quint16 iQosNumber,
                  const QString& strConnOnStartupAddress,
                  const QString& strMIDISetup,
                  const bool     bNoAutoJackConnect,
                  const QString& strNClientName,
                  const bool     bNEnableIPv6,
                  const bool     bNMuteMeInPersonalMix)
                : CClient(iPortNumber, iQosNumber, strConnOnStartupAddress, strMIDISetup, bNoAutoJackConnect, strNClientName, bNEnableIPv6, bNMuteMeInPersonalMix),
                    inFifo(nullptr), outFifo(nullptr), workerRunning(false)
    {
        // default FIFO capacity: 2 seconds @ 48k -> 96000 frames
        const size_t defaultCapacity = 48000 * 2;
        const int channels = 2;
        inFifo = std::make_unique<AudioFifo>(defaultCapacity, channels);
        outFifo = std::make_unique<AudioFifo>(defaultCapacity, channels);
    }

    // Process interleaved float audio (frames x channels) in-place: input -> output
    // Returns 0 on success.
    int ProcessInterleavedFloat(const float* in, float* out, int frames, int channels)
    {
        if (!in || !out || frames <= 0 || channels <= 0) return -1;

        // Convert floats [-1,1] to int16_t interleaved
        CVector<int16_t> vec;
        vec.Init(frames * channels);
        for (int i = 0; i < frames * channels; ++i)
        {
            float v = in[i];
            if (v > 1.0f) v = 1.0f;
            if (v < -1.0f) v = -1.0f;
            vec[i] = static_cast<int16_t>(v * 32767.0f);
        }

        // Call protected processing path
        ProcessSndCrdAudioData(vec);

        // Convert back to float
        for (int i = 0; i < frames * channels; ++i)
        {
            out[i] = static_cast<float>(vec[i]) / 32767.0f;
        }

        return 0;
    }

    // Start internal processing worker
    void StartWorker(int processBlockFrames = 128)
    {
        if (workerRunning.load()) return;
        workerRunning.store(true);
        blockFrames = processBlockFrames;
        worker = std::thread([this]() {
            std::vector<float> inbuf;
            std::vector<float> outbuf;
            inbuf.resize(blockFrames * 2);
            outbuf.resize(blockFrames * 2);
            while (workerRunning.load())
            {
                size_t got = inFifo->pop(inbuf.data(), blockFrames);
                if (got == 0)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                // process
                ProcessInterleavedFloat(inbuf.data(), outbuf.data(), static_cast<int>(got), 2);
                // push to outFifo
                outFifo->push(outbuf.data(), got);
            }
        });
    }

    void StopWorker()
    {
        if (!workerRunning.load()) return;
        workerRunning.store(false);
        if (worker.joinable()) worker.join();
    }

    // FIFO access for async API
    AudioFifo* GetInputFifo() { return inFifo.get(); }
    AudioFifo* GetOutputFifo() { return outFifo.get(); }

private:
    std::unique_ptr<AudioFifo> inFifo;
    std::unique_ptr<AudioFifo> outFifo;
    std::thread worker;
    std::atomic<bool> workerRunning;
    int blockFrames{128};
};

extern "C" {

jamulus_client_t jamulus_client_create(unsigned short port, const char* server_addr, const char* client_name, bool enable_ipv6)
{
    // Ensure QApplication exists for Qt widgets
    ensureQtApp();

    // Minimal parameters: qos=0, empty MIDI setup, no auto jack connect, not muted
    // Allocate without throwing; return null on allocation failure.
    WrapperClient* c = new (std::nothrow) WrapperClient((quint16)port,
                             (quint16)0,
                             QString(server_addr ? server_addr : QString()),
                             QString(""),
                             true,
                             QString(client_name ? client_name : "jamulus_wrapper"),
                             enable_ipv6,
                             false);
    if (!c) return nullptr;
    return reinterpret_cast<jamulus_client_t>(c);
}

void jamulus_client_destroy(jamulus_client_t c)
{
    if (!c) return;
    
    // If this client is associated with the GUI, clean up GUI first
    WrapperClient* wc = reinterpret_cast<WrapperClient*>(c);
    if (g_clientDlg) {
        // Hide and delete the dialog before deleting the client it references
        g_clientDlg->hide();
        delete g_clientDlg;
        g_clientDlg = nullptr;
    }
    if (g_settings) {
        delete g_settings;
        g_settings = nullptr;
    }
    
    delete wc;
}

int jamulus_client_start(jamulus_client_t c)
{
    CClient* client = reinterpret_cast<CClient*>(c);
    if (!client) return -1;
    client->Start();
    return 0;
}

int jamulus_client_stop(jamulus_client_t c)
{
    CClient* client = reinterpret_cast<CClient*>(c);
    if (!client) return -1;
    client->Stop();
    return 0;
}

int jamulus_client_set_server_addr(jamulus_client_t c, const char* addr)
{
    CClient* client = reinterpret_cast<CClient*>(c);
    if (!client) return -1;
    bool ok = client->SetServerAddr(QString(addr ? addr : ""));
    return ok ? 0 : -1;
}

int jamulus_client_process_audio(jamulus_client_t c, const float* in, float* out, int frames, int channels)
{
    if (!c) return -1;
    WrapperClient* wc = reinterpret_cast<WrapperClient*>(c);
    return wc->ProcessInterleavedFloat(in, out, frames, channels);
}

int jamulus_client_push_audio(jamulus_client_t c, const float* in, int frames, int channels)
{
    if (!c || !in || frames <= 0) return -1;
    WrapperClient* wc = reinterpret_cast<WrapperClient*>(c);
    AudioFifo* f = wc->GetInputFifo();
    if (!f) return -1;
    bool ok = f->push(in, static_cast<size_t>(frames));
    return ok ? 0 : -1;
}

int jamulus_client_pop_audio(jamulus_client_t c, float* out, int maxFrames, int channels)
{
    if (!c || !out || maxFrames <= 0) return -1;
    WrapperClient* wc = reinterpret_cast<WrapperClient*>(c);
    AudioFifo* f = wc->GetOutputFifo();
    if (!f) return -1;
    size_t got = f->pop(out, static_cast<size_t>(maxFrames));
    return static_cast<int>(got);
}

int jamulus_client_start_worker(jamulus_client_t c, int processBlockFrames)
{
    if (!c) return -1;
    WrapperClient* wc = reinterpret_cast<WrapperClient*>(c);
    wc->StartWorker(processBlockFrames);
    return 0;
}

int jamulus_client_stop_worker(jamulus_client_t c)
{
    if (!c) return -1;
    WrapperClient* wc = reinterpret_cast<WrapperClient*>(c);
    wc->StopWorker();
    return 0;
}

// GUI functions

void* jamulus_gui_create(jamulus_client_t c)
{
    if (!c) return nullptr;
    
    ensureQtApp();
    
    WrapperClient* wc = reinterpret_cast<WrapperClient*>(c);
    
    // Create settings if not exists (needs CClient* and an ini file name)
    if (!g_settings) {
        g_settings = new CClientSettings(wc, QString());  // Empty string = use default ini location
    }
    
    // Create dialog if not exists
    if (!g_clientDlg) {
        g_clientDlg = new CClientDlg(
            wc,           // CClient*
            g_settings,   // CClientSettings*
            QString(),    // strConnOnStartupAddress
            QString(),    // strMIDISetup
            false,        // bShowComplRegConnList
            false,        // bShowAnalyzerConsole
            false,        // bMuteStream
            false,        // bEnableIPv6
            nullptr       // parent
        );
    }
    
    return g_clientDlg;
}

void jamulus_gui_destroy(void* gui)
{
    // Don't actually destroy - keep the singleton alive
    // The dialog will be hidden when the editor closes
    (void)gui;
}

void jamulus_gui_show(void* gui)
{
    if (gui) {
        QWidget* w = reinterpret_cast<QWidget*>(gui);
        w->show();
        w->raise();
        w->activateWindow();
    }
}

void jamulus_gui_hide(void* gui)
{
    if (gui) {
        QWidget* w = reinterpret_cast<QWidget*>(gui);
        w->hide();
    }
}

void* jamulus_gui_get_native_handle(void* gui)
{
    if (!gui) return nullptr;
    QWidget* w = reinterpret_cast<QWidget*>(gui);
    return reinterpret_cast<void*>(w->winId());
}

void jamulus_gui_set_parent(void* gui, void* parentHwnd)
{
#ifdef _WIN32
    if (gui && parentHwnd) {
        QWidget* w = reinterpret_cast<QWidget*>(gui);
        HWND hwnd = reinterpret_cast<HWND>(w->winId());
        HWND parent = reinterpret_cast<HWND>(parentHwnd);
        
        // Make the Qt window a child of the parent HWND
        SetParent(hwnd, parent);
        
        // Change window style to be a child window
        LONG style = GetWindowLong(hwnd, GWL_STYLE);
        style = (style & ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME)) | WS_CHILD;
        SetWindowLong(hwnd, GWL_STYLE, style);
        
        // Show and position the window
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, 
                     SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
#else
    (void)gui;
    (void)parentHwnd;
#endif
}

void jamulus_gui_resize(void* gui, int width, int height)
{
    if (gui) {
        QWidget* w = reinterpret_cast<QWidget*>(gui);
        w->resize(width, height);
    }
}

void jamulus_gui_get_preferred_size(void* gui, int* width, int* height)
{
    if (gui && width && height) {
        QWidget* w = reinterpret_cast<QWidget*>(gui);
        QSize hint = w->sizeHint();
        *width = hint.width();
        *height = hint.height();
    }
}

} // extern "C"
