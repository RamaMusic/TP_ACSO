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
    function<bool(void)> testfn;
};

// ---------------------------------------------------------------------------
// Básicos (B): Casos simples
// ---------------------------------------------------------------------------

// Verifica ejecución de 3 tareas en un pool de 2 hilos
bool test_basic() {
    try {
        ThreadPool pool(2);
        vector<int> result(3, 0);
        for (int i = 0; i < 3; ++i) {
            pool.schedule([i, &result](){ result[i] = i + 1; });
        }
        pool.wait();
        return result == vector<int>({1,2,3});
    } catch (...) {
        return false;
    }
}

// Asegura que wait() no bloquea cuando no hay tareas
bool test_wait_only() {
    try {
        ThreadPool pool(4);
        pool.wait();
        return true;
    } catch (...) {
        return false;
    }
}

// Comprueba ejecución secuencial con un único hilo
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
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
// Concurrencia (C): Uso normal del pool
// ---------------------------------------------------------------------------

// Evalúa estrés con 1000 tareas concurrentes
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
    } catch (...) {
        return false;
    }
}

// Comprueba reutilización del pool tras llamada a wait()
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
    } catch (...) {
        return false;
    }
}

// Verifica que múltiples wait() no afectan la ejecución
bool test_multiple_wait_calls() {
    try {
        ThreadPool pool(4);
        atomic<int> val(0);
        pool.schedule([&](){ val++; });
        pool.wait();
        pool.wait();
        return val == 1;
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
// Extremos (E): Casos de estrés y detección de fallos
// ---------------------------------------------------------------------------

// Ejecuta 10 000 tareas para probar atomicidad
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
    } catch (...) {
        return false;
    }
}

// Prueba tareas largas seguidas de shutdown
bool test_long_tasks_then_quit() {
    try {
        ThreadPool pool(4);
        for (int i = 0; i < 10; ++i) {
            pool.schedule([=](){ sleep_for_ms(200); });
        }
        pool.wait();
        return true;
    } catch (...) {
        return false;
    }
}

// Ejecuta muchas tareas cortas en pocos hilos
bool test_many_short_tasks_on_few_threads() {
    try {
        ThreadPool pool(2);
        atomic<int> count(0);
        for (int i = 0; i < 200; ++i) {
            pool.schedule([&](){ sleep_for_ms(1); count++; });
        }
        pool.wait();
        return count == 200;
    } catch (...) {
        return false;
    }
}

// Detecta posible deadlock al intentar lockear durante la ejecución
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
            return true;
        }
        if (locked) mtx.unlock();
        pool.wait();
        return true;
    } catch (...) {
        return false;
    }
}

// Simula seguimiento de tareas pendientes sin acceso interno
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
// Funcionales (F): Casos de diseño lógico interno
// ---------------------------------------------------------------------------

// Permite scheduling desde múltiples hilos
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
    } catch (...) {
        return false;
    }
}

// Verifica comportamiento al programar tras destrucción del pool
bool test_schedule_after_destruction() {
    try {
        ThreadPool* pool = new ThreadPool(2);
        pool->schedule([]() { sleep_for_ms(100); });
        pool->wait();
        delete pool;
        try {
            pool->schedule([]() {});
            return false;
        } catch (...) {
            return true;
        }
    } catch (...) {
        return true;
    }
}

// Permite programar tareas desde dentro de otra tarea
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

// Asegura que wait() bloquea hasta completar tareas largas
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

// Permite múltiples wait() en paralelo durante la ejecución
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

// Prueba alta contención en actualizaciones atómicas
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

// Destruye el pool inmediatamente después de encolar tareas
bool test_immediate_destruction_after_schedule() {
    try {
        ThreadPool* pool = new ThreadPool(2);
        for (int i = 0; i < 10; ++i) {
            pool->schedule([]() {
                sleep_for_ms(50);
            });
        }
        delete pool;
        return true;
    } catch (...) {
        return false;
    }
}

// Alterna rápidamente schedule() y wait()
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

// Varias rondas de schedule() seguidas de wait()
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
        cout << "✅ PASSED\n";
    } else {
        cout << "❌ FAILED\n";
        global_success = false;
    }
}

void print_summary(const vector<TestCase>& tests) {
    cout << "\n========================================\n";
    cout << "Ran " << tests.size() << " tests.\n";
    cout << (global_success ? "✅ ALL TESTS PASSED\n" : "❌ SOME TESTS FAILED\n");
    cout << "========================================\n";
}

int main() {
    vector<TestCase> tests = {
        {"B01", "Basic execution (3 tasks on 2 threads)", test_basic},
        {"B02", "Wait without scheduling",        test_wait_only},
        {"B03", "Serial execution with 1 thread", test_serial_execution},

        {"C01", "Stress with 1000 tasks",        test_concurrent_stress},
        {"C02", "Reusing the pool after wait",    test_reuse_pool},
        {"C03", "Multiple wait() calls",         test_multiple_wait_calls},

        {"E01", "Massive stress (10k tasks)",    test_massive_stress},
        {"E02", "Long tasks then shutdown",      test_long_tasks_then_quit},
        {"E03", "Lots of short tasks on few threads", test_many_short_tasks_on_few_threads},
        {"E04", "Detect potential deadlock",     test_potential_deadlock},
        {"E05", "Simulated pendingTasks tracking", test_pending_tasks_tracking_simulado},

        {"F01", "Schedule from multiple threads",        test_schedule_from_multiple_threads},
        {"F02", "Schedule after destruction (invalid use)", test_schedule_after_destruction},
        {"F03", "Schedule inside another task",         test_schedule_inside_task},
        {"F04", "Wait blocks until all tasks finish",   test_wait_blocks_until_finish},
        {"F06", "Many waits in parallel",               test_many_waits_during_execution},
        {"F07", "High contention on atomic counter",    test_high_contention_atomic_updates},
        {"F08", "Destroy pool immediately after scheduling", test_immediate_destruction_after_schedule},
        {"F09", "Interleaved schedule/wait execution",  test_massive_schedule_wait_interleave},
        {"F10", "Multiple schedule/wait rounds",        test_schedule_after_wait_multiple_times},
    };

    for (const auto& t : tests) {
        run_test(t);
    }

    print_summary(tests);
    return global_success ? 0 : 1;
}
