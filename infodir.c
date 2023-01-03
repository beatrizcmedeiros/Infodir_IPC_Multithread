#include "infodir.h"

int main(int argc, char *argv[]){
    return iniciar(argc, argv);
}

int iniciar(int argc, char *argv[]){
    setlocale(LC_ALL, "pt-BR"); 
    metodo(argc, argv, PROCESSO);
    metodo(argc, argv, THREAD);
    return 0;
}

/*
Inicializa a execução e finaliza apresentando o relatorio referente.
    int argc -> quantidade de argumentos passados.
    char *argv[] -> argumentos que forão passados.
    int tipoMetodo -> PROCESSO ou THREAD.
*/
void metodo(int argc, char *argv[], int tipoMetodo){
    //Tempo inicial.
    time_t tempoInicial = time(NULL), segundos;
    time(&segundos);
    tm = localtime(&segundos);
    clock_t contagem[2];
    contagem[0] = clock();
    contagem[1] = clock();
    time_t rawtime;
    struct tm * timeinfo;

    //Memoria compartilhada.
    int segment_id;
	unsigned long long int *shared_memory;
	struct shmid_ds shmbuffer;
	int segment_size;
	const int shared_segment_size = 0x6400;

    //Aloca o espaço de memoria compartilhada.
    segment_id = shmget (IPC_PRIVATE, shared_segment_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    //Aloca a memória compartilhada.
    shared_memory = (unsigned long long int*) shmat (segment_id, 0, 0); 

    Relatorio relatorio;
    MemoriaRelatorio *memoriaRel = inicializaMemoriaRelatorio();
    char path[TAM_MAX_PATH];

    //Armazena o horário que começou a ser executado.
    sprintf(relatorio.inicio, "%.2d:%.2d:%.2d",tm->tm_hour,tm->tm_min,tm->tm_sec);

    //Verifica se foi passado um caminho de diretório de acordo com o número de argumentos
    if(argc > 1){ //Passa o caminho por parâmetro.
        if(tipoMetodo == PROCESSO){
            criaProcesso(argv[1], "", PROCESSO, shared_memory, memoriaRel);
            relatorio.indicador = PROCESSO;  
        }
        if(tipoMetodo == THREAD){
            criaThread(argv[1], "", THREAD, shared_memory, memoriaRel);
            relatorio.indicador = THREAD;
        }
        strcpy(relatorio.diretorio, argv[1]);
    }else{ //Não passa o caminho por parâmetro.
        if(tipoMetodo == PROCESSO){
            criaProcesso(".", "", PROCESSO, shared_memory, memoriaRel);
            relatorio.indicador = PROCESSO;  
        }
        if(tipoMetodo == THREAD){
            criaThread(".", "", PROCESSO, shared_memory, memoriaRel);
            relatorio.indicador = THREAD;
        }
        getcwd(path, TAM_MAX_PATH);
        strcpy(relatorio.diretorio, path);
    }

    //Tempo final.
    time(&segundos);   
    tm = localtime(&segundos);

    //Armazena o horário que terminou a execução.
    sprintf(relatorio.termino, "%.2d:%.2d:%.2d",tm->tm_hour,tm->tm_min,tm->tm_sec); 

    //Calcula qual foi o tempo de duração e armazena na variável.
    double tempoSegundos = difftime(time(NULL),tempoInicial);
    double tempoMS = (contagem[1] - contagem[0]) * 1000.0 / CLOCKS_PER_SEC;
    relatorio.duracao = tempoSegundos; 

    memoriaRel->totalSubDiretorios--; //Excluindo o diretorio raiz 
    relatorio.memoriaRel = memoriaRel;
    exibeRelatorio(relatorio);
}

/*
Cria um processo para percorrer um diretorio especificado.
    char *diretorio -> nome do diretório que será percorrido.
    int tipoMetodo -> metodo PROCESSO ou metodo THREAD.
    unsigned long long int *shared_memory -> memoria compartilhada.
    MemoriaRelatorio *memoriaRelo -> struct que armazena as informações que serão apresentadas no relatório.
*/
void criaProcesso(char *diretorio, char *subdir, int tipoMetodo, unsigned long long int *shared_memory, MemoriaRelatorio *memoriaRel){
    int id;
    id = fork();

    if(id < 0)
        printf("Falha ao executar processo filho!");

    //Processo filho.
    if(id == 0){
        percorreDiretorio(diretorio, subdir, tipoMetodo, shared_memory, memoriaRel);
        exit(0);
    }

    //Processo pai.
    if(id > 0){
        wait(0);
        memoriaRel->tamanhoTotal = shared_memory[0];
        memoriaRel->totalArquivos = shared_memory[1];
        memoriaRel->totalSubDiretorios = shared_memory[2];
    }
}

/*
Cria uma thread para percorrer um diretorio especificado.
    char *diretorio -> nome do diretório que será percorrido.
    int tipoMetodo -> metodo PROCESSO ou metodo THREAD.
    unsigned long long int *shared_memory -> memoria compartilhada.
    MemoriaRelatorio *memoriaRelo -> struct que armazena as informações que serão apresentadas no relatório.
*/
void criaThread(char *diretorio, char *subdir, int tipoMetodo, unsigned long long int *shared_memory, MemoriaRelatorio *memoriaRel){
    thrd_t idThread;
    ParamThread *parametros = malloc(sizeof(ParamThread));;
    int status;

    parametros->diretorio = diretorio;
    parametros->subdir = subdir;
    parametros->tipoMetodo = tipoMetodo;
    parametros->shared_memory = shared_memory;
    parametros->memoriaRel = memoriaRel;

    status = thrd_create(&idThread, (thrd_start_t) gerenciadorThread, (ParamThread *) parametros);
    if (status == thrd_error) {
        printf("ERORR; thrd_create() failed\n");
        exit(EXIT_FAILURE);
    }
    
    thrd_join(idThread, NULL);  
}

/*
Ações que a thread executa. Nesse caso percorrer o diretório especificado e pegar as informações necessárias.
    ParamThread *parametros -> struct com os parametros que devem ser passados para a função de percorrer diretorio,
*/
void gerenciadorThread(ParamThread *parametros){
    percorreDiretorio(parametros->diretorio, parametros->subdir, parametros->tipoMetodo, parametros->shared_memory, parametros->memoriaRel);   

    parametros->memoriaRel->tamanhoTotal = parametros->shared_memory[0];
    parametros->memoriaRel->totalArquivos = parametros->shared_memory[1];
    parametros->memoriaRel->totalSubDiretorios = parametros->shared_memory[2]; 

    thrd_exit(EXIT_SUCCESS);
}

/*
Inicializa a struct que sera utilizada para armazenar as informações que serão apresentadas no relatorio.
    return struct inicializada.
*/
MemoriaRelatorio* inicializaMemoriaRelatorio(){
    MemoriaRelatorio *memoriaRel = malloc(sizeof(MemoriaRelatorio));
    memoriaRel->tamanhoTotal = 0;
    memoriaRel->totalArquivos = 0;
    memoriaRel->totalSubDiretorios = 0; 
    return memoriaRel;
}

/*
Percorre um diretorio especificado e os arquivos que estão dentro dele.
    char *diretorio -> nome do diretório que será percorrido.
    int tipoMetodo -> metodo PROCESSO ou metodo THREAD.
    unsigned long long int *shared_memory -> memoria compartilhada.
    MemoriaRelatorio *memoriaRel -> struct que armazena as informações que serão apresentadas no relatório.
    return -> -1 em caso de erro e 0 de sucesso.
*/
int percorreDiretorio(char *diretorio, char *subdir, int tipoMetodo, unsigned long long int *shared_memory, MemoriaRelatorio *memoriaRel){
    char subdiretorio[PATH_MAX];
    struct dirent* entry;
    DIR* dir = opendir(diretorio);

    if (dir == NULL){
        printf("Não foi possivel abrir o diretório %s\n", diretorio);
        return -1;
    }
    for (;;){
        entry = readdir(dir);
        if (entry == NULL){            
            closedir(dir);
            break;
        }else{
            snprintf(subdiretorio,sizeof(subdiretorio),"%s/%s", diretorio, entry->d_name);  
            if ((strcmp(".",entry->d_name) != 0) && (strcmp("..",entry->d_name) != 0) && (entry->d_type != 4)){         
                shared_memory[0] += calculaTamanhoArquivo(subdiretorio);
                shared_memory[1]++;
            }                          
        }

        //Encontra o subdiretório e armazena o nome na variável.
        snprintf(subdiretorio,sizeof(subdiretorio),"%s/%s", diretorio, entry->d_name);
        if ((strcmp(".",entry->d_name)) && (strcmp("..",entry->d_name)) && (entry->d_type == 4)){   
            if(subdir == "")
                subdir = entry->d_name;
            if(strstr(subdiretorio, subdir) != NULL)
                percorreDiretorio(subdiretorio, subdir, tipoMetodo, shared_memory, memoriaRel);
            else{
                subdir = entry->d_name;
                if(tipoMetodo == PROCESSO)
                    criaProcesso(subdiretorio, subdir, tipoMetodo, shared_memory, memoriaRel); 
                if(tipoMetodo == THREAD)
                    criaThread(subdiretorio, subdir, tipoMetodo, shared_memory, memoriaRel);
            }            
        }
    }
    shared_memory[2]++;
    return 0;
}

/*
Calcula o tamanho do arquivo que foi passado como parâmetro.
    return: tamanho do arquivo recebido.
*/
unsigned long long int calculaTamanhoArquivo(char *nomeArquivo){
    FILE* arq = fopen(nomeArquivo, "r");
  
    if (arq == NULL) {
        printf("Arquvio %s não encontrado ou sem permissão de leitura!\n", nomeArquivo);
        return -1;
    }
    fseek(arq, 0L, SEEK_END);
    unsigned long long int tamanho = ftell(arq);
    fclose(arq);
    return tamanho;
}

/*
Exibe o relatório no prompt.
    Relatorio relatorio -> struct com os dados que serão apresentados no relatório.
*/
void exibeRelatorio(Relatorio relatorio){
    if(relatorio.indicador == PROCESSO)
        printf("\n-Método: IPC - Interprocess Communication\n");
    if(relatorio.indicador == THREAD)
        printf("\n-Método: Multithread\n");

    printf("-Diretório: %s\n\n", relatorio.diretorio);
    printf("-Conteúdo do diretório\n");
    printf("  Arquivos = %d\n", relatorio.memoriaRel->totalArquivos);
    printf("  Subdiretórios = %d\n", relatorio.memoriaRel->totalSubDiretorios);
    printf("  Tamanho do Diretório = %llu bytes\n\n", relatorio.memoriaRel->tamanhoTotal);

    if(relatorio.indicador == PROCESSO)
        printf("-Tempo usando IPC\n");
    if(relatorio.indicador == THREAD)
        printf("-Tempo usando Multithread\n");
    
    printf("  Início.: %s\n", relatorio.inicio);
    printf("  Término: %s\n", relatorio.termino);
    printf("  Duração: %0.0f segundos\n", relatorio.duracao);
    printf("\n");
    if(relatorio.indicador == THREAD)
        exit(EXIT_SUCCESS);
}