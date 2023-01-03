#ifndef INFODIR_H
#define INFODIR_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <locale.h>
#include <sys/wait.h>
#include <dirent.h> // Manipulação de diretório.
#include <sys/types.h> // Manipulação de diretório.
#include <time.h> // Referente a hora e data do sistema.
#include <unistd.h> // Relacionado ao fork, pipe, read e write.
#include <sys/shm.h> // Relacionado a memoria compartilhada.
#include <sys/stat.h> // Relacionado a memoria compartilhada.
#include <threads.h> // Requerido pela biblioteca de threads do C11.

#define TAM_MAX_NOME 60
#define TAM_MAX_PATH 200

#define PROCESSO 1
#define THREAD 2

struct tm *tm;
struct tm * timeinfo;

typedef struct{
    int totalArquivos;
    int totalSubDiretorios;
    unsigned long long int tamanhoTotal;
}MemoriaRelatorio;

typedef struct{
    int indicador;
    char inicio[9];
    char termino[9];
    double duracao;
    char diretorio[TAM_MAX_PATH];
    MemoriaRelatorio *memoriaRel;
}Relatorio;

typedef struct {
    char *diretorio; 
    char *subdir;
    int tipoMetodo;
    unsigned long long int *shared_memory;
    MemoriaRelatorio *memoriaRel;
}ParamThread;


int iniciar(int argc, char *argv[]);
MemoriaRelatorio* inicializaMemoriaRelatorio();

void metodo(int argc, char *argv[], int tipoMetodo);

void criaProcesso(char *diretorio, char *subdir, int tipoMetodo, unsigned long long int *shared_memory, MemoriaRelatorio *memoriaRel);
void criaThread(char *diretorio, char *subdir, int tipoMetodo, unsigned long long int *shared_memory, MemoriaRelatorio *memoriaRel);
void gerenciadorThread(ParamThread *parametros);

int percorreDiretorio(char *diretorio, char *subdir, int tipoMetodo, unsigned long long int *shared_memory, MemoriaRelatorio *memoriaRel);
unsigned long long int calculaTamanhoArquivo(char *nomeArquivo);
void exibeRelatorio(Relatorio relatorio);

#endif