

void cmd_stat(SISTEMA_ARQUIVOS *fs)
{
    int i;
    int espacos_livres_bytes;
    int blocos_livres = 0;

    for (i = 0; i < fs->super_bloco_info.total_blocos; i++)
    {
        if (verificar_bit(fs->mapa_espaco_livre, i, fs->super_bloco_info.total_blocos) == 0)
        {
            blocos_livres++;
        }
    }

    espacos_livres_bytes = blocos_livres * fs->super_bloco_info.tamanho_bloco;

    printf("Espaço livre: %d bytes\n", espacos_livres_bytes);
    printf("Blocos livres: %d blocos\n", blocos_livres);
    printf("Tamanho do bloco: %d bytes\n", fs->super_bloco_info.tamanho_bloco);
}

void cmd_ls(SISTEMA_ARQUIVOS *fs)
{
    INODE diretorio_atual;
    ENTRADA_DIRETORIO *entradas_do_bloco_original;
    ENTRADA_DIRETORIO *entradas_ordenaveis;
    int entradas_validas;

    entradas_do_bloco_original = NULL;
    entradas_ordenaveis = NULL;
    entradas_validas = 0;

    diretorio_atual = ler_inode(fs, fs->num_inode_diretorio_atual);

    entradas_ordenaveis = contar_e_coletar_entradas(
        fs,
        diretorio_atual,
        &entradas_validas,
        &entradas_do_bloco_original);

    if (entradas_ordenaveis == NULL)
    {
        if (entradas_do_bloco_original == NULL)
        {
            printf("Erro ao tentar ler o conteudo do diretorio.\n");
        }
    }
    else
    {
        ordenar_e_imprimir_entradas(fs, entradas_ordenaveis, entradas_validas);
    }

    if (entradas_do_bloco_original != NULL)
    {
        free(entradas_do_bloco_original);
    }
    if (entradas_ordenaveis != NULL)
    {
        free(entradas_ordenaveis);
    }
}


void cmd_mkdir(SISTEMA_ARQUIVOS *fs, char *nome_dir)
{
    INODE inode_pai;
    int indice_novo_slot;      
    int indice_anterior;        
    int num_inode_duplicata;    
    int novo_inode_num;
    int novo_bloco_num;
    int sucesso;

    if (nome_dir == NULL || strlen(nome_dir) == 0)
    {
        printf("mkdir: nome invalido\n");
    }
    else
    {
        indice_novo_slot = -1;
        indice_anterior = INDICE_NULL;
        num_inode_duplicata = -1;
        sucesso = 0;

        inode_pai = ler_inode(fs, fs->num_inode_diretorio_atual);

        buscar_duplicata_e_slot(fs, &inode_pai, nome_dir, &indice_novo_slot, &indice_anterior, &num_inode_duplicata);

        if (num_inode_duplicata != -1)
        {
            printf("mkdir: '%s' já existe\n", nome_dir);
        }
        else if (indice_novo_slot == -1)
        {
            printf("mkdir: diretorio pai está cheio\n");
        }
        else
        {
            sucesso = recursos_novo_diretorio(fs, &novo_inode_num, &novo_bloco_num);

            if (sucesso)
            {
                novo_diretorio_inode_bloco(fs, novo_inode_num, novo_bloco_num, fs->num_inode_diretorio_atual);

                adicionar_entrada_diretorio_pai(fs, &inode_pai, indice_novo_slot, indice_anterior, nome_dir, novo_inode_num);

                printf("Diretório '%s' criado.\n", nome_dir);
            }
        }
    }
}

void cmd_touch(SISTEMA_ARQUIVOS *fs, char *nome_arq)
{
    INODE inode_pai;
    int indice_novo_slot;
    int indice_anterior;
    int num_inode_duplicata;
    char *buffer_conteudo;
    int tamanho_total;
    int novo_inode_num;
    int CAPACIDADE_BUFFER;
    int alocado_com_sucesso;

    CAPACIDADE_BUFFER = 4096;
    alocado_com_sucesso = 0;
    indice_novo_slot = -1;
    indice_anterior = INDICE_NULL;
    num_inode_duplicata = -1;

    if (nome_arq == NULL || strlen(nome_arq) == 0)
    {
        printf("touch: nome invalido\n");
    }
    else
    {
        inode_pai = ler_inode(fs, fs->num_inode_diretorio_atual);
        
        buscar_duplicata_e_slot(fs, &inode_pai, nome_arq, &indice_novo_slot, &indice_anterior, &num_inode_duplicata);

        if (num_inode_duplicata != -1)
        {
            printf("touch: '%s' já existe\n", nome_arq);
        }
        else if (indice_novo_slot == -1)
        {
            printf("touch: diretorio pai esta cheio\n");
        }
        else
        {
            buffer_conteudo = (char *)malloc(CAPACIDADE_BUFFER);
            if (buffer_conteudo == NULL)
            {
                printf("touch: falha ao alocar buffer de memoria\n");
            }
            else
            {
                tamanho_total = ler_conteudo_stdin(buffer_conteudo, CAPACIDADE_BUFFER);

                novo_inode_num = alocar_inode(fs);
                if (novo_inode_num == -1)
                {
                    printf("touch: sem inodes livres\n");
                    free(buffer_conteudo);
                }
                else
                {
                    if (criar_arquivo_conteudo(fs, novo_inode_num, buffer_conteudo, tamanho_total))
                    {
                        adicionar_entrada_diretorio_pai(fs, &inode_pai, indice_novo_slot, indice_anterior, nome_arq, novo_inode_num);
                        printf("Arquivo '%s' criado.\n", nome_arq);
                    }
                    else
                    {
                        liberar_inode(fs, novo_inode_num);
                    }
                    free(buffer_conteudo);
                }
            }
        }
    }
}

void cmd_cat(SISTEMA_ARQUIVOS *fs, char *nome_arq)
{
    int num_inode_arq;
    INODE inode_arq;

    num_inode_arq = validar_e_obter_inode_arquivo(fs, nome_arq);

    if (num_inode_arq >= 0)
    {

        inode_arq = ler_inode(fs, num_inode_arq);

        if (inode_arq.tamanho > 0)
        {
            imprimir_conteudo_arquivo(fs, &inode_arq);
        }
    }
}

void cmd_rm(SISTEMA_ARQUIVOS *fs, char *nome)
{
    INODE dir_atual;
    INODE inode_alvo;
    int num_inode_alvo;

    if (nome == NULL || strlen(nome) == 0)
    {
        // nada
    }
    else
    {
        if (strcmp(nome, ".") == 0 || strcmp(nome, "..") == 0)
        {
            printf("rm: Nao e permitido remover '.' ou '..'\n");
        }
        else
        {
            dir_atual = ler_inode(fs, fs->num_inode_diretorio_atual);
            num_inode_alvo = encontrar_e_validar_alvo_rm(fs, &dir_atual, nome, &inode_alvo);

            if (num_inode_alvo >= 0)
            {
                desalocar_recursos_inode(fs, &inode_alvo, num_inode_alvo);

                remover_entrada_diretorio_pai(fs, &dir_atual, nome, num_inode_alvo);

                printf("'%s' removido.\n", nome);
            }
        }
    }
}

void cmd_cd(SISTEMA_ARQUIVOS *fs, char *nome_dir)
{
    int num_inode_alvo;

    if (nome_dir == NULL || strlen(nome_dir) == 0)
    {
        printf("cd: argumento faltando\n");
        return;
    }

    num_inode_alvo = buscar_diretorio(fs, nome_dir);

    if (num_inode_alvo >= 0)
    {
        fs->num_inode_diretorio_atual = num_inode_alvo;
    }
}

void cmd_pwd(SISTEMA_ARQUIVOS* fs)
{
    char* caminho;

    caminho = obter_caminho_absoluto(fs);

    if (caminho != NULL)
    {
        printf("%s\n", caminho);
        
        free(caminho);
    }
}

void cmd_help()
{
    printf("--- Ajuda do BrenimFS Shell ---\n");
    printf("Comandos disponiveis:\n\n");
    printf("  help\n");
    printf("    Mostra este menu de ajuda.\n\n");
    printf("  ls\n");
    printf("    Lista os arquivos e diretorios do diretorio atual.\n\n");
    printf("  mkdir <nome>\n");
    printf("    Cria um novo diretorio chamado <nome>.\n\n");
    printf("  cd <caminho>\n");
    printf("    Muda o diretorio atual para <caminho> (ex: '..', 'subdir', '/').\n\n");
    printf("  pwd\n");
    printf("    Mostra o caminho completo do diretorio atual.\n\n");
    printf("  touch <nome>\n");
    printf("    Cria um novo arquivo <nome> e pede o conteudo (CTRL+D para salvar).\n\n");
    printf("  cat <nome>\n");
    printf("    Mostra o conteudo de um arquivo <nome>.\n\n");
    printf("  rm <nome>\n");
    printf("    Remove o arquivo ou diretorio <nome> (so remove diretorios vazios).\n\n");
    printf("  stat\n");
    printf("    Mostra o status de espaco livre e blocos do sistema.\n\n");
    printf("  exit / shutdown\n");
    printf("    Salva o estado do disco e fecha a shell.\n\n");
    printf("---------------------------------\n");
}