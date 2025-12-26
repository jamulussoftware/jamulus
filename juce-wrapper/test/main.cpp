#include <iostream>
#include <thread>
#include <chrono>
#include "../jamulus_wrapper.h"

int main()
{
    auto c = jamulus_client_create(0, nullptr, "test-client", false);
    if (!c) {
        std::cerr << "failed to create client\n";
        return 1;
    }

    jamulus_client_start(c);
    // let network threads run a bit if present
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    jamulus_client_stop(c);
    jamulus_client_destroy(c);
    std::cout << "ok\n";
    return 0;
}
