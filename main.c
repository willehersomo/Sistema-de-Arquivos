#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#ifdef _WIN32
    #include <io.h>    
    #include <stdio.h> 
    #define read _read
    #define STDIN_FILENO _fileno(stdin) //pra rodar no windows
#else
    #include <unistd.h>  
#endif


#include "fs_utils.h"
#include "comandos.h"

void nome_usuario(char *nome_buffer)
{
    printf("Digite seu nome de usuario: ");
    scanf(" %63[^\n]%*c", nome_buffer);
}


SISTEMA_ARQUIVOS *inicializar(void)
{
    SISTEMA_ARQUIVOS *fs;
    char resp_linha;

    fs = montar_sistema_arquivos();

    if (fs == NULL)
    {
        printf("Sistema de arquivos 'BrenimFS' nao encontrado.\n");
        printf("Deseja inicializar (formatar) um novo sistema de arquivos? (s/n): ");

        scanf(" %c", &resp_linha);
        while (getchar() != '\n');

        if (resp_linha == 's' || resp_linha == 'S')
        {
            inicializar_sistema_arquivos();
            printf("Sistema inicializado com sucesso.\n");
            
            fs = montar_sistema_arquivos();
            
            if (fs != NULL)
            {
                return fs;
            }
            else
            {
                printf("FATAL: Falha na montagem\n");
            }
        }
        else
        {
            printf("Saindo.\n");
        }
        return NULL;
    }

    return fs;
}


int executar_comando(SISTEMA_ARQUIVOS *fs, char *comando, char *argumento)
{
    int shell_em_execucao;
    shell_em_execucao = 1;

    if (strcmp(comando, "exit") == 0 || strcmp(comando, "shutdown") == 0)
    {
        shell_em_execucao = 0;
    }
    else if (strcmp(comando, "help") == 0)
    {
        cmd_help();
    }
    else if (strcmp(comando, "ls") == 0)
    {
        cmd_ls(fs);
    }
    else if (strcmp(comando, "mkdir") == 0)
    {
        cmd_mkdir(fs, argumento);
    }
    else if (strcmp(comando, "cd") == 0)
    {
        cmd_cd(fs, argumento);
    }
    else if (strcmp(comando, "pwd") == 0)
    {
        cmd_pwd(fs);
    }
    else if (strcmp(comando, "touch") == 0)
    {
        cmd_touch(fs, argumento);
    }
    else if (strcmp(comando, "cat") == 0)
    {
        cmd_cat(fs, argumento);
    }
    else if (strcmp(comando, "rm") == 0)
    {
        cmd_rm(fs, argumento);
    }
    else if (strcmp(comando, "stat") == 0)
    {
        cmd_stat(fs);
    }
    else
    {
        printf("Comando desconhecido: '%s'. Digite 'help' para ajuda.\n", comando);
    }

    return shell_em_execucao;
}
void processar_loop_shell(SISTEMA_ARQUIVOS *fs, char *nome_usuario)
{
    char linha_input[512];
    char comando[256];
    char argumento[256];
    int shell_em_execucao;
    int scan_retorno;
    int tokens;

    char *caminho_absoluto;

    caminho_absoluto = NULL;
    shell_em_execucao = 1;

    while (shell_em_execucao)
    {
        caminho_absoluto = obter_caminho_absoluto(fs);

        if (caminho_absoluto == NULL) {
            printf("\nErro critico ao obter caminho. Saindo.\n");
            shell_em_execucao = 0; 
        }
        else
        {
            printf("\n");
            printf("%s@BrenimFS:%s$ ", nome_usuario, caminho_absoluto);
            fflush(stdout);

            free(caminho_absoluto);
            caminho_absoluto = NULL;

            scan_retorno = scanf(" %511[^\n]%*c", linha_input);

            if (scan_retorno <= 0)
            {
                printf("\n");
                shell_em_execucao = 0;
            }
            else
            {
                comando[0] = '\0';
                argumento[0] = '\0';
                tokens = sscanf(linha_input, "%s %s", comando, argumento);

                if (tokens > 0)
                {
                    shell_em_execucao = executar_comando(fs, comando, argumento);
                }
            }
        }
    }
}

int main()
{
    char nome_buffer[64];
    SISTEMA_ARQUIVOS *fs;

    nome_usuario(nome_buffer);
    fs = inicializar();

    if (fs != NULL)
    {
        printf("Sistema de arquivos 'BrenimFS' montado. Bem-vindo, %s!\n", nome_buffer); 

        processar_loop_shell(fs, nome_buffer);

        printf("\nDesmontando o sistema de arquivos...\n");
        desmontar_sistema_arquivos(fs);
    }

    return 0;
}
