int verificar_bit(unsigned char *bitmap, int num_bloco, int total_blocos)
{
    int indice_byte;
    int deslocamento_bit;
    int resultado;
    int bloco_valido;

    bloco_valido = 0;
    resultado = -1;

    if (num_bloco >= 0 && num_bloco < total_blocos)
    {
        bloco_valido = 1;
    }

    if (bloco_valido)
    {
        indice_byte = num_bloco / 8;
        deslocamento_bit = num_bloco % 8;
        resultado = (bitmap[indice_byte] & (1 << deslocamento_bit)) != 0;
    }

    return resultado;
}

void marcar_bit_usado(unsigned char *bitmap, int num_bloco, int total_blocos)
{
    int indice_byte;
    int deslocamento_bit;
    int bloco_valido;

    bloco_valido = 0;

    if (num_bloco >= 0 && num_bloco < total_blocos)
    {
        bloco_valido = 1;
    }

    if (bloco_valido)
    {
        indice_byte = num_bloco / 8;
        deslocamento_bit = num_bloco % 8;
        bitmap[indice_byte] |= (1 << deslocamento_bit);
    }
}

void marcar_bit_livre(unsigned char *bitmap, int num_bloco, int total_blocos)
{
    int indice_byte;
    int deslocamento_bit;
    int bloco_valido;

    bloco_valido = 0;

    if (num_bloco >= 0 && num_bloco < total_blocos)
    {
        bloco_valido = 1;
    }

    if (bloco_valido)
    {
        indice_byte = num_bloco / 8;
        deslocamento_bit = num_bloco % 8;
        bitmap[indice_byte] &= ~(1 << deslocamento_bit);
    }
}

int encontrar_bloco_livre(unsigned char *bitmap, int total_blocos)
{
    int i;
    int bloco_encontrado;

    bloco_encontrado = -1;
    i = 0;

    while (i < total_blocos && bloco_encontrado == -1)
    {
        if (verificar_bit(bitmap, i, total_blocos) == 0)
        {
            bloco_encontrado = i;
        }
        else
        {
            i++;
        }
    }

    return bloco_encontrado;
}
