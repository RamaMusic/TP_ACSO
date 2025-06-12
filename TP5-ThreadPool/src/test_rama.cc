// File: ramiro_tests.cc
// ----------------------
// Mega tester para ThreadPool: casos básicos, extremos y situaciones límite.
// Incluye formato limpio, documentación clara y veredictos finales.

#include "thread-pool.h"
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <functional>
#include <atomic>
#include <sstream>
#include <stdexcept>

using namespace std;

mutex oslock;
bool global_success = true;

void sleep_for_ms(int ms) {
    this_thread::sleep_for(chrono::milliseconds(ms));
}

struct TestCase {
    string id;
    string name;
    function<bool(void)> testfn; // true si pasa, false si falla
};

// ---------------------------------------------------------------------------
bool test_basic() {
    try {
        ThreadPool pool(2);
        vector<int> result(3, 0);
        for (int i = 0; i < 3; ++i) {
            pool.schedule([i, &result](){ result[i] = i + 1; });
        }
        pool.wait();
        return result == vector<int>({1,2,3});
    } catch (...) { return false; }
}

bool test_wait_only() {
    try {
        ThreadPool pool(4);
        pool.wait();
        return true;
    } catch (...) { return false; }
}

bool test_concurrent_stress() {
    try {
        const int N = 1000;
        vector<int> counter(N, 0);
        ThreadPool pool(8);
        for (int i = 0; i < N; ++i) {
            pool.schedule([i, &counter](){ counter[i] = 1; });
        }
        pool.wait();
        for (int v : counter)
            if (v != 1) return false;
        return true;
    } catch (...) { return false; }
}

bool test_reuse_pool() {
    try {
        ThreadPool pool(4);
        bool ok = false;
        pool.schedule([&](){ ok = true; });
        pool.wait();
        if (!ok) return false;
        ok = false;
        pool.schedule([&](){ ok = true; });
        pool.wait();
        return ok;
    } catch (...) { return false; }
}

bool test_massive_stress() {
    try {
        const int N = 10000;
        atomic<int> count(0);
        ThreadPool pool(16);
        for (int i = 0; i < N; ++i) {
            pool.schedule([&]() { count++; });
        }
        pool.wait();
        return count == N;
    } catch (...) { return false; }
}

bool test_schedule_from_multiple_threads() {
    try {
        const int N = 500;
        atomic<int> count(0);
        ThreadPool pool(8);
        vector<thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&]() {
                for (int i = 0; i < N; ++i) {
                    pool.schedule([&](){ count++; });
                }
            });
        }
        for (auto& t : threads) t.join();
        pool.wait();
        return count == N * 4;
    } catch (...) { return false; }
}

bool test_long_tasks_then_quit() {
    try {
        ThreadPool pool(4);
        for (int i = 0; i < 10; ++i) {
            pool.schedule([=](){ sleep_for_ms(200); });
        }
        pool.wait();
        return true;
    } catch (...) { return false; }
}

bool test_multiple_wait_calls() {
    try {
        ThreadPool pool(4);
        atomic<int> val(0);
        pool.schedule([&](){ val++; });
        pool.wait();
        pool.wait();
        return val == 1;
    } catch (...) { return false; }
}

bool test_serial_execution() {
    try {
        stringstream log;
        mutex mtx;
        ThreadPool pool(1);
        for (int i = 0; i < 5; ++i) {
            pool.schedule([i, &log, &mtx]() {
                lock_guard<mutex> l(mtx);
                log << i << " ";
            });
        }
        pool.wait();
        return log.str() == "0 1 2 3 4 ";
    } catch (...) { return false; }
}

bool test_many_short_tasks_on_few_threads() {
    try {
        ThreadPool pool(2);
        atomic<int> count(0);
        for (int i = 0; i < 200; ++i) {
            pool.schedule([&](){ sleep_for_ms(1); count++; });
        }
        pool.wait();
        return count == 200;
    } catch (...) { return false; }
}

// ---------------------------------------------------------------------------
// F02 - Llamar schedule después del destructor (no debe crashear el programa)
bool test_schedule_after_destruction() {
    try {
        ThreadPool* pool = new ThreadPool(2);
        pool->schedule([]() { sleep_for_ms(100); });
        pool->wait();
        delete pool;
        try {
            pool->schedule([]() {}); // comportamiento indefinido, simulamos fallo esperado
            return false;            // si llegó acá sin crashear, algo anda mal
        } catch (...) {
            return true;             // si rompe, está bien
        }
    } catch (...) {
        return true; // si se rompe antes, es aceptable
    }
}

// ---------------------------------------------------------------------------
// E04 - Deadlock inducido si wait() es llamada mientras un thread no libera
bool test_potential_deadlock() {
    try {
        ThreadPool pool(2);
        mutex mtx;
        bool ready = false;
        pool.schedule([&]() {
            lock_guard<mutex> l(mtx);
            ready = true;
            sleep_for_ms(200);
        });

        sleep_for_ms(50);

        bool locked = mtx.try_lock();
        if (!locked && !ready) {
            // El hilo está esperando mtx, y no liberó
            return true; // situación detectada, test útil
        }
        if (locked) mtx.unlock();
        pool.wait();
        return true;
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
// F03 - Schedule desde dentro de una tarea
bool test_schedule_inside_task() {
    try {
        ThreadPool pool(4);
        atomic<int> count(0);
        pool.schedule([&]() {
            count++;
            pool.schedule([&]() { count++; });
        });
        pool.wait();
        return count == 2;
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
void run_test(const TestCase& t) {
    cout << "[" << t.id << "] " << t.name << "... ";
    bool result = t.testfn();
    if (result) {
        cout << "✅ PASSED" << endl;
    } else {
        cout << "❌ FAILED" << endl;
        global_success = false;
    }
}

void print_summary(const vector<TestCase>& tests) {
    cout << "\n========================================\n";
    cout << "Ran " << tests.size() << " tests." << endl;
    if (global_success) {
        cout << "✅ ALL TESTS PASSED" << endl;
    } else {
        cout << "❌ SOME TESTS FAILED" << endl;
    }
    cout << "========================================\n";
}

int main() {
    vector<TestCase> tests = {
        {"B01", "Basic execution (3 tasks on 2 threads)", test_basic},
        {"B02", "Wait without scheduling", test_wait_only},
        {"C01", "Stress with 1000 tasks", test_concurrent_stress},
        {"C02", "Reusing the pool after wait", test_reuse_pool},
        {"E01", "Massive stress (10k tasks)", test_massive_stress},
        {"F01", "Schedule from multiple threads", test_schedule_from_multiple_threads},
        {"E02", "Long tasks then shutdown", test_long_tasks_then_quit},
        {"C03", "Multiple wait() calls", test_multiple_wait_calls},
        {"B03", "Serial execution with 1 thread", test_serial_execution},
        {"E03", "Lots of short tasks on few threads", test_many_short_tasks_on_few_threads},
        {"F02", "Schedule after destruction (invalid use)", test_schedule_after_destruction},
        {"E04", "Detect potential deadlock", test_potential_deadlock},
        {"F03", "Schedule inside another task", test_schedule_inside_task},
    };

    for (const auto& t : tests) {
        run_test(t);
    }

    print_summary(tests);
    return global_success ? 0 : 1;
}

// Código	Categoría		Qué significa
// B		Basic			Casos simples: ejecución, wait(), secuencial
// C		Concurrency		Casos de concurrencia, uso normal del pool
// E		Extreme/Edge		Casos de stress, posibles deadlocks
// F		Functional		Casos de diseño lógico o llamadas internas
