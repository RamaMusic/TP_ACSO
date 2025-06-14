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
#include <future>
#include <sys/wait.h>  // waitpid
#include <unistd.h>    // fork

using namespace std;
using namespace chrono;

mutex oslock;
bool global_success = true;

// ---------------------------------------------------------------------------
void sleep_for_ms(int ms) {
    this_thread::sleep_for(milliseconds(ms));
}

// ---------------------------------------------------------------------------
struct TestCase {
    string id;
    string name;

    function < bool(void) > testfn;
};

// ---------------------------------------------------------------------------
// Básicos (B): Casos simples
// ---------------------------------------------------------------------------

bool test_basic() {
    try {
        ThreadPool pool(2);
        vector < int > result(3, 0);
        for (int i = 0; i < 3; ++i) {
            pool.schedule([i, & result]() {
                result[i] = i + 1;
            });
        }
        pool.wait();
        return result == vector < int > ({
            1,
            2,
            3
        });
    } catch (...) {
        return false;
    }
}

bool test_wait_only() {
    try {
        ThreadPool pool(4);
        pool.wait();
        return true;
    } catch (...) {
        return false;
    }
}

bool test_serial_execution() {
    try {
        stringstream log;
        mutex mtx;
        ThreadPool pool(1);
        for (int i = 0; i < 5; ++i) {
            pool.schedule([i, & log, & mtx]() {
                lock_guard < mutex > l(mtx);
                log << i << " ";
            });
        }
        pool.wait();
        return log.str() == "0 1 2 3 4 ";
    } catch (...) {
        return false;
    }
}

bool test_fifo_single_thread() {
    try {
        ThreadPool pool(1); // un solo thread garantiza orden estricto
        vector < int > log;
        mutex mtx;

        for (int i = 0; i < 10; ++i) {
            pool.schedule([i, & log, & mtx]() {
                lock_guard < mutex > lock(mtx);
                log.push_back(i);
            });
        }

        pool.wait();

        for (int i = 0; i < 10; ++i) {
            if (log[i] != i) return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
// Concurrencia (C): Uso normal del pool
// ---------------------------------------------------------------------------

bool test_concurrent_stress() {
    try {
        const int N = 1000;
        vector < int > counter(N, 0);
        ThreadPool pool(8);
        for (int i = 0; i < N; ++i) {
            pool.schedule([i, & counter]() {
                counter[i] = 1;
            });
        }
        pool.wait();
        for (int v: counter)
            if (v != 1) return false;
        return true;
    } catch (...) {
        return false;
    }
}

bool test_reuse_pool() {
    try {
        ThreadPool pool(4);
        bool ok = false;
        pool.schedule([ & ]() {
            ok = true;
        });
        pool.wait();
        if (!ok) return false;
        ok = false;
        pool.schedule([ & ]() {
            ok = true;
        });
        pool.wait();
        return ok;
    } catch (...) {
        return false;
    }
}

bool test_multiple_wait_calls() {
    try {
        ThreadPool pool(4);
        atomic < int > val(0);
        pool.schedule([ & ]() {
            val++;
        });
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

bool test_massive_stress() {
    try {
        const int N = 10000;
        atomic < int > count(0);
        ThreadPool pool(16);
        for (int i = 0; i < N; ++i) {
            pool.schedule([ & ]() {
                count++;
            });
        }
        pool.wait();
        return count == N;
    } catch (...) {
        return false;
    }
}

bool test_long_tasks_then_quit() {
    try {
        ThreadPool pool(4);
        for (int i = 0; i < 10; ++i) {
            pool.schedule([ = ]() {
                sleep_for_ms(200);
            });
        }
        pool.wait();
        return true;
    } catch (...) {
        return false;
    }
}

bool test_many_short_tasks_on_few_threads() {
    try {
        ThreadPool pool(2);
        atomic < int > count(0);
        for (int i = 0; i < 200; ++i) {
            pool.schedule([ & ]() {
                sleep_for_ms(1);
                count++;
            });
        }
        pool.wait();
        return count == 200;
    } catch (...) {
        return false;
    }
}

bool test_potential_deadlock() {
    promise < bool > prom;
    future < bool > fut = prom.get_future();

    thread t([ & prom]() {
        try {
            ThreadPool pool(2);
            mutex mtx;
            bool ready = false;

            pool.schedule([ & ]() {
                lock_guard < mutex > l(mtx);
                ready = true;
                sleep_for_ms(200);
            });

            sleep_for_ms(50);

            bool locked = mtx.try_lock();
            if (!locked && !ready) {
                prom.set_value(true); // condición detectada correctamente
                return;
            }
            if (locked) mtx.unlock();

            pool.wait();
            prom.set_value(true); // no hubo deadlock
        } catch (...) {
            prom.set_value(false); // error inesperado
        }
    });

    if (fut.wait_for(chrono::milliseconds(1000)) != future_status::ready) {
        t.detach(); // se colgó
        return false;
    }

    bool result = fut.get();
    t.join();
    return result;
}

bool test_pending_tasks_tracking_simulado() {
    try {
        ThreadPool pool(4);
        atomic < int > counter {
            0
        };
        for (int i = 0; i < 100; ++i) {
            pool.schedule([ & ]() {
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

bool test_schedule_from_multiple_threads() {
    try {
        const int N = 500;
        atomic < int > count(0);
        ThreadPool pool(8);
        vector < thread > threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([ & ]() {
                for (int i = 0; i < N; ++i) {
                    pool.schedule([ & ]() {
                        count++;
                    });
                }
            });
        }
        for (auto & t: threads) t.join();
        pool.wait();
        return count == N * 4;
    } catch (...) {
        return false;
    }
}

bool test_schedule_after_destruction() {
    try {
        ThreadPool * pool = new ThreadPool(2);
        pool -> schedule([]() {
            sleep_for_ms(100);
        });
        pool -> wait();
        delete pool;
        try {
            pool -> schedule([]() {});
            return false;
        } catch (...) {
            return true;
        }
    } catch (...) {
        return true;
    }
}

bool test_schedule_inside_task() {
    promise < bool > prom;
    auto fut = prom.get_future();

    thread t([ & prom]() {
        try {
            ThreadPool pool(4);
            atomic < int > count(0);

            pool.schedule([ & ]() {
                count++;
                pool.schedule([ & ]() {
                    count++;
                });
            });

            pool.wait();
            prom.set_value(count == 2); // solo pasa si ambas tareas se ejecutaron
        } catch (...) {
            prom.set_value(false); // error inesperado
        }
    });

    if (fut.wait_for(chrono::milliseconds(1000)) != future_status::ready) {
        t.detach(); // posible cuelgue por mal manejo de schedule interno
        return false;
    }

    bool result = fut.get();
    t.join();
    return result;
}

bool test_wait_blocks_until_finish() {
    try {
        ThreadPool pool(2);
        atomic < bool > completed {
            false
        };
        pool.schedule([ & ]() {
            sleep_for_ms(300);
            completed = true;
        });
        pool.wait();
        return completed.load();
    } catch (...) {
        return false;
    }
}

bool test_many_waits_during_execution() {
    try {
        ThreadPool pool(4);
        atomic < int > completed(0);

        for (int i = 0; i < 50; ++i) {
            pool.schedule([ & ]() {
                sleep_for_ms(10);
                completed++;
            });
        }

        vector < thread > waiters;
        vector < future < bool >> futures;

        for (int i = 0; i < 5; ++i) {
            auto prom_ptr = make_shared < promise < bool >> ();
            futures.push_back(prom_ptr -> get_future());

            waiters.emplace_back([ & pool, prom_ptr]() {
                try {
                    pool.wait();
                    prom_ptr -> set_value(true);
                } catch (...) {
                    prom_ptr -> set_value(false);
                }
            });
        }

        for (auto & f: futures) {
            if (f.wait_for(chrono::milliseconds(1000)) != future_status::ready || !f.get()) {
                for (auto & t: waiters) t.detach();
                return false; // alguno se colgó o lanzó excepción
            }
        }

        for (auto & t: waiters) t.join();
        return completed == 50;
    } catch (...) {
        return false;
    }
}

bool test_high_contention_atomic_updates() {
    try {
        ThreadPool pool(4);
        atomic < int > counter {
            0
        };
        for (int i = 0; i < 1000; ++i) {
            pool.schedule([ & ]() {
                counter.fetch_add(1, memory_order_relaxed);
            });
        }
        pool.wait();
        return counter == 1000;
    } catch (...) {
        return false;
    }
}

bool test_immediate_destruction_after_schedule() {
    try {
        ThreadPool * pool = new ThreadPool(2);
        for (int i = 0; i < 10; ++i) {
            pool -> schedule([]() {
                sleep_for_ms(50);
            });
        }
        delete pool;
        return true;
    } catch (...) {
        return false;
    }
}

bool test_massive_schedule_wait_interleave() {
    promise < bool > prom;
    auto fut = prom.get_future();

    thread t([ & prom]() {
        try {
            ThreadPool pool(2);
            atomic < int > count(0);

            for (int i = 0; i < 50; ++i) {
                pool.schedule([ & ]() {
                    sleep_for_ms(2);
                    count++;
                });
                if (i % 5 == 0) pool.wait();
            }

            pool.wait();
            prom.set_value(count == 50); // solo pasa si todas las tareas se ejecutaron
        } catch (...) {
            prom.set_value(false); // se produjo una excepción inesperada
        }
    });

    if (fut.wait_for(chrono::milliseconds(1000)) != future_status::ready) {
        t.detach(); // el test se colgó, probablemente por sincronización incorrecta
        return false;
    }

    bool result = fut.get();
    t.join();
    return result;
}

bool test_schedule_after_wait_multiple_times() {
    promise < bool > prom;
    auto fut = prom.get_future();

    thread t([ & prom]() {
        try {
            ThreadPool pool(2);
            atomic < int > total {
                0
            };

            for (int round = 0; round < 20; ++round) {
                pool.schedule([ & ]() {
                    sleep_for_ms(5);
                    total++;
                });
                pool.wait();
            }

            prom.set_value(total == 20); // valida que se ejecutaron todas las tareas
        } catch (...) {
            prom.set_value(false); // se produjo una excepción inesperada
        }
    });

    if (fut.wait_for(chrono::milliseconds(1000)) != future_status::ready) {
        t.detach(); // el test se colgó, probablemente por manejo incorrecto de rondas
        return false;
    }

    bool result = fut.get();
    t.join();
    return result;
}

bool test_multiple_wait_inside_tasks() {
    promise < bool > prom;
    auto fut = prom.get_future();

    thread t([ & prom]() {
        ThreadPool pool(4);

        for (int i = 0; i < 4; ++i) {
            pool.schedule([ & ]() {
                pool.wait(); // esto puede colgar si no se maneja reentrancia bien
            });
        }

        // Este wait espera a que los anteriores terminen, lo que nunca sucederá
        pool.wait();
        prom.set_value(true); // no debería llegar
    });

    if (fut.wait_for(chrono::milliseconds(500)) != future_status::ready) {
        t.detach();
        return true; // correcto: se colgó
    }

    bool result = fut.get();
    t.join();
    return !result;
}

bool test_concurrent_schedule_wait_parallel() {
    const int schedulerThreads = 4;
    const int tasksPerThread = 50;
    const int expected = schedulerThreads * tasksPerThread;
    const int timeout_ms = 3000;

    try {
        ThreadPool pool(4);
        atomic<int> executed{0};

        vector<thread> schedulers;
        for (int s = 0; s < schedulerThreads; ++s) {
            schedulers.emplace_back([&]() {
                for (int i = 0; i < tasksPerThread; ++i) {
                    pool.schedule([&]() {
                        executed.fetch_add(1, memory_order_relaxed);
                        this_thread::sleep_for(chrono::milliseconds(1));
                    });
                }
            });
        }

        vector<thread> waiters;
        for (int w = 0; w < 2; ++w) {
            waiters.emplace_back([&]() {
                pool.wait();
            });
        }

        auto start = chrono::steady_clock::now();

        for (auto &t : schedulers) t.join();
        pool.wait();
        for (auto &t : waiters) t.join();

        auto end = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        return (executed == expected) && (elapsed <= timeout_ms);
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
// Lifecycle (L): pruebas de ciclo de vida del pool
// ---------------------------------------------------------------------------

bool test_destructor_waits_for_tasks() {
    auto start = high_resolution_clock::now();
    {
        ThreadPool pool(1);
        pool.schedule([]() {
            sleep_for_ms(100);
        });
    } // Destructor acá
    auto end = high_resolution_clock::now();
    auto ms = duration_cast < milliseconds > (end - start).count();
    return ms >= 100;
}

bool test_repeated_pool_creation() {
    try {
        const int rounds = 100;
        for (int r = 0; r < rounds; ++r) {
            ThreadPool pool(2);
            atomic<int> counter{0};
            for (int i = 0; i < 10; ++i) {
                pool.schedule([&]() {
                    counter.fetch_add(1, memory_order_relaxed);
                });
            }
            pool.wait();
            if (counter != 10) return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
// Nesting (N): scheduling anidado profundo
// ---------------------------------------------------------------------------

bool test_deep_nested_scheduling() {
    promise < bool > prom;
    auto fut = prom.get_future();

    thread t([ & prom]() {
        try {
            ThreadPool pool(4);
            atomic < int > count(0);
            pool.schedule([ & ]() {
                count++;
                pool.schedule([ & ]() {
                    count++;
                    pool.schedule([ & ]() {
                        count++;
                    });
                });
            });
            pool.wait();
            prom.set_value(count == 3); // solo pasa si las 3 tareas se ejecutaron
        } catch (...) {
            prom.set_value(false); // se produjo una excepción inesperada
        }
    });

    if (fut.wait_for(chrono::milliseconds(1000)) != future_status::ready) {
        t.detach(); // se colgó, probablemente por mal manejo del anidamiento
        return false;
    }

    bool result = fut.get();
    t.join();
    return result;
}

bool test_extreme_nested_scheduling() {
    const int depth = 1000;
    const int timeout_ms = 2000;

    try {
        ThreadPool pool(4);
        atomic<int> leafCount{0};

        function<void(int)> scheduleDepth = [&](int d) {
            if (d == 0) {
                leafCount.fetch_add(1, memory_order_relaxed);
                return;
            }
            pool.schedule([&, d]() {
                scheduleDepth(d - 1);
            });
        };

        auto start = chrono::steady_clock::now();

        scheduleDepth(depth);
        pool.wait();

        auto end = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        return (elapsed <= timeout_ms) && (leafCount == 1);
    } catch (...) {
        return false;
    }
}


// ---------------------------------------------------------------------------
// Timing (T): mediciones de paralelismo
// ---------------------------------------------------------------------------

bool test_parallel_speedup() {
    const int tasks = 4;
    const int sleep_ms = 100;
    ThreadPool pool(tasks);
    auto t0 = high_resolution_clock::now();
    for (int i = 0; i < tasks; ++i) {
        pool.schedule([ = ]() {
            sleep_for_ms(sleep_ms);
        });
    }
    pool.wait();
    auto t1 = high_resolution_clock::now();
    auto elapsed = duration_cast < milliseconds > (t1 - t0).count();
    // Debería ser significativamente menor que tasks * sleep_ms
    return elapsed < (sleep_ms * tasks / 2);
}

// ---------------------------------------------------------------------------
// Error-Handling (H): llamadas a wait dentro de tareas con timeout
// ---------------------------------------------------------------------------
bool test_wait_inside_task() {
    std::promise < bool > prom;
    auto fut = prom.get_future();

    std::thread t([ & prom]() {
        ThreadPool pool(2);

        pool.schedule([ & ]() {
            // Esto debería causar deadlock
            pool.wait();
            prom.set_value(true);
        });

        pool.wait();

        prom.set_value(false);
    });

    if (fut.wait_for(std::chrono::milliseconds(500)) == std::future_status::timeout) {
        t.detach();
        return true; // Solo pasa si se bloqueó correctamente
    }

    bool result = fut.get();
    t.join();
    return !result; // Debería ser false, ya que no debería poder ejecutar wait() dentro de una tarea 
}

// ---------------------------------------------------------------------------
// Misuse (M): pruebas de mal uso del pool
// ---------------------------------------------------------------------------

bool test_schedule_nullptr() {
    try {
        ThreadPool pool(2);

        function < void() > f = nullptr;
        pool.schedule(f); // comportamiento indefinido si no se valida
        pool.wait();
        return false; // si no lanza error, está mal
    } catch (...) {
        return true; // correcto: debe lanzar excepción o prevenirlo
    }
}

bool test_wait_with_infinite_schedule() {
    promise < bool > prom;
    auto fut = prom.get_future();

    thread t([ & prom]() {
        try {
            ThreadPool pool(2);
            pool.schedule([ & ]() {
                while (true) {
                    pool.schedule([]() {
                        sleep_for_ms(10);
                    });
                    sleep_for_ms(1);
                }
            });
            pool.wait(); // esto debería colgarse
            prom.set_value(false);
        } catch (...) {
            prom.set_value(true); // aceptable si se maneja
        }
    });

    if (fut.wait_for(milliseconds(500)) != future_status::ready) {
        t.detach();
        return true; // el wait se cuelga como debería
    }

    bool result = fut.get();
    t.join();
    return result == false;
}

// ---------------------------------------------------------------------------

void run_test(const TestCase& t) {
    const string reset  = "\033[0m";
    const string bold   = "\033[1m";

    const string colorB = "\033[36m";
    const string colorC = "\033[32m";
    const string colorE = "\033[35m";
    const string colorF = "\033[34m";
    const string colorH = "\033[31m";
    const string colorL = "\033[33m";
    const string colorM = "\033[91m";
    const string colorN = "\033[96m";
    const string colorT = "\033[95m";

    char group = t.id[0];
    string color;
    switch (group) {
        case 'B': color = colorB; break;
        case 'C': color = colorC; break;
        case 'E': color = colorE; break;
        case 'F': color = colorF; break;
        case 'H': color = colorH; break;
        case 'L': color = colorL; break;
        case 'M': color = colorM; break;
        case 'N': color = colorN; break;
        case 'T': color = colorT; break;
        default:  color = "";     break;
    }

    lock_guard<mutex> lg(oslock);
    cout << color << "[" << t.id << "]" << reset << " " << t.name << "... ";
    cout.flush();

    pid_t pid = fork();
    if (pid == 0) {
        // Proceso hijo: ejecuta el test
        bool result = t.testfn();
        exit(result ? 0 : 1);
    } else {
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            if (code == 0) {
                cout << "\033[1;32m✅ PASSED" << reset << "\n";
            } else {
                cout << "\033[1;31m❌ FAILED" << reset << "\n";
                global_success = false;
            }
        } else if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            cout << "\033[1;31m❌ FAILED\033[0m ";
            cout << "\033[1;31m💥 CRASHED (signal " << sig << ")\033[0m\n";
            global_success = false;
        } else {
            cout << "\033[1;31m❌ FAILED\033[0m ";
            cout << "\033[1;31m❓ UNKNOWN ERROR\033[0m\n";
            global_success = false;
        }
    }
}

void print_summary(const vector < TestCase > & tests) {
    cout << "\n========================================\n";
    cout << "Ran " << tests.size() << " tests.\n";
    cout << (global_success ? "✅ ALL TESTS PASSED\n" : "❌ SOME TESTS FAILED\n");
    cout << "========================================\n";
}

int main() {
    vector<TestCase> tests = {
        {"B01", "Basic execution (3 tasks on 2 threads)",           test_basic},
        {"B02", "Wait without scheduling",                          test_wait_only},
        {"B03", "Serial execution with 1 thread",                   test_serial_execution},
        {"B04", "FIFO execution in single-thread mode",             test_fifo_single_thread},

        {"C01", "Stress with 1000 tasks",                           test_concurrent_stress},
        {"C02", "Reusing the pool after wait",                      test_reuse_pool},
        {"C03", "Multiple wait() calls",                            test_multiple_wait_calls},

        {"E01", "Massive stress (10k tasks)",                       test_massive_stress},
        {"E02", "Long tasks then shutdown",                         test_long_tasks_then_quit},
        {"E03", "Lots of short tasks on few threads",               test_many_short_tasks_on_few_threads},
        {"E04", "Detect potential deadlock",                        test_potential_deadlock},
        {"E05", "Simulated pendingTasks tracking",                  test_pending_tasks_tracking_simulado},

        {"F01", "Schedule from multiple threads",                   test_schedule_from_multiple_threads},
        {"F02", "Schedule after destruction (invalid use)",         test_schedule_after_destruction},
        {"F03", "Schedule inside another task",                     test_schedule_inside_task},
        {"F04", "Wait blocks until all tasks finish",               test_wait_blocks_until_finish},
        {"F06", "Many waits in parallel",                           test_many_waits_during_execution},
        {"F07", "High contention on atomic counter",                test_high_contention_atomic_updates},
        {"F08", "Destroy pool immediately after scheduling",        test_immediate_destruction_after_schedule},
        {"F09", "Interleaved schedule/wait execution",              test_massive_schedule_wait_interleave},
        {"F10", "Multiple schedule/wait rounds",                    test_schedule_after_wait_multiple_times},
        {"F11", "Multiple wait() calls inside tasks",               test_multiple_wait_inside_tasks},
        {"F12", "Concurrent schedule/wait in parallel",             test_concurrent_schedule_wait_parallel},

        {"H01", "Wait inside task should deadlock",                 test_wait_inside_task},

        {"L01", "Destructor waits for tasks completion",            test_destructor_waits_for_tasks},
        {"L02", "Repeated pool creation and destruction",           test_repeated_pool_creation},

        {"M01", "Schedule nullptr function",                        test_schedule_nullptr},
        {"M02", "wait() during infinite rescheduling",              test_wait_with_infinite_schedule},

        {"N01", "Deep nested task scheduling",                      test_deep_nested_scheduling},
        {"N02", "Extreme nested scheduling (1000)",                 test_extreme_nested_scheduling},

        {"T01", "Parallel speedup benchmark (4 tasks)",             test_parallel_speedup},
    };

    for (const auto & t: tests) {
        run_test(t);
    }

    print_summary(tests);
    return global_success ? 0 : 1;
}