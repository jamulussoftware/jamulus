#pragma once

extern "C" {
    typedef void* jamulus_client_t;

    // Create a Jamulus client instance. Parameters are simplified for initial prototype.
    jamulus_client_t jamulus_client_create(unsigned short port,
                                          const char* server_addr,
                                          const char* client_name,
                                          bool enable_ipv6);

    void jamulus_client_destroy(jamulus_client_t c);

    int jamulus_client_start(jamulus_client_t c);
    int jamulus_client_stop(jamulus_client_t c);

    int jamulus_client_set_server_addr(jamulus_client_t c, const char* addr);
    // Process audio through the internal Jamulus client: interleaved float
    // `in` (frames * channels) -> processed interleaved float `out`.
    // Returns 0 on success, negative on error.
    int jamulus_client_process_audio(jamulus_client_t c, const float* in, float* out, int frames, int channels);
    // Async push/pop API (real-time safe): push interleaved float frames into
    // client's input FIFO and pop processed frames from output FIFO.
    int jamulus_client_push_audio(jamulus_client_t c, const float* in, int frames, int channels);
    int jamulus_client_pop_audio(jamulus_client_t c, float* out, int maxFrames, int channels);
    int jamulus_client_start_worker(jamulus_client_t c, int processBlockFrames);
    int jamulus_client_stop_worker(jamulus_client_t c);

    // GUI functions - create and manage the Jamulus client dialog
    void* jamulus_gui_create(jamulus_client_t c);
    void jamulus_gui_destroy(void* gui);
    void jamulus_gui_show(void* gui);
    void jamulus_gui_hide(void* gui);
    void* jamulus_gui_get_native_handle(void* gui);
    void jamulus_gui_set_parent(void* gui, void* parentHwnd);
    void jamulus_gui_resize(void* gui, int width, int height);
    void jamulus_gui_position(void* gui, int x, int y);
    void jamulus_gui_get_preferred_size(void* gui, int* width, int* height);
    void jamulus_gui_repaint(void* gui);
    void jamulus_gui_set_active(bool active);  // Pause/resume Qt event pump
}
