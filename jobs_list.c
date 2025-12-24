#include "jobs_list.h"
#include <stdlib.h>



void crearVacia(tLista * lista){
    *lista = NULL;
}

void construir(tLista * plista, tElemento e){
    tNodo * nodo = (tNodo *) malloc(sizeof(tNodo));
    nodo->info = e;
    nodo->sig = *plista;
    *plista = nodo;
}


void mostrar_proceso(tProceso proceso){
    printf("[%d]   ",proceso.pid);

    switch(proceso.estado){
        case 0:
            printf("Running\t\t");
            break;
        case 1:
            printf("Stopped\t\t");
            break;
        case 2:
            printf("Paused\t\t");
            break;
    }
    
    printf("%s\n",proceso.nombre);

}

void mostrarLista(tLista lista) {
    
    tNodo* nodo = lista;
    while (nodo != NULL) {
        mostrar_proceso(nodo->info);
        nodo = nodo->sig;
    }
    printf("\n");
}

void borrarElemento(tLista* pL, pid_t pid) {
    tLista aux = *pL;
    tNodo* anterior = NULL;
    while (aux != NULL) {
        if (aux->info.pid == pid) {
            if (anterior == NULL) { //Caso cabecera
                *pL = aux->sig;
                free(aux);
                aux = *pL;
            } else {
                anterior->sig = aux->sig;
                free(aux);
                aux = anterior->sig;
            }
        } else {
            anterior = aux;
            aux = aux->sig;
        }
    }
}

void cambiarEstado(tLista* pL, pid_t pid,estado_proceso nuevoEstado){
    tLista aux = *pL;
    tNodo* anterior = NULL;
    while (aux != NULL) {
        if (aux->info.pid == pid) {
            aux->info.estado = nuevoEstado;
            return;
        } else {
            anterior = aux;
            aux = aux->sig;
        }
    }

}

pid_t peekPID(tLista *pLista){
    tNodo * cabeza = *pLista;

    if (cabeza == NULL) return -1;
    return cabeza->info.pid;
}