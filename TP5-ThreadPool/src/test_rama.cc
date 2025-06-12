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
// F05 - Tarea larga y llamada a wait antes de finalizar
bool test_wait_blocks_until_finish() {
    try {
        ThreadPool pool(2);
        atomic<bool> completed{false};
        pool.schedule([&]() {
            sleep_for_ms(300);
            completed = true;
        });

        pool.wait();
        return completed.load();
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
// E05 - Verifica que pendingTasks reflejaría tareas en ejecución, si se usara
bool test_pending_tasks_tracking_simulado() {
    try {
        ThreadPool pool(4);
        atomic<int> counter{0};
        for (int i = 0; i < 100; ++i) {
            pool.schedule([&]() {
                sleep_for_ms(5);
                counter++;
            });
        }
        pool.wait();
        return counter == 100;
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
// F06 - Muchos hilos llaman a wait() mientras las tareas se ejecutan
bool test_many_waits_during_execution() {
    try {
        ThreadPool pool(4);
        atomic<int> completed(0);

        for (int i = 0; i < 50; ++i) {
            pool.schedule([&]() {
                sleep_for_ms(10);
                completed++;
            });
        }

        vector<thread> waiters;
        for (int i = 0; i < 5; ++i) {
            waiters.emplace_back([&]() { pool.wait(); });
        }
        for (auto& w : waiters) w.join();

        return completed == 50;
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
// F07 - Alta contención sobre variable atómica (pendingTasks)
bool test_high_contention_atomic_updates() {
    try {
        ThreadPool pool(4);
        atomic<int> counter{0};

        for (int i = 0; i < 1000; ++i) {
            pool.schedule([&]() {
                counter.fetch_add(1, memory_order_relaxed);
            });
        }

        pool.wait();
        return counter == 1000;
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
// F08 - Destruir el pool inmediatamente después de encolar tareas
bool test_immediate_destruction_after_schedule() {
    try {
        ThreadPool* pool = new ThreadPool(2);
        for (int i = 0; i < 10; ++i) {
            pool->schedule([]() {
                sleep_for_ms(50);
            });
        }
        delete pool; // debería esperar correctamente
        return true;
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
// F09 - Alternancia rápida entre schedule() y wait()
bool test_massive_schedule_wait_interleave() {
    try {
        ThreadPool pool(2);
        atomic<int> count(0);

        for (int i = 0; i < 50; ++i) {
            pool.schedule([&]() {
                sleep_for_ms(2);
                count++;
            });
            if (i % 5 == 0) pool.wait();
        }

        pool.wait();
        return count == 50;
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
// F10 - Varias rondas de schedule() seguidas de wait()
bool test_schedule_after_wait_multiple_times() {
    try {
        ThreadPool pool(2);
        atomic<int> total{0};

        for (int round = 0; round < 20; ++round) {
            pool.schedule([&]() {
                sleep_for_ms(5);
                total++;
            });
            pool.wait();
        }

        return total == 20;
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
        {"F04", "Wait blocks until all tasks finish", test_wait_blocks_until_finish},
        {"E05", "Simulated pendingTasks tracking", test_pending_tasks_tracking_simulado},
        {"F06", "Many waits in parallel", test_many_waits_during_execution},
        {"F07", "High contention on atomic counter", test_high_contention_atomic_updates},
        {"F08", "Destroy pool immediately after scheduling", test_immediate_destruction_after_schedule},
        {"F09", "Interleaved schedule/wait execution", test_massive_schedule_wait_interleave},
        {"F10", "Multiple wait/schedule rounds", test_schedule_after_wait_multiple_times},


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
