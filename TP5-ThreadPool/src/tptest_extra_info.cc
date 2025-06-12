/**
 * File: tptest.cc
 * ---------------
 * Simple test in place to verify that the ThreadPool class works.
 */

#include <iostream>
#include "thread-pool.h"
#include <mutex>
using namespace std;

void sleep_for(int slp){
    this_thread::sleep_for(chrono::milliseconds(slp));
}

static mutex oslock;

static const size_t kNumThreads = 12;
static const size_t kNumFunctions = 1000;

// Variable global para registrar si hubo algún problema (por ahora la usamos manualmente)
static bool success = true;

static void simpleTest() {
  ThreadPool pool(kNumThreads);
  for (size_t id = 0; id < kNumFunctions; id++) {
    pool.schedule([id] {
      try {
        oslock.lock();
        cout << "Thread (ID: " << id << ") has started." << endl;
        oslock.unlock();

        size_t sleepTime = (id % 3) * 10;
        sleep_for(sleepTime);

        oslock.lock();
        cout << "Thread (ID: " << id << ") has finished." << endl;
        oslock.unlock();
      } catch (...) {
        success = false; // cualquier excepción cuenta como fallo
      }
    });
  }

  pool.wait();
}

int main(int argc, char *argv[]) {
  simpleTest();

  cout << "----------------------------------------" << endl;
  if (success) {
    cout << "✅ TEST PASSED: all threads completed successfully." << endl;
  } else {
    cout << "❌ TEST FAILED: an error occurred in at least one thread." << endl;
  }

  return success ? 0 : 1;
}
