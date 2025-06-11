/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
#include <chrono>
#include <thread>

using namespace std;

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), done(false) {
    // arrancamos el dispatcher
    dt = thread(&ThreadPool::dispatcher, this);
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    lock_guard<mutex> lock(queueLock);
    taskQueue.push(thunk);
    tasksAvailable.signal(); // aviso que hay una nueva tarea
}

void ThreadPool::wait() {
    while (true) {
        {
            lock_guard<mutex> lock(queueLock);
            if (taskQueue.empty()) break;
        }
        this_thread::sleep_for(chrono::milliseconds(10)); // pausa corta para no saturar el CPU mientras esperamos
    }
}

void ThreadPool::dispatcher() {
    while (!done) {
        tasksAvailable.wait(); // esperamos a que haya tareas disponibles

        function<void(void)> task;
        {
            lock_guard<mutex> lock(queueLock);
            if (taskQueue.empty()) continue;
            task = taskQueue.front();
            taskQueue.pop(); // sacamos del queue
        }

        task(); // ejecutamos
    }
}

ThreadPool::~ThreadPool() {
    wait();
    done = true;
    tasksAvailable.signal();
    if (dt.joinable()) dt.join();
}
