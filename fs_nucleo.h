#include "bitmap.h"

SISTEMA_ARQUIVOS* alocar_estrutura_fs(void)
{
    SISTEMA_ARQUIVOS* fs;

    fs = (SISTEMA_ARQUIVOS*)malloc(sizeof(SISTEMA_ARQUIVOS));
    if (fs == NULL)
    {
        printf("Erro ao alocar memória para o sistema de arquivos\n");
        return NULL;
    }
    return fs;
}

int abrir_arquivos_dados(SISTEMA_ARQUIVOS* fs)
{
    fs->arquivo_inodes = fopen("fs/inodes.dat", "r+b");
    fs->arquivo_blocos = fopen("fs/blocks.dat", "r+b");

    if (fs->arquivo_inodes == NULL || fs->arquivo_blocos == NULL)
    {
        return 0;
    }
    return 1;
}

int carregar_superbloco(SISTEMA_ARQUIVOS* fs)
{
    FILE* fp_superbloco;

    fp_superbloco = fopen("fs/superblock.dat", "rb");
    if (fp_superbloco == NULL)
    {
        return 0;
    }
    fread(&fs->super_bloco_info, sizeof(SUPER_BLOCO), 1, fp_superbloco);
    fclose(fp_superbloco);
    return 1;
}

int carregar_bitmap(SISTEMA_ARQUIVOS* fs)
{
    int tamanho_bitmap_bytes;
    FILE* fp_free;

    tamanho_bitmap_bytes = (fs->super_bloco_info.total_blocos + 7) / 8;
    fs->mapa_espaco_livre = (unsigned char*)malloc(tamanho_bitmap_bytes);
    if (fs->mapa_espaco_livre == NULL)
    {
        return 0;
    }

    fp_free = fopen("fs/bitmap.dat", "rb");
    if (fp_free == NULL)
    {
        free(fs->mapa_espaco_livre);
        return 0;
    }
    
    fread(fs->mapa_espaco_livre, tamanho_bitmap_bytes, 1, fp_free);
    fclose(fp_free);
    return 1;
}

void desmontar_sistema_arquivos_em_falha(SISTEMA_ARQUIVOS* fs, int etapa)
{
    switch (etapa)
    {
    case 3:
        free(fs->mapa_espaco_livre);
        fclose(fs->arquivo_inodes);
        fclose(fs->arquivo_blocos);
        break;
    case 2:
        fclose(fs->arquivo_inodes);
        fclose(fs->arquivo_blocos);
        break;
    case 1:
        free(fs);
        break;
    }
}


SISTEMA_ARQUIVOS* montar_sistema_arquivos()
{
    SISTEMA_ARQUIVOS* fs;

    fs = alocar_estrutura_fs();
    if (fs == NULL)
    {
        return NULL;
    }

    if (!abrir_arquivos_dados(fs))
    {
        desmontar_sistema_arquivos_em_falha(fs, 1);
        return NULL;
    }

    if (!carregar_superbloco(fs))
    {
        desmontar_sistema_arquivos_em_falha(fs, 2);
        return NULL;
    }

    if (!carregar_bitmap(fs))
    {
        desmontar_sistema_arquivos_em_falha(fs, 3);
        return NULL;
    }

    fs->num_inode_diretorio_atual = fs->super_bloco_info.inode_raiz;

    return fs;
}

void desmontar_sistema_arquivos(SISTEMA_ARQUIVOS* fs)
{
    FILE* fp_free;
    int tamanho_bitmap_bytes;

    if (fs != NULL)
    {
        fp_free = fopen("fs/bitmap.dat", "wb");
        if (fp_free != NULL)
        {
            tamanho_bitmap_bytes = (fs->super_bloco_info.total_blocos + 7) / 8;
            fwrite(fs->mapa_espaco_livre, tamanho_bitmap_bytes, 1, fp_free);
            fclose(fp_free);
        }
        else
        {
            printf("Erro ao salvar bitmap\n");
        }

        free(fs->mapa_espaco_livre);

        if (fs->arquivo_inodes != NULL)
        {
            fclose(fs->arquivo_inodes);
        }
        if (fs->arquivo_blocos != NULL)
        {
            fclose(fs->arquivo_blocos);
        }

        free(fs);
        printf("Sistema de arquivos desmontado\n");
    }
    else
    {
        printf("Sistema de arquivos já está desmontado\n");
    }
}

INODE ler_inode(SISTEMA_ARQUIVOS* fs, int num_inode)
{
    INODE inode_lido;
    long int deslocamento = num_inode * sizeof(INODE);

    if(fseek(fs->arquivo_inodes, deslocamento, SEEK_SET) != 0)
    {
        printf("Erro ao ler o arquivs\n");
        inode_lido.tipo = 'u';
    }
    else
    {
        fread(&inode_lido, sizeof(INODE), 1, fs->arquivo_inodes);
    }
    return inode_lido;
}

void escrever_inode(SISTEMA_ARQUIVOS* fs, int num_inode, INODE *inode)
{
    long int deslocamento = num_inode * sizeof(INODE);
    
    if (fseek(fs->arquivo_inodes, deslocamento, SEEK_SET) != 0) {
        perror("Erro fseek escrever_inode");
    } else {
        fwrite(inode, sizeof(INODE), 1, fs->arquivo_inodes);
    }
}

int alocar_inode(SISTEMA_ARQUIVOS* fs)
{
    int i;
    INODE inode;
    for(i = 1; i < fs->super_bloco_info.total_inodes; i++) 
    {
        inode = ler_inode(fs, i);
        if(inode.tipo == 'u')
        {
            return i;
        }
    }
    return -1;
}
void liberar_inode(SISTEMA_ARQUIVOS* fs, int num_inode)
{
    INODE inode = ler_inode(fs, num_inode);
    inode.tipo = 'u';
    inode.tamanho = 0;
    inode.qtd_blocos = 0;
    escrever_inode(fs, num_inode, &inode);
}

void* ler_bloco(SISTEMA_ARQUIVOS* fs, int num_bloco)
{
    long int deslocamento;
    void* buffer = malloc(fs->super_bloco_info.tamanho_bloco);
    if (buffer == NULL) {
        perror("Erro ao alocar memoria para ler bloco");
        return NULL;
    }
    
    deslocamento = num_bloco * fs->super_bloco_info.tamanho_bloco;

    if (fseek(fs->arquivo_blocos, deslocamento, SEEK_SET) != 0) {
        perror("Erro fseek ler_bloco");
        free(buffer);

        return NULL;
    } 
    else 
    {
        fread(buffer, fs->super_bloco_info.tamanho_bloco, 1, fs->arquivo_blocos);
    }
    return buffer;
}

void escrever_bloco_dados(SISTEMA_ARQUIVOS* fs, int num_bloco, void *dados)
{
    long int deslocamento = num_bloco * fs->super_bloco_info.tamanho_bloco;
    if(fseek(fs->arquivo_blocos, deslocamento, SEEK_SET) != 0) {
        printf("Erro fseek escrever_bloco_dados");
    } else 
    {
        fwrite(dados, fs->super_bloco_info.tamanho_bloco, 1, fs->arquivo_blocos);
    }
}

int alocar_bloco_dados(SISTEMA_ARQUIVOS* fs)
{
    int num_bloco_livre = encontrar_bloco_livre(fs->mapa_espaco_livre, fs->super_bloco_info.total_blocos);

    if(num_bloco_livre == -1)
    {
        printf("Nenhum bloco livre disponível\n");
        return -1;
    }

    marcar_bit_usado(fs->mapa_espaco_livre, num_bloco_livre, fs->super_bloco_info.total_blocos);
    return num_bloco_livre;
}

void liberar_bloco_dados(SISTEMA_ARQUIVOS *fs, int num_bloco)
{
    marcar_bit_livre(fs->mapa_espaco_livre, num_bloco, fs->super_bloco_info.total_blocos);
}

int criar_arquivo_superbloco(SUPER_BLOCO *sb)
{
    FILE *fp;

    fp = fopen("fs/superblock.dat", "wb");
    if (fp == NULL)
    {
        printf("Erro ao criar superbloco\n");
        return 0;
    }

    fwrite(sb, sizeof(SUPER_BLOCO), 1, fp);
    fclose(fp);
    return 1;
}

int criar_arquivo_inodes(SUPER_BLOCO *sb)
{
    FILE *fp;
    INODE inode_vazio;
    int i;

    fp = fopen("fs/inodes.dat", "wb");
    if (fp == NULL)
    {
        printf("Erro ao criar inodes\n");
        return 0;
    }

    inode_vazio.tipo = 'u';
    inode_vazio.tamanho = 0;
    inode_vazio.qtd_blocos = 0;

    for (i = 0; i < sb->total_inodes; i++)
    {
        fwrite(&inode_vazio, sizeof(INODE), 1, fp);
    }
    fclose(fp);
    return 1;
}

int criar_arquivo_blocos(SUPER_BLOCO *sb)
{
    FILE *fp;
    char *bloco_vazio;
    int i;

    fp = fopen("fs/blocks.dat", "wb");
    if (fp == NULL)
    {
        printf("Erro ao criar blocos de dados\n");
        return 0;
    }

    bloco_vazio = (char *)calloc(sb->tamanho_bloco, sizeof(char));
    for (i = 0; i < sb->total_blocos; i++)
    {
        fwrite(bloco_vazio, sb->tamanho_bloco, 1, fp);
    }
    free(bloco_vazio);
    fclose(fp);
    return 1;
}

int criar_arquivo_bitmap(SUPER_BLOCO *sb)
{
    FILE *fp;
    int tamanho_bitmap_bytes;
    unsigned char *bitmap;

    tamanho_bitmap_bytes = (sb->total_blocos + 7) / 8;
    bitmap = (unsigned char *)calloc(tamanho_bitmap_bytes, 1);

    fp = fopen("fs/bitmap.dat", "wb");
    if (fp == NULL)
    {
        printf("Erro ao criar bitmap\n");
        free(bitmap);
        return 0;
    }

    fwrite(bitmap, tamanho_bitmap_bytes, 1, fp);
    fclose(fp);
    free(bitmap);
    return 1;
}

int configurar_diretorio_raiz(void)
{
    SISTEMA_ARQUIVOS *fs;
    int num_inode_raiz;
    int num_bloco_raiz;
    INODE inode_raiz;
    ENTRADA_DIRETORIO entradas[8];
    FILE *fp_sb;
    int i;
    
    for (i = 0; i < 8; i++) {
        entradas[i].nome[0] = '\0';
        entradas[i].num_inode = 0;
        entradas[i].indice_proximo = INDICE_NULL;
    }

    fs = montar_sistema_arquivos();
    if (fs == NULL)
    {
        printf("Erro ao montar sistema de arquivos\n");
        return 0;
    }

    num_inode_raiz = alocar_inode(fs); 

    if (num_inode_raiz == -1)
    {
        printf("Não foi possível alocar inode para a raiz\n");
        desmontar_sistema_arquivos(fs);
        return 0;
    }
    
    fs->super_bloco_info.inode_raiz = num_inode_raiz;

    inode_raiz = ler_inode(fs, num_inode_raiz);
    inode_raiz.tipo = 'd';

    num_bloco_raiz = alocar_bloco_dados(fs);
    if (num_bloco_raiz == -1)
    {
        printf("Não foi possível alocar bloco para a raiz\n");
        desmontar_sistema_arquivos(fs);
        return 0;
    }

    inode_raiz.blocos[0] = num_bloco_raiz;
    inode_raiz.qtd_blocos = 1;

    strcpy(entradas[0].nome, ".");
    entradas[0].num_inode = num_inode_raiz; 
    entradas[0].indice_proximo = INDICE_NULL;

    strcpy(entradas[1].nome, "..");
    entradas[1].num_inode = num_inode_raiz;
    entradas[1].indice_proximo = INDICE_NULL;

    escrever_bloco_dados(fs, num_bloco_raiz, entradas);
    escrever_inode(fs, num_inode_raiz, &inode_raiz);
    
    fp_sb = fopen("fs/superblock.dat", "wb");
    if (fp_sb != NULL) {
        fwrite(&fs->super_bloco_info, sizeof(SUPER_BLOCO), 1, fp_sb); 
        fclose(fp_sb);
    }
    
    desmontar_sistema_arquivos(fs);

    return 1;
}

int inicializar_sistema_arquivos()
{
    SUPER_BLOCO sb;

    printf("Incializando sistema de arquivozn");

    strcpy(sb.sistema_arquivos, "brenimFS");
    sb.tamanho_particao = 10240;
    sb.tamanho_bloco = 128;
    sb.total_blocos = sb.tamanho_particao / sb.tamanho_bloco;
    sb.total_inodes = sb.total_blocos;
    
    sb.inode_raiz = 1; 

    if (criar_arquivo_superbloco(&sb) && criar_arquivo_inodes(&sb) && criar_arquivo_blocos(&sb) && criar_arquivo_bitmap(&sb) && configurar_diretorio_raiz())
    {

        printf("Sistema de arquivos inicializado\n");
        return 1;
    }
    else
    {
        printf("Erro ao inicializar sistema de arquivos\n");
        return 0;
    }
}