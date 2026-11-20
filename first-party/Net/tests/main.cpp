
#include <cstdio>
#include <cstdlib>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

#include "../Net.h"
#include "../InterprocessLock.h"

#include "test_common.h"

void test_address();
void test_bitmsg();
void test_compress();
void test_hton();
void test_http();
void test_packet_queue();
void test_pcp();
void test_pmp();
void test_reliable_udp_connection();
void test_tcp_socket();
void test_udp_connection();
void test_udp_socket();
void test_var();
// no test_types(): its TEST_BE define changes Net::le_* type definitions and would ODR-clash with this binary

bool g_stop_on_fail = false;
std::atomic_int g_failures{};

int main() {
    printf("Net Version: %s\n", Net::Version());
    puts(" ---------------");

    // Ensure only single instance is running; failing to take the lock (Global\ namespace privileges,
    // abandoned mutex) must not kill the run by itself
    try {
        static Net::InterprocessLock _("test_Net lock");
    } catch (const std::exception &e) {
        printf("warning: no single-instance lock: %s\n", e.what());
    }

    // hard deadline: a hung blocking call must fail the run, not stall CI forever
    std::thread watchdog([] {
        std::this_thread::sleep_for(std::chrono::minutes(5));
        fflush(stdout);
        fputs("FATAL: test_Net exceeded 5 minutes, aborting\n", stderr);
        std::_Exit(2);
    });
    watchdog.detach();

    test_address();
    test_compress();
    test_hton();
    test_http();
    test_packet_queue();
    test_var();
    test_bitmsg();
    test_pcp();
    test_pmp();
    test_reliable_udp_connection();
    test_udp_connection();
    test_udp_socket();
    test_tcp_socket();

    return g_failures ? 1 : 0;
}
