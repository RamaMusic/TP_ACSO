/**
 * File: thread-pool.cc
 * --------------------
 * Implementación de la clase ThreadPool.
 */

#include "thread-pool.h"
#include <chrono>
#include <thread>
#include <stdexcept>

using namespace std;

/**
 * Constructor: inicializa los workers y el dispatcher.
 * @param numThreads - cantidad de hilos trabajadores.
 */
ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), done(false) {
    for (size_t i = 0; i < numThreads; ++i) { 
        wts[i].ts = thread(&ThreadPool::worker, this, i); // Inicia cada worker
    }
    dt = thread(&ThreadPool::dispatcher, this);
}

/**
 * worker: ejecuta tareas asignadas y marca disponibilidad al terminar.
 * @param id - identificador del worker (posición en wts).
 */
void ThreadPool::worker(int id) {
    while (!done) {
        wts[id].ready.wait(); // espera que el dispatcher le asigne trabajo
        if (done) break;

        function<void(void)> task;
        {
            lock_guard<mutex> lock(wts[id].lock);
            task = wts[id].thunk;
        }

        task(); // ejecuta la tarea

        {
            lock_guard<mutex> lock(queueLock);
            if (--pendingTasks == 0 && taskQueue.empty()) {
                allDone.signal(); // si no quedan tareas, termino
            }
        }

        {
            lock_guard<mutex> lock(wts[id].lock);
            wts[id].available = true;
        }
    }
}

/**
 * schedule: encola una nueva tarea para ejecutar.
 * @param thunk - función sin argumentos ni retorno.
 */
void ThreadPool::schedule(const function<void(void)>& thunk) {

    if (!thunk) {
        throw invalid_argument("Cannot schedule a null task.");
    }

    if (done) {
        throw runtime_error("Cannot schedule tasks on a destroyed ThreadPool.");
    }
    
    lock_guard<mutex> lock(queueLock); // bloqueo para proteger la cola de tareas
    taskQueue.push(thunk);
    pendingTasks++;
    tasksAvailable.signal(); // avisamos al dispatcher
}

/**
 * wait: bloquea hasta que todas las tareas se hayan ejecutado.
 */
void ThreadPool::wait() {
    lock_guard<mutex> lk(waitLock); // bloqueo para proteger pendingTasks y taskQueue
    while (true) {
        {
            lock_guard<mutex> lock(queueLock);
            if (pendingTasks == 0 && taskQueue.empty()) break; // si no hay tareas pendientes, salgo
        }
        this_thread::sleep_for(chrono::milliseconds(1));
    }
}

/**
 * dispatcher: asigna tareas a workers disponibles.
 */
void ThreadPool::dispatcher() {
    while (!done) {
        tasksAvailable.wait(); // espera nueva tarea
        if (done) break;

        function<void(void)> task; // obtengo una tarea de la cola
        {
            lock_guard<mutex> lock(queueLock);
            if (taskQueue.empty()) continue; // si no hay tareas, sigo esperando
            task = taskQueue.front();
            taskQueue.pop();
        }

        // buscamos un worker libre
        bool assigned = false;
        while (!assigned && !done) {
            for (size_t i = 0; i < wts.size(); ++i) {
                lock_guard<mutex> lock(wts[i].lock);
                if (wts[i].available) {
                    wts[i].available = false;
                    wts[i].thunk = task;
                    wts[i].ready.signal(); // lo activamos
                    assigned = true;
                    break;
                }
            }
            if (!assigned) {
                this_thread::sleep_for(chrono::milliseconds(1)); // esperamos si todos están ocupados
            }
        }
    }
}

/**
 * Destructor: espera a que terminen las tareas y cierra los hilos.
 */
ThreadPool::~ThreadPool() {
    wait(); // espero a que todas las tareas terminen

    done = true; // Marco la pool como destruida

    tasksAvailable.signal(); // libero al dispatcher por si está esperando

    for (worker_t& w : wts) {
        w.ready.signal(); // libero a todos los workers
    }

    if (dt.joinable()) dt.join(); // espero al dispatcher

    for (worker_t& w : wts) {
        if (w.ts.joinable()) w.ts.join(); // espero a que terminen los workers
    }
}
