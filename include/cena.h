#ifndef _CENA_H
#define _CENA_H

// Cena do jogo: a casa (piso e paredes) e a lista de móveis.
//
// Cada móvel é composto de várias partes (ex: mesa = tampo + 4 pernas), mas é
// guardado como uma única posição + rotação. Assim, para mover ou girar um
// móvel no modo decoração basta alterar "posicao" ou "rotacao_y" dele em
// g_Moveis; todas as partes acompanham.

#include <vector>
#include <glm/vec3.hpp>
#include <glad/glad.h>

enum TipoMovel
{
    MESA,
    CADEIRA,
    SOFA,
    CAMA,
    GELADEIRA,
    BALCAO,
    VASO_SANITARIO,
    PIA,
    LUMINARIA, // a esfera no topo é a fonte de luz da lâmpada
    ESTATUA,   // coelho (malha complexa) em cima da mesa
};

struct Movel
{
    TipoMovel tipo;
    glm::vec3 posicao;   // posição da base do móvel no chão (y = 0 em geral)
    float     rotacao_y; // rotação em torno do eixo Y, em radianos
};

// Lista de todos os móveis da casa. Pode ser alterada em tempo de execução.
extern std::vector<Movel> g_Moveis;

// Preenche g_Moveis com a disposição inicial da casa.
void Cena_Inicializar();

// Envia as luzes para a GPU e desenha a casa e todos os móveis.
// Deve ser chamada depois de glUseProgram() e de enviar view/projection.
void Cena_Desenhar();

// Posição (no mundo) da lâmpada da luminária, usada como fonte de luz pontual.
glm::vec3 Cena_PosicaoLampada();

// ---------------------------------------------------------------------------
// Definidos em main.cpp e usados pela cena.
void DrawVirtualObject(const char* object_name);
void GetVirtualObjectBBox(const char* object_name, glm::vec3& bbox_min, glm::vec3& bbox_max);

extern GLint g_model_uniform;
extern GLint g_object_id_uniform;
extern GLint g_object_scale_uniform;
extern GLint g_sun_direction_uniform;
extern GLint g_lamp_position_uniform;

#endif // _CENA_H
