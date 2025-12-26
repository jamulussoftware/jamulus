#include "jamulus_wrapper.h"
#include <QString>
#include "../libjamulus/../src/client.h"
#include "../libjamulus/../src/settings.h"
#include "../libjamulus/../src/clientdlg.h"
#include <QApplication>
#include <QLibraryInfo>
#include <QTimer>
#include <QWindow>
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

// Global event filter that intercepts Close and other problematic events on ALL widgets
// to prevent dialogs from triggering logic that could crash the plugin.
class GlobalEventFilter : public QObject
{
public:
    explicit GlobalEventFilter(QObject* parent = nullptr) : QObject(parent) {}
    
    // Set the main embedded window to protect from focus events
    void setEmbeddedWindow(QWidget* w) { embeddedWindow = w; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        QWidget* w = qobject_cast<QWidget*>(watched);
        
        // Intercept close events on the MAIN embedded window only - convert to hide
        if (event->type() == QEvent::Close) {
            if (w && w == embeddedWindow) {
                w->hide();
                event->ignore();
                return true;
            }
            // Let other dialogs (Settings, Chat, etc.) close normally - they just hide themselves
        }
        
        // Only block focus/activation events on the EMBEDDED window (not popup dialogs)
        if (w && w == embeddedWindow) {
            switch (event->type()) {
                case QEvent::WindowActivate:
                case QEvent::WindowDeactivate:
                case QEvent::FocusIn:
                case QEvent::FocusOut:
                case QEvent::FocusAboutToChange:
                case QEvent::ActivationChange:
                    return true;
                default:
                    break;
            }
        }
        
        return QObject::eventFilter(watched, event);
    }
    
private:
    QWidget* embeddedWindow = nullptr;
};

// Global event filter instance
static GlobalEventFilter* g_globalFilter = nullptr;

// Global GUI instances - forward declare
static CClientSettings* g_settings = nullptr;
static CClientDlg* g_clientDlg = nullptr;

// Re-entrancy guard for Qt initialization
static bool g_qtAppInitializing = false;

// Ensure QApplication exists and event processing is set up
static void ensureQtApp() {
    // Guard against re-entrant calls during Qt initialization
    if (g_qtAppInitializing) {
        return;
    }
    
    static bool s_pathsSetup = false;
    if (!s_pathsSetup) {
        setupQtPaths();
        s_pathsSetup = true;
    }

    if (!QApplication::instance()) {
        g_qtAppInitializing = true;
        
        static int fakeArgc = 1;
        static char fakeArg0[] = "jamulus_vst3";
        static char* fakeArgv[] = { fakeArg0, nullptr };
        g_qtApp = new QApplication(fakeArgc, fakeArgv);
        
        // Install global event filter to intercept close events on all windows
        g_globalFilter = new GlobalEventFilter(g_qtApp);
        g_qtApp->installEventFilter(g_globalFilter);
        
        // Create a timer to pump Qt events since we're not running exec()
        g_eventPumpTimer = new QTimer();
        QObject::connect(g_eventPumpTimer, &QTimer::timeout, []() {
            QApplication::processEvents(QEventLoop::AllEvents, 10);
        });
        g_eventPumpTimer->start(16); // ~60fps event processing
        
        g_qtAppInitializing = false;
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
        
        // Make it frameless and non-focusable to prevent crashes
        // Don't use WindowStaysOnTopHint - it's annoying when switching apps
        g_clientDlg->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
        g_clientDlg->setAttribute(Qt::WA_ShowWithoutActivating, true);
        
        // Register this as the embedded window to protect from focus events
        if (g_globalFilter) {
            g_globalFilter->setEmbeddedWindow(g_clientDlg);
        }
        
        // Hide ASIO/sound card related widgets - audio comes from host, not ASIO
        // The settings dialog is a separate top-level window, so we need to search
        // all application top-level widgets
        
        QStringList widgetsToHide = {
            "cbxSoundcard",             // Sound card dropdown
            "lblSoundcardDevice",       // "Audio Device" label
            "butDriverSetup",           // "ASIO Device Settings" button
            "FrameSoundcardChannelSelection",  // Channel selection frame
            "lblInputChannelL",
            "lblInputChannelR", 
            "lblOutputChannelL",
            "lblOutputChannelR",
            "cbxLInChan",
            "cbxRInChan",
            "cbxLOutChan",
            "cbxROutChan",
            "rbtBufferDelayPreferred",  // Buffer delay options
            "rbtBufferDelayDefault",
            "rbtBufferDelaySafe",
            "grbSoundCrdBufDelay",      // Buffer delay group
        };
        
        // Search in ALL top-level widgets (dialogs) and hide matching widgets
        for (QWidget* topLevel : QApplication::topLevelWidgets()) {
            QList<QWidget*> allWidgets = topLevel->findChildren<QWidget*>();
            for (QWidget* w : allWidgets) {
                if (widgetsToHide.contains(w->objectName())) {
                    w->hide();
                }
            }
        }
        // Note: CloseToHideFilter is now installed globally on QApplication in ensureQtApp()
    }
    
    return g_clientDlg;
}

void jamulus_gui_destroy(void* gui)
{
    // Don't actually destroy - keep the singleton alive
    // The dialog will be hidden when the editor closes
    (void)gui;
}

// Helper to ensure window stays non-activatable
static void ensureNoActivate(QWidget* w)
{
#ifdef _WIN32
    if (w) {
        HWND hwnd = reinterpret_cast<HWND>(w->winId());
        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        if (!(exStyle & WS_EX_NOACTIVATE)) {
            SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_NOACTIVATE);
        }
    }
#else
    (void)w;
#endif
}

void jamulus_gui_show(void* gui)
{
    if (gui) {
        QWidget* w = reinterpret_cast<QWidget*>(gui);
        w->show();
        
#ifdef _WIN32
        HWND hwnd = reinterpret_cast<HWND>(w->winId());
        // Position at (0,0) and force redraw
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, 
                     SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED);
        // Invalidate and force immediate repaint at Windows level
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
#endif
        
        // Qt-level updates
        w->raise();
        w->repaint();
        
        // Schedule follow-up repaints
        QTimer::singleShot(100, w, [w]() {
            if (w->isVisible()) {
#ifdef _WIN32
                HWND hwnd = reinterpret_cast<HWND>(w->winId());
                InvalidateRect(hwnd, NULL, TRUE);
                UpdateWindow(hwnd);
#endif
                w->repaint();
            }
        });
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
    if (!gui || !parentHwnd) return;
    
    QWidget* w = reinterpret_cast<QWidget*>(gui);
    HWND qtHwnd = reinterpret_cast<HWND>(w->winId());
    HWND parent = reinterpret_cast<HWND>(parentHwnd);
    
    // Make it a child window with simple style
    LONG_PTR style = GetWindowLongPtr(qtHwnd, GWL_STYLE);
    style = (style & ~WS_POPUP) | WS_CHILD;
    SetWindowLongPtr(qtHwnd, GWL_STYLE, style);
    
    // Set the parent
    SetParent(qtHwnd, parent);
    
    // Position at origin of parent
    SetWindowPos(qtHwnd, HWND_TOP, 0, 0, 0, 0, 
                 SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
#else
    (void)gui;
    (void)parentHwnd;
#endif
}

void jamulus_gui_resize(void* gui, int width, int height)
{
    if (gui) {
        QWidget* w = reinterpret_cast<QWidget*>(gui);
#ifdef _WIN32
        HWND hwnd = reinterpret_cast<HWND>(w->winId());
        SetWindowPos(hwnd, HWND_TOP, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
#else
        w->move(0, 0);
        w->resize(width, height);
#endif
    }
}

void jamulus_gui_position(void* gui, int x, int y)
{
    if (gui) {
        QWidget* w = reinterpret_cast<QWidget*>(gui);
        w->move(x, y);
        ensureNoActivate(w);  // Re-apply after move
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

void jamulus_gui_repaint(void* gui)
{
    if (gui) {
        QWidget* w = reinterpret_cast<QWidget*>(gui);
        w->repaint();
        // Also repaint all child widgets
        for (QWidget* child : w->findChildren<QWidget*>()) {
            child->repaint();
        }
    }
}

void jamulus_gui_set_active(bool active)
{
    // Pause or resume Qt event pump based on whether the GUI is visible/active
    if (g_eventPumpTimer) {
        if (active) {
            if (!g_eventPumpTimer->isActive()) {
                g_eventPumpTimer->start(16);
            }
        } else {
            g_eventPumpTimer->stop();
        }
    }
}

} // extern "C"
