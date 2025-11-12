#define NOME_MAXIMO 14
#define PONTEIROS_BLOCO_INODE 8 
#define INDICE_NULL 65535 

typedef struct {
    char sistema_arquivos[8];      
    uint32_t tamanho_particao;       
    uint16_t tamanho_bloco;        
    uint16_t total_inodes;          
    uint16_t total_blocos;           
    uint16_t inode_raiz;            
} SUPER_BLOCO;

typedef struct {
    uint8_t tipo;                    
    uint32_t tamanho;                
    uint16_t qtd_blocos;            
    uint32_t blocos[PONTEIROS_BLOCO_INODE]; 
} INODE;

typedef struct {
    char nome[NOME_MAXIMO];          
    uint16_t num_inode;             
    uint16_t indice_proximo;    
} ENTRADA_DIRETORIO;

typedef struct {
    FILE *arquivo_inodes;
    FILE *arquivo_blocos;
    SUPER_BLOCO super_bloco_info;
    unsigned char* mapa_espaco_livre;       
    int num_inode_diretorio_atual;        
} SISTEMA_ARQUIVOS;