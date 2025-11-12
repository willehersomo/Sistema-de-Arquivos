#include "estruturas.h"
#include "fs_nucleo.h"



unsigned long hash_djb2(const char *string)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *string++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

int buscar_inode_nome(SISTEMA_ARQUIVOS *fs, INODE *inode_diretorio, const char *nome_entrada)
{
    int bloco_num;
    ENTRADA_DIRETORIO *entradas;
    int entradas_por_bloco;
    int indice_base;
    int indice_atual;
    int num_inode_encontrado;
    int continuar_busca;

    num_inode_encontrado = -1;
    continuar_busca = 1;

    if (inode_diretorio->tipo != 'd' || inode_diretorio->qtd_blocos == 0)
    {
        return -1;
    }

    bloco_num = inode_diretorio->blocos[0];
    entradas = (ENTRADA_DIRETORIO *)ler_bloco(fs, bloco_num);

    if (entradas == NULL)
    {
        return -1;
    }

    entradas_por_bloco = fs->super_bloco_info.tamanho_bloco / sizeof(ENTRADA_DIRETORIO);
    indice_base = hash_djb2(nome_entrada) % entradas_por_bloco;
    indice_atual = indice_base;

    while (indice_atual != INDICE_NULL && entradas[indice_atual].num_inode != 0 && continuar_busca == 1)
    {
        if (strcmp(entradas[indice_atual].nome, nome_entrada) == 0)
        {
            num_inode_encontrado = entradas[indice_atual].num_inode;
            continuar_busca = 0;
        }
        else
        {
            indice_atual = entradas[indice_atual].indice_proximo;
        }
    }

    free(entradas);
    return num_inode_encontrado;
}

void buscar_duplicata_e_slot(SISTEMA_ARQUIVOS *fs, INODE *inode_diretorio, const char *nome_procurado,
                            int *slot_livre_out, int *indice_anterior_out, int *num_inode_duplicata_out)
{
    int bloco_num;
    ENTRADA_DIRETORIO *entradas;
    int entradas_por_bloco;
    int indice_base;
    int indice_atual;
    int i;
    int indice_slot_livre;
    int continuar_busca;

    *slot_livre_out = -1;
    *indice_anterior_out = INDICE_NULL;
    *num_inode_duplicata_out = -1;
    indice_slot_livre = -1;
    continuar_busca = 1;

    bloco_num = inode_diretorio->blocos[0];
    entradas = (ENTRADA_DIRETORIO *)ler_bloco(fs, bloco_num);

    if (entradas != NULL)
    {
        
    

    entradas_por_bloco = fs->super_bloco_info.tamanho_bloco / sizeof(ENTRADA_DIRETORIO);
    indice_base = hash_djb2(nome_procurado) % entradas_por_bloco;

    for (i = 0; i < entradas_por_bloco; i++)
    {
        if (entradas[i].num_inode == 0)
        {
            indice_slot_livre = i;
            i = entradas_por_bloco;
        }
    }
    *slot_livre_out = indice_slot_livre;

    indice_atual = indice_base;

    while (indice_atual != INDICE_NULL && entradas[indice_atual].num_inode != 0 && continuar_busca == 1)
    {
        if (strcmp(entradas[indice_atual].nome, nome_procurado) == 0)
        {
            *num_inode_duplicata_out = entradas[indice_atual].num_inode;
            continuar_busca = 0;
        }
        else
        {
            *indice_anterior_out = indice_atual;
            indice_atual = entradas[indice_atual].indice_proximo;
        }
    }

    free(entradas);
}
}

int comparar_entradas(const void *a, const void *b) {
    const ENTRADA_DIRETORIO *entrada_a;
    const ENTRADA_DIRETORIO *entrada_b;

    entrada_a = (const ENTRADA_DIRETORIO *)a;
    entrada_b = (const ENTRADA_DIRETORIO *)b;

    return strcmp(entrada_a->nome, entrada_b->nome);
}

ENTRADA_DIRETORIO* contar_e_coletar_entradas(SISTEMA_ARQUIVOS *fs, INODE diretorio_atual, int *qtd_entradas_validas_out, ENTRADA_DIRETORIO **entradas_do_bloco_out)
{
    ENTRADA_DIRETORIO *entradas_do_bloco;
    ENTRADA_DIRETORIO *lista_ordenavel;
    int entradas_por_bloco;
    int i;
    int indice_copia;
    int bloco_num;

    lista_ordenavel = NULL;
    *qtd_entradas_validas_out = 0;
    *entradas_do_bloco_out = NULL;

    bloco_num = diretorio_atual.blocos[0];
    entradas_do_bloco = (ENTRADA_DIRETORIO *)ler_bloco(fs, bloco_num);

    if (entradas_do_bloco == NULL)
    {
        return NULL;
    }

    entradas_por_bloco = fs->super_bloco_info.tamanho_bloco / sizeof(ENTRADA_DIRETORIO);
    for (i = 0; i < entradas_por_bloco; i++)
    {
        if (entradas_do_bloco[i].num_inode != 0)
        {
            (*qtd_entradas_validas_out)++;
        }
    }
    
    *entradas_do_bloco_out = entradas_do_bloco;

    if (*qtd_entradas_validas_out == 0)
    {
        return NULL;
    }
    
    lista_ordenavel = (ENTRADA_DIRETORIO *)malloc((*qtd_entradas_validas_out) * sizeof(ENTRADA_DIRETORIO));
    
    if (lista_ordenavel == NULL)
    {
        return NULL;
    }

    indice_copia = 0;
    for (i = 0; i < entradas_por_bloco; i++)
    {
        if (entradas_do_bloco[i].num_inode != 0)
        {
            lista_ordenavel[indice_copia] = entradas_do_bloco[i];
            indice_copia++;
        }
    }

    return lista_ordenavel;
}

void ordenar_e_imprimir_entradas(SISTEMA_ARQUIVOS *fs, ENTRADA_DIRETORIO *lista_ordenavel, int qtd_entradas_validas)
{
    int i;
    INODE inode_entrada;

    qsort(lista_ordenavel, 
          qtd_entradas_validas, 
          sizeof(ENTRADA_DIRETORIO), 
          comparar_entradas);
          
    for (i = 0; i < qtd_entradas_validas; i++)
    {
        inode_entrada = ler_inode(fs, lista_ordenavel[i].num_inode);
        
        printf("%c - %-4d - %-14s - %d\n",
               inode_entrada.tipo,
               lista_ordenavel[i].num_inode,
               lista_ordenavel[i].nome,
               inode_entrada.tamanho);
    }
}

int recursos_novo_diretorio(SISTEMA_ARQUIVOS *fs, int *novo_inode_num, int *novo_bloco_num)
{
    int inode_num;
    int bloco_num;

    inode_num = alocar_inode(fs);
    if (inode_num == -1)
    {
        printf("mkdir: sem inodes livres\n");
        return 0;
    }

    bloco_num = alocar_bloco_dados(fs);
    if (bloco_num == -1)
    {
        printf("mkdir: sem blocos livres\n");
        liberar_inode(fs, inode_num);
        return 0;
    }

    *novo_inode_num = inode_num;
    *novo_bloco_num = bloco_num;
    return 1;
}

void novo_diretorio_inode_bloco(SISTEMA_ARQUIVOS *fs, int novo_inode_num, int novo_bloco_num, int inode_pai_num)
{
    INODE novo_inode = ler_inode(fs, novo_inode_num);
    novo_inode.tipo = 'd';
    novo_inode.tamanho = sizeof(ENTRADA_DIRETORIO)*2; // "." e ".."
    novo_inode.qtd_blocos = 1;
    novo_inode.blocos[0] = novo_bloco_num;
    escrever_inode(fs, novo_inode_num, &novo_inode);

    int entradas_por_bloco = fs->super_bloco_info.tamanho_bloco / sizeof(ENTRADA_DIRETORIO);
    ENTRADA_DIRETORIO *novo_bloco_dados = (ENTRADA_DIRETORIO*) calloc(entradas_por_bloco, sizeof(ENTRADA_DIRETORIO));

    int idx_ponto = hash_djb2(".") % entradas_por_bloco;
    strcpy(novo_bloco_dados[idx_ponto].nome, ".");
    novo_bloco_dados[idx_ponto].num_inode = novo_inode_num;

    int idx_ponto2 = hash_djb2("..") % entradas_por_bloco;
    while (novo_bloco_dados[idx_ponto2].num_inode != 0)
    {
        idx_ponto2 = (idx_ponto2 + 1) % entradas_por_bloco;
    }
    strcpy(novo_bloco_dados[idx_ponto2].nome, "..");
    novo_bloco_dados[idx_ponto2].num_inode = inode_pai_num;

    escrever_bloco_dados(fs, novo_bloco_num, novo_bloco_dados);
    free(novo_bloco_dados);
}


void adicionar_entrada_diretorio_pai(SISTEMA_ARQUIVOS *fs, INODE *inode_pai, int indice_novo_slot, int indice_anterior, const char *nome_dir, int novo_inode_num)
{
    ENTRADA_DIRETORIO *bloco_pai;
    int entradas_por_bloco;
    int indice_base;
    ENTRADA_DIRETORIO entrada_movida;
    int erro_leitura;
    
    erro_leitura = 0;
    
    bloco_pai = (ENTRADA_DIRETORIO*) ler_bloco(fs, inode_pai->blocos[0]);
    if (bloco_pai == NULL)
    {
        printf("mkdir: erro ao ler bloco pai\n");
        erro_leitura = 1;
    }
    
    if (erro_leitura == 0)
    {
        entradas_por_bloco = fs->super_bloco_info.tamanho_bloco / sizeof(ENTRADA_DIRETORIO);
        indice_base = hash_djb2(nome_dir) % entradas_por_bloco;
    
        strcpy(bloco_pai[indice_novo_slot].nome, nome_dir);
        bloco_pai[indice_novo_slot].num_inode = novo_inode_num;
        bloco_pai[indice_novo_slot].indice_proximo = INDICE_NULL;
    
        if (indice_anterior != INDICE_NULL)
        {
            bloco_pai[indice_anterior].indice_proximo = indice_novo_slot;
        }
        else
        {
            if (bloco_pai[indice_base].num_inode == 0) 
            {
                entrada_movida = bloco_pai[indice_novo_slot];
                
                bloco_pai[indice_base] = entrada_movida;
    
                bloco_pai[indice_novo_slot].num_inode = 0;
                bloco_pai[indice_novo_slot].nome[0] = '\0';
                bloco_pai[indice_novo_slot].indice_proximo = INDICE_NULL;
            }
            else
            {
                printf("mkdir: erro logico no encadeamento (slot base ocupado)\n");
            }
        }
    
        escrever_bloco_dados(fs, inode_pai->blocos[0], bloco_pai);
        free(bloco_pai);
    }
}

int ler_conteudo_stdin(char *buffer_conteudo, int capacidade_maxima)
{
    int tamanho_total;
    int bytes_lidos;

    tamanho_total = 0;

    printf("Digite o conteudo. Pressione CTRL+D para salvar.\n");

    while (tamanho_total < capacidade_maxima &&
           (bytes_lidos = read(STDIN_FILENO, buffer_conteudo + tamanho_total, 1)) > 0)
    {
        tamanho_total += bytes_lidos;
    }

    return tamanho_total;
}


int criar_arquivo_conteudo(SISTEMA_ARQUIVOS *fs, int novo_inode_num, char *buffer_conteudo, int tamanho_total)
{
    int blocos_necessarios;
    int i;
    int j;
    int novo_bloco_num;
    int blocos_alocados[PONTEIROS_BLOCO_INODE];
    INODE novo_inode;
    char *dados_bloco;

    blocos_necessarios = 0;
    if (tamanho_total > 0)
    {
        blocos_necessarios = (tamanho_total + fs->super_bloco_info.tamanho_bloco - 1) / fs->super_bloco_info.tamanho_bloco;
    }

    if (blocos_necessarios > PONTEIROS_BLOCO_INODE)
    {
        printf("touch: Arquivo muito grande. Maximo de %d blocos.\n", PONTEIROS_BLOCO_INODE);
        return 0;
    }

    for (i = 0; i < blocos_necessarios; i++)
    {
        novo_bloco_num = alocar_bloco_dados(fs);

        if (novo_bloco_num == -1)
        {
            printf("touch: sem blocos livres\n");

            for (j = 0; j < i; j++)
            {
                liberar_bloco_dados(fs, blocos_alocados[j]);
            }
            return 0;
        }

        blocos_alocados[i] = novo_bloco_num;

        dados_bloco = buffer_conteudo + (i * fs->super_bloco_info.tamanho_bloco);
        escrever_bloco_dados(fs, novo_bloco_num, dados_bloco);
    }

    novo_inode = ler_inode(fs, novo_inode_num);
    novo_inode.tipo = 'f';
    novo_inode.tamanho = tamanho_total;
    novo_inode.qtd_blocos = blocos_necessarios;

    for (i = 0; i < blocos_necessarios; i++)
    {
        novo_inode.blocos[i] = blocos_alocados[i];
    }

    escrever_inode(fs, novo_inode_num, &novo_inode);
    return 1;
}

int validar_e_obter_inode_arquivo(SISTEMA_ARQUIVOS *fs, char *nome_arq)
{
    INODE dir_atual;
    int num_inode_arq;
    INODE inode_arq;

    dir_atual = ler_inode(fs, fs->num_inode_diretorio_atual);
    num_inode_arq = buscar_inode_nome(fs, &dir_atual, nome_arq);

    if (num_inode_arq == -1)
    {
        printf("cat: '%s': Nao encontrado\n", nome_arq);
        return -1;
    }

    inode_arq = ler_inode(fs, num_inode_arq);
    if (inode_arq.tipo != 'f')
    {
        printf("cat: '%s': Nao e um arquivo\n", nome_arq);
        return -2;
    }

    return num_inode_arq;
}

void imprimir_conteudo_arquivo(SISTEMA_ARQUIVOS *fs, INODE *inode_arq)
{
    int bytes_lidos;
    int i;
    char *dados_bloco;
    int bytes_para_ler;
    int bytes_restantes;

    bytes_lidos = 0;

    for (i = 0; i < inode_arq->qtd_blocos; i++)
    {
        dados_bloco = (char *)ler_bloco(fs, inode_arq->blocos[i]);
        if (dados_bloco == NULL)
        {
            printf("cat: Erro ao ler bloco %d\n", inode_arq->blocos[i]);
            i = inode_arq->qtd_blocos;
        }

        bytes_para_ler = fs->super_bloco_info.tamanho_bloco;
        bytes_restantes = inode_arq->tamanho - bytes_lidos;

        if (bytes_para_ler > bytes_restantes)
        {
            bytes_para_ler = bytes_restantes;
        }

        fwrite(dados_bloco, 1, bytes_para_ler, stdout);

        bytes_lidos += bytes_para_ler;
        free(dados_bloco);
    }

    fflush(stdout);
}

int verificar_diretorio_vazio(SISTEMA_ARQUIVOS *fs, INODE *inode_dir)
{
    int bloco_num;
    ENTRADA_DIRETORIO *entradas;
    int entradas_por_bloco;
    int contagem;
    int i;
    int esta_vazio;

    bloco_num = inode_dir->blocos[0];
    entradas = (ENTRADA_DIRETORIO *)ler_bloco(fs, bloco_num);
    if (entradas == NULL)
    {
        printf("rm: erro ao ler bloco de diretorio\n");
        return 0;
    }

    entradas_por_bloco = fs->super_bloco_info.tamanho_bloco / sizeof(ENTRADA_DIRETORIO);
    contagem = 0;

    for (i = 0; i < entradas_por_bloco; i++)
    {
        if (entradas[i].num_inode != 0)
        {
            contagem++;
        }
    }

    free(entradas);

    esta_vazio = (contagem <= 2);
    return esta_vazio;
}

int encontrar_e_validar_alvo_rm(SISTEMA_ARQUIVOS *fs, INODE *dir_atual, char *nome, INODE *inode_alvo_out)
{
    int num_inode_alvo;

    num_inode_alvo = buscar_inode_nome(fs, dir_atual, nome);

    if (num_inode_alvo == -1)
    {
        printf("rm: '%s': Nao encontrado\n", nome);
        return -1;
    }

    *inode_alvo_out = ler_inode(fs, num_inode_alvo);

    if (inode_alvo_out->tipo == 'd')
    {
        if (!verificar_diretorio_vazio(fs, inode_alvo_out))
        {
            printf("rm: '%s': Diretorio nao esta vazio\n", nome);
            return -1;
        }
    }

    return num_inode_alvo;
}

void desalocar_recursos_inode(SISTEMA_ARQUIVOS *fs, INODE *inode_alvo, int num_inode_alvo)
{
    int i;

    for (i = 0; i < inode_alvo->qtd_blocos; i++)
    {
        liberar_bloco_dados(fs, inode_alvo->blocos[i]);
    }

    liberar_inode(fs, num_inode_alvo);
}

void remover_entrada_diretorio_pai(SISTEMA_ARQUIVOS *fs, INODE *dir_atual, char *nome, int num_inode_alvo)
{
    int bloco_pai_num;
    ENTRADA_DIRETORIO *bloco_pai;
    int entradas_por_bloco;
    int indice_base;
    int indice_anterior;
    int indice_atual;
    int encontrado;
    int erro_leitura;
    
    int indice_a_limpar;
    int indice_proximo;

    encontrado = 0;
    erro_leitura = 0;
    indice_anterior = INDICE_NULL;
    indice_a_limpar = INDICE_NULL;
    indice_proximo = INDICE_NULL;

    bloco_pai_num = dir_atual->blocos[0];
    bloco_pai = (ENTRADA_DIRETORIO *)ler_bloco(fs, bloco_pai_num);

    if (bloco_pai == NULL)
    {
        printf("rm: erro critico ao ler bloco pai\n");
        erro_leitura = 1;
    }

    if (erro_leitura == 0)
    {
        entradas_por_bloco = fs->super_bloco_info.tamanho_bloco / sizeof(ENTRADA_DIRETORIO);
        indice_base = hash_djb2(nome) % entradas_por_bloco;
        indice_atual = indice_base;

        while (indice_atual != INDICE_NULL && bloco_pai[indice_atual].num_inode != 0 && encontrado == 0)
        {
            if (bloco_pai[indice_atual].num_inode == num_inode_alvo)
            {
                encontrado = 1;
            }
            else
            {
                indice_anterior = indice_atual;
                indice_atual = bloco_pai[indice_atual].indice_proximo;
            }
        }

        if (encontrado == 1)
        {
            indice_proximo = bloco_pai[indice_atual].indice_proximo;
            indice_a_limpar = indice_atual;

            if (indice_anterior != INDICE_NULL)
            {
                bloco_pai[indice_anterior].indice_proximo = indice_proximo;
            }
            else 
            {
                if (indice_proximo != INDICE_NULL)
                {
                    bloco_pai[indice_atual] = bloco_pai[indice_proximo];
                    indice_a_limpar = indice_proximo;
                }
            }

            bloco_pai[indice_a_limpar].num_inode = 0;
            bloco_pai[indice_a_limpar].nome[0] = '\0';
            bloco_pai[indice_a_limpar].indice_proximo = INDICE_NULL;
        }

        escrever_bloco_dados(fs, bloco_pai_num, bloco_pai);
        free(bloco_pai);
    }
}

int buscar_diretorio(SISTEMA_ARQUIVOS* fs, char* nome_dir)
{
    INODE dir_atual;
    int num_inode_alvo;
    INODE inode_alvo;

    if (strcmp(nome_dir, "/") == 0)
    {
        return fs->super_bloco_info.inode_raiz;
    }

    dir_atual = ler_inode(fs, fs->num_inode_diretorio_atual);
    num_inode_alvo = buscar_inode_nome(fs, &dir_atual, nome_dir);

    if (num_inode_alvo == -1)
    {
        printf("cd: '%s': Nao encontrado\n", nome_dir);
        return -1;
    }

    inode_alvo = ler_inode(fs, num_inode_alvo);
    if (inode_alvo.tipo != 'd')
    {
        printf("cd: '%s': Nao e um diretorio\n", nome_dir);
        return -2;
    }

    return num_inode_alvo;
}

int encontrar_nome_inode_pai(SISTEMA_ARQUIVOS* fs, int num_inode_pai, int num_inode_filho, char* nome_filho_out)
{
    INODE inode_pai;
    int bloco_num;
    ENTRADA_DIRETORIO* entradas;
    int entradas_por_bloco;
    int i;
    int encontrado;
    int continuar_busca;

    entradas = NULL;
    encontrado = 0;
    continuar_busca = 1;
    i = 0;

    inode_pai = ler_inode(fs, num_inode_pai);
    
    if (inode_pai.tipo != 'd' || inode_pai.qtd_blocos == 0) {
        return 0;
    }
    
    bloco_num = inode_pai.blocos[0];
    entradas = (ENTRADA_DIRETORIO*) ler_bloco(fs, bloco_num);

    if (entradas != NULL)
    {
        entradas_por_bloco = fs->super_bloco_info.tamanho_bloco / sizeof(ENTRADA_DIRETORIO);

        while (i < entradas_por_bloco && continuar_busca)
        {
            if (entradas[i].num_inode == num_inode_filho)
            {
                if (strcmp(entradas[i].nome, ".") != 0 && strcmp(entradas[i].nome, "..") != 0)
                {
                    strcpy(nome_filho_out, entradas[i].nome);
                    encontrado = 1;
                    continuar_busca = 0;
                }
            }
            
            if (continuar_busca) {
                i++;
            }
        }
        
        free(entradas); 
    }
    
    return encontrado;
}

char* obter_caminho_absoluto(SISTEMA_ARQUIVOS *fs) 
{
    const int MAX_PATH_LEN = 256; 
    char temp_path[MAX_PATH_LEN];
    char nome_buffer[MAX_PATH_LEN];
    char buffer_path_construido[MAX_PATH_LEN];
    char *resultado;
    
    int inode_atual;
    int inode_raiz;
    int inode_pai_num;
    
    INODE inode_atual_info;
    ENTRADA_DIRETORIO *entradas_atual; 
    int entradas_por_bloco;
    
    int i;
    int encontrado_pai;
    int loop_valido;
    
    strcpy(temp_path, "");
    resultado = NULL;
    loop_valido = 1;
    entradas_atual = NULL;
    
    inode_atual = fs->num_inode_diretorio_atual;
    inode_raiz = fs->super_bloco_info.inode_raiz;
    
    if (inode_atual == inode_raiz) {
        return strdup("/");
    }

    while (inode_atual != inode_raiz && loop_valido) {
        
        inode_atual_info = ler_inode(fs, inode_atual);
        entradas_atual = (ENTRADA_DIRETORIO*) ler_bloco(fs, inode_atual_info.blocos[0]);

        if (entradas_atual == NULL) {
            resultado = strdup("(erro_leitura_dir)");
            loop_valido = 0;
        } else {
            inode_pai_num = -1;
            encontrado_pai = 0;
            i = 0;
            entradas_por_bloco = fs->super_bloco_info.tamanho_bloco / sizeof(ENTRADA_DIRETORIO);
            
            while (i < entradas_por_bloco && encontrado_pai == 0) {
                if (strcmp(entradas_atual[i].nome, "..") == 0) {
                    inode_pai_num = entradas_atual[i].num_inode;
                    encontrado_pai = 1;
                }
                i++;
            }
            
            free(entradas_atual); 
            entradas_atual = NULL;
            
            if (inode_pai_num == -1) { 
                resultado = strdup("(erro_sem_pai)"); 
                loop_valido = 0;
            } else {
                
                if (encontrar_nome_inode_pai(fs, inode_pai_num, inode_atual, nome_buffer)) {
                    
                    snprintf(buffer_path_construido, MAX_PATH_LEN, "/%s%s", nome_buffer, temp_path);
                    strcpy(temp_path, buffer_path_construido);
                    
                    inode_atual = inode_pai_num;
                } else {
                    resultado = strdup("(erro_nome_nao_encontrado)"); 
                    loop_valido = 0;
                }
            }
        }
    }
    
    if (resultado != NULL) {
        return resultado;
    } else {
        return strdup(temp_path);
    }
}