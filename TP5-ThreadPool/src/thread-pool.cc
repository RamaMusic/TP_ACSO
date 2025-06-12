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
    for (size_t i = 0; i < numThreads; ++i) {
        wts[i].ts = thread(&ThreadPool::worker, this, i); // Worker por hilo
    }

    dt = thread(&ThreadPool::dispatcher, this); // Inicio el dispatcher
}

/**
 * Worker: espera una señal del dispatcher para ejecutar una tarea,
 * la corre, y luego se marca como disponible.
 * 
 * argument id: el identificador del worker (índice en el vector wts).
 */
void ThreadPool::worker(int id) {
    while (!done) {
        wts[id].ready.wait(); // espera hasta que el dispatcher lo despierte

        if (done) break;

        function<void(void)> task;
        {
            lock_guard<mutex> lock(wts[id].lock);
            task = wts[id].thunk;
        }

        task(); // ejecuta la tarea

        {
            lock_guard<mutex> lock(wts[id].lock);
            wts[id].available = true;
        }
    }
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

/**
 * Dispatcher: espera nuevas tareas, busca un worker libre y se la asigna.
 */
void ThreadPool::dispatcher() {
    while (!done) {
        tasksAvailable.wait(); // bloquea hasta que haya tna tarea

        if (done) break;

        function<void(void)> task;
        {
            lock_guard<mutex> lock(queueLock);
            if (taskQueue.empty()) continue;
            task = taskQueue.front();
            taskQueue.pop();
        }

        // buscamos un worker disponible
        bool assigned = false;
        while (!assigned && !done) {
            for (size_t i = 0; i < wts.size(); ++i) {
                lock_guard<mutex> lock(wts[i].lock);
                if (wts[i].available) {
                    wts[i].available = false;
                    wts[i].thunk = task;
                    wts[i].ready.signal(); // lo despertamos
                    assigned = true;
                    break;
                }
            }
            if (!assigned) {
                this_thread::sleep_for(chrono::milliseconds(1)); // si estan todos ocupados, esperamos.
            }
        }
    }
}

/**
 * Destructor: espera a que terminen todas las tareas y cierra el pool ordenadamente.
 */
ThreadPool::~ThreadPool() {
    wait();            // nos aseguramos que no quedan tareas en ejecución
    done = true;       // marcamos que se va a cerrar todo
    tasksAvailable.signal(); // por si el dispatcher está esperando

    // despertamos a todos los workers que puedan estar bloqueados
    for (worker_t& w : wts) {
        w.ready.signal();
    }

    // esperamos a que todos los hilos terminen
    if (dt.joinable()) dt.join();
    for (worker_t& w : wts) {
        if (w.ts.joinable()) w.ts.join();
    }
}

