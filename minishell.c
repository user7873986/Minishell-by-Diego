#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include "jobs_list.h"
#include "parser.h"
#define TAM_BUFFER 1024

//Para compilar ->                   gcc jobs_list.c minishell.c libparser_64.a  -o minishell

void inicializar_pipes(int i, int n_commands, int pipes[][2], char * redirect_input , char * redirect_output, char * redirect_error);
void manejador_seniales(int sig);
void ejecutarCd(char * directorio);
void ejecutarUmask(char **argv);
void ejecutarJobs();
void limpiar_jobs();
void ejecutarFg(char **argv);
void imprimirAscii();

tLista procesos_bg;

int main() {

    crearVacia(&procesos_bg);

    imprimirAscii();
    signal(SIGINT, manejador_seniales);

    //Por cada línea hay n comandos, lo que significa n procesos hijos, y n - 1 pipes.
    //Para que el proceso n se comunique con el n + 1, se debe crear un pipe entre ellos.

    while(1){
        limpiar_jobs();
        printf("\nmsh> ");

        char buffer[TAM_BUFFER];
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) continue;

        tline * line = tokenize(buffer);

        pid_t pid[line->ncommands];
        int pipes[line->ncommands - 1][2];   // Pipes para sincronizar a cada hijo
        for(int i = 0; i < line->ncommands - 1; i++){
            pipe(pipes[i]);
        }

        char * mandato = line->commands[0].argv[0];

        if(strcmp(mandato,"cd") == 0){
            ejecutarCd(line->commands[0].argv[1]);
            continue;
        } else if (strcmp(mandato,"exit") == 0){
            return 0;
        } else if(strcmp(mandato,"umask") == 0){
            ejecutarUmask(line->commands[0].argv);
            continue;
        } else if(strcmp(mandato,"jobs") == 0){
            ejecutarJobs();
            continue;
        } else if(strcmp(mandato,"fg") == 0){
            ejecutarFg(line->commands[0].argv);
            continue;
        }

        //Cada comando es un proceso hijo que se debe de comunicar con otros
        for(int i = 0; i < line->ncommands; i++){
            tcommand cmd = line->commands[i];

            if(cmd.filename == NULL) fprintf(stderr, " no se encuentra el mandato\n");
            //Crear un hijo
            pid[i] = fork();
            if(pid[i] < 0){
                fprintf(stderr, "Error al crear el proceso hijo\n");
                exit(1);
            } else if(pid[i] == 0){
                //Proceso hijo
                inicializar_pipes(i,line->ncommands,pipes, line->redirect_input,line->redirect_output,line->redirect_error);

                execvp(cmd.filename, cmd.argv);
                fprintf(stderr, "%s: no se encuentra el mandato\n", cmd.filename);
                exit(1);
            } else {
                //Proceso padre
                if (line->background) {
                    tProceso nuevo_trabajo;
        
                    nuevo_trabajo.pid = pid[i]; 
                    nuevo_trabajo.nombre = strdup(cmd.argv[0]);
                    nuevo_trabajo.estado = RUNNING; 
        
                    construir(&procesos_bg, nuevo_trabajo); 
                }
            }
        }

        for(int j = 0; j < line->ncommands - 1; j++){
            close(pipes[j][0]);
            close(pipes[j][1]);
        }

        //Bucle para asegurarnos de ejecución secuencial
        for (int i = 0; i < line->ncommands; i++) {
            if (!line->background){
                wait(NULL);
            } else {
                printf("[%d]\n", pid[i]);
            }
        }

    }

    return 0;
}


void manejador_seniales(int sig){
    if (sig == SIGINT){
        printf("\n");
        printf("msh> ");
    }
}


void inicializar_pipes(int i, int n_commands, int pipes[][2], char * redirect_input ,char * redirect_output, char * redirect_error){

    if (i > 0) {
        dup2(pipes[i-1][0], STDIN_FILENO);
    }

    if (i < n_commands - 1) {
        dup2(pipes[i][1], STDOUT_FILENO);
    }

    for(int j = 0; j < n_commands - 1; j++){
        close(pipes[j][0]);
        close(pipes[j][1]);
    }

    if (i == 0 && redirect_input != NULL) {
        int df = open(redirect_input, O_RDONLY);
        if (df == -1) {
            fprintf(stderr, "%s: Error al abrir entrada\n", redirect_input);
            exit(1);
        }
        dup2(df, STDIN_FILENO);
        close(df);
    }

    if (i == n_commands - 1 && redirect_output != NULL) {
        int df = open(redirect_output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (df == -1) {
            fprintf(stderr, "%s: Error al abrir salida\n", redirect_output);
            exit(1);
        }
        dup2(df, STDOUT_FILENO);
        close(df);
    }

    if (i == n_commands - 1 && redirect_error != NULL) {
        int df = open(redirect_error, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (df == -1) {
            fprintf(stderr, "%s: Error al abrir archivo de error\n", redirect_error);
            exit(1);
        }
        dup2(df, STDERR_FILENO);
        close(df);
    }
}


void ejecutarCd(char * directorio){

    if (directorio == NULL) directorio = getenv("HOME");

    if (chdir(directorio) != 0){
        fprintf(stderr,"No se encuentra el directorio %s",directorio);
    } else {
        printf("Teletransportado a %s",directorio);
    }
}

void ejecutarUmask(char **argv){
    if (argv[1] == NULL){
        long oldmask = umask(0);
        printf("%04lo",oldmask);
        umask(oldmask);
    } else {
        long permisos = strtol(argv[1],NULL,8);
        if (permisos < 0 || permisos > 0777){
            fprintf(stderr, "umask: %04lo: octal number out of range",permisos);
        }

        umask(permisos);
    }
}

void ejecutarJobs(){
    mostrarLista(procesos_bg);
}



void limpiar_jobs(){
    tNodo * actual = procesos_bg;
    pid_t pid_terminado;

    while(actual != NULL){

        pid_terminado = waitpid(actual->info.pid, NULL, WNOHANG);

        if (pid_terminado == actual->info.pid){
            //Hay un proceso en la lista de jobs que ya ha terminado
            borrarElemento(&procesos_bg, actual->info.pid);
            actual = procesos_bg;
        } else {
            actual = actual->sig;
        }
    }
}

void ejecutarFg(char **argv){
    pid_t target_pid;

    if (argv[1] != NULL){
        target_pid = (pid_t) atoi(argv[1]);
    } else {
        target_pid = peekPID(&procesos_bg);
    }

    if (target_pid <= 0) {
        fprintf(stderr, "%d: no such job\n",target_pid);
        return;
    }
    
    kill(-target_pid,SIGCONT);

    int status;
    waitpid(target_pid,&status,WUNTRACED);

    if (WIFEXITED(status) || WIFSIGNALED(status)){
        //El proceso termino correctamente
        borrarElemento(&procesos_bg,target_pid);
    } else if (WIFSTOPPED(status)){
        //El proceso fue detenido
        cambiarEstado(&procesos_bg,target_pid,STOPPED);
    }

}

void imprimirAscii() {

    printf("Bienvenido a \n");

    printf("              _       _      __         ____   __             ____  _                \n");
    printf("   ____ ___  (_)___  (_)____/ /_  ___  / / /  / /_  __  __   / __ \\(_)__  ____ _____ \n");
    printf("  / __ `__ \\/ / __ \\/ / ___/ __ \\/ _ \\/ / /  / __ \\/ / / /  / / / / / _ \\/ __ `/ __ \\\n");
    printf(" / / / / / / / / / / (__  ) / / /  __/ / /  / /_/ / /_/ /  / /_/ / /  __/ /_/ / /_/ /\n");
    printf("/_/ /_/ /_/_/_/ /_/_/____/_/ /_/\\___/_/_/  /_.___/\\__, /  /_____/_/\\___/\\__, /\\____/ \n");
    printf("                                                 /____/                /____/        \n");
    printf(" \n\nTodos los derechos reservados                                      2025 @BohemianGroove ");
}
