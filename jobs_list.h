#include <sys/types.h>
#include <stdio.h>


typedef enum estado_proceso{
    RUNNING,STOPPED,PAUSED
}estado_proceso;

typedef struct proceso_segundo_plano{
    pid_t pid;
    char * nombre;
    enum estado_proceso estado;
}tProceso;

typedef tProceso tElemento;

typedef struct Nodo {
    tElemento info;
    struct Nodo* sig;
} tNodo;

typedef tNodo * tLista;


void crearVacia(tLista * lista);
void construir(tLista * lista, tElemento e);
void mostrarLista(tLista  lista);
void borrarElemento(tLista * L, pid_t pid);
void cambiarEstado(tLista* pL, pid_t pid,estado_proceso nuevoEstado);
pid_t peekPID(tLista *pLista);