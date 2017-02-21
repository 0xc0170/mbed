#include "mbed.h"
#include "greentea-client/test_env.h"
#include "rtos.h"

#if defined(MBED_RTOS_SINGLE_THREAD)
  #error [NOT_SUPPORTED] test not supported
#endif

#define THREAD_DELAY     50
#define SIGNALS_TO_EMIT  100

#define THREAD_STACK_SIZE 1024

void print_char(char c = '*') {
    printf("%c", c);
    fflush(stdout);
}

Mutex stdio_mutex;
DigitalOut led(LED1);

volatile int change_counter = 0;
volatile bool changing_counter = false;
volatile bool mutex_defect = false;

bool manipulate_protected_zone(const int thread_delay) {
    bool result = true;

    stdio_mutex.lock(); // LOCK
    if (changing_counter == true) {
        // 'e' stands for error. If changing_counter is true access is not exclusively
        print_char('e');
        result = false;
        mutex_defect = true;
    }
    changing_counter = true;

    // Some action on protected
    led = !led;
    change_counter++;
    print_char('.');
    Thread::wait(thread_delay);

    changing_counter = false;
    stdio_mutex.unlock();   // UNLOCK
    return result;
}

void test_thread(int const *thread_delay) {
    while (true) {
        manipulate_protected_zone(*thread_delay);
    }
}

int main() {
    GREENTEA_SETUP(20, "default_auto");

    const int t1_delay = THREAD_DELAY * 1;
    const int t2_delay = THREAD_DELAY * 2;
    const int t3_delay = THREAD_DELAY * 3;

    Thread t2(osPriorityNormal, THREAD_STACK_SIZE);
    Thread t3(osPriorityNormal, THREAD_STACK_SIZE);

    t2.start(callback(test_thread, &t2_delay));
    t3.start(callback(test_thread, &t3_delay));

    while (true) {
        // Thread 1 action
        Thread::wait(t1_delay);
        manipulate_protected_zone(t1_delay);
        if (change_counter >= SIGNALS_TO_EMIT or mutex_defect == true) {
            t2.terminate();
            t3.terminate();
            break;
        }
    }

    fflush(stdout);
    GREENTEA_TESTSUITE_RESULT(!mutex_defect);
    return 0;
}
