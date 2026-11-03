// Montagem e desenho da cena: casa (piso, paredes) e móveis.
//
// Unidades em metros. A casa ocupa x em [-6, 6] e z em [-4, 4], com o piso em
// y = 0. Cômodos:
//   - Sala + cozinha: x em [-6, 1]
//   - Quarto:         x em [ 1, 6], z em [-4, 1]
//   - Banheiro:       x em [ 1, 6], z em [ 1, 4]
// Não há teto, para a câmera poder ver a casa de cima (como no The Sims).

#include <cmath>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "cena.h"
#include "matrices.h"

// Materiais. DEVEM ter os mesmos valores dos #define em shader_vertex.glsl e
// shader_fragment.glsl.
enum Material
{
    MAT_PISO_MADEIRA   = 0,
    MAT_PISO_AZULEJO   = 1,
    MAT_PAREDE_INTERNA = 2,
    MAT_PAREDE_TIJOLO  = 3,
    MAT_MADEIRA        = 4,
    MAT_TECIDO         = 5,
    MAT_TECIDO_CLARO   = 6,
    MAT_CERAMICA       = 7,
    MAT_METAL          = 8,
    MAT_TERRENO        = 9,
    MAT_ESTATUA        = 10,
    MAT_LAMPADA        = 11,
};

std::vector<Movel> g_Moveis;

static const float ALTURA_PAREDE = 2.6f;
static const float PI = 3.14159265f;

// Altura do centro da lâmpada da luminária, em coordenadas locais do móvel.
static const float ALTURA_LAMPADA = 1.65f;

// ---------------------------------------------------------------------------
// Funções auxiliares de desenho

static void EnviarModelo(const glm::mat4& model, int material, glm::vec3 escala)
{
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, material);
    // A escala é usada no shader para repetir a textura conforme o tamanho
    // real da peça, sem esticar a imagem.
    glUniform3f(g_object_scale_uniform, escala.x, escala.y, escala.z);
}

// Desenha uma caixa que vai de "min" até "max", nas coordenadas do sistema
// definido por "base" (identidade = coordenadas do mundo).
static void Caixa(const glm::mat4& base, int material, glm::vec3 min, glm::vec3 max)
{
    glm::vec3 tam = max - min;
    // O cubo unitário (cube.obj) tem base em y = 0 e centro em x = z = 0.
    glm::mat4 model = base
                    * Matrix_Translate((min.x + max.x) / 2.0f, min.y, (min.z + max.z) / 2.0f)
                    * Matrix_Scale(tam.x, tam.y, tam.z);
    EnviarModelo(model, material, tam);
    DrawVirtualObject("the_cube");
}

static void Caixa(int material, glm::vec3 min, glm::vec3 max)
{
    Caixa(Matrix_Identity(), material, min, max);
}

// Parede externa: tijolo por fora e reboco pintado no lado de dentro.
// "dentro" é o eixo (0 = x, 2 = z) e o sentido (+1/-1) que aponta para o
// interior da casa.
static void ParedeExterna(glm::vec3 min, glm::vec3 max, int eixo, float sentido)
{
    Caixa(MAT_PAREDE_TIJOLO, min, max);

    const float e = 0.02f; // espessura do reboco
    glm::vec3 rmin = min, rmax = max;
    if (sentido > 0) { rmin[eixo] = max[eixo];     rmax[eixo] = max[eixo] + e; }
    else             { rmin[eixo] = min[eixo] - e; rmax[eixo] = min[eixo];     }
    Caixa(MAT_PAREDE_INTERNA, rmin, rmax);
}

// ---------------------------------------------------------------------------
// Casa

static void DesenharCasa()
{
    const float H = ALTURA_PAREDE;

    // Terreno em volta da casa (levemente abaixo do piso para evitar z-fighting)
    Caixa(MAT_TERRENO, glm::vec3(-30.0f, -0.10f, -30.0f), glm::vec3(30.0f, -0.01f, 30.0f));

    // Pisos
    Caixa(MAT_PISO_MADEIRA, glm::vec3(-6.0f, -0.05f, -4.0f), glm::vec3(1.0f, 0.0f, 4.0f)); // sala + cozinha
    Caixa(MAT_PISO_MADEIRA, glm::vec3( 1.0f, -0.05f, -4.0f), glm::vec3(6.0f, 0.0f, 1.0f)); // quarto
    Caixa(MAT_PISO_AZULEJO, glm::vec3( 1.0f, -0.05f,  1.0f), glm::vec3(6.0f, 0.0f, 4.0f)); // banheiro

    // Parede norte (z = -4), com janela na sala em x em [-4, -2]
    ParedeExterna(glm::vec3(-6.2f, 0.0f, -4.2f), glm::vec3(-4.0f, H,    -4.0f), 2, +1);
    ParedeExterna(glm::vec3(-2.0f, 0.0f, -4.2f), glm::vec3( 6.2f, H,    -4.0f), 2, +1);
    ParedeExterna(glm::vec3(-4.0f, 0.0f, -4.2f), glm::vec3(-2.0f, 0.9f, -4.0f), 2, +1); // abaixo da janela
    ParedeExterna(glm::vec3(-4.0f, 2.0f, -4.2f), glm::vec3(-2.0f, H,    -4.0f), 2, +1); // acima da janela

    // Parede sul (z = 4), com a porta de entrada em x em [-3, -2]
    ParedeExterna(glm::vec3(-6.2f, 0.0f, 4.0f), glm::vec3(-3.0f, H, 4.2f), 2, -1);
    ParedeExterna(glm::vec3(-2.0f, 0.0f, 4.0f), glm::vec3( 6.2f, H, 4.2f), 2, -1);
    ParedeExterna(glm::vec3(-3.0f, 2.1f, 4.0f), glm::vec3(-2.0f, H, 4.2f), 2, -1);

    // Paredes oeste (x = -6) e leste (x = 6)
    ParedeExterna(glm::vec3(-6.2f, 0.0f, -4.0f), glm::vec3(-6.0f, H, 4.0f), 0, +1);
    ParedeExterna(glm::vec3( 6.0f, 0.0f, -4.0f), glm::vec3( 6.2f, H, 4.0f), 0, -1);

    // Parede interna x = 1 (separa a sala do quarto e do banheiro), com duas portas
    const float x0 = 0.94f, x1 = 1.06f;
    Caixa(MAT_PAREDE_INTERNA, glm::vec3(x0, 0.0f, -4.0f), glm::vec3(x1, H,    -2.6f));
    Caixa(MAT_PAREDE_INTERNA, glm::vec3(x0, 2.1f, -2.6f), glm::vec3(x1, H,    -1.6f)); // porta do quarto
    Caixa(MAT_PAREDE_INTERNA, glm::vec3(x0, 0.0f, -1.6f), glm::vec3(x1, H,     2.0f));
    Caixa(MAT_PAREDE_INTERNA, glm::vec3(x0, 2.1f,  2.0f), glm::vec3(x1, H,     3.0f)); // porta do banheiro
    Caixa(MAT_PAREDE_INTERNA, glm::vec3(x0, 0.0f,  3.0f), glm::vec3(x1, H,     4.0f));

    // Parede interna z = 1 (separa o quarto do banheiro)
    Caixa(MAT_PAREDE_INTERNA, glm::vec3(x1, 0.0f, 0.94f), glm::vec3(6.0f, H, 1.06f));
}

// ---------------------------------------------------------------------------
// Móveis. Cada um é desenhado em coordenadas locais: base no chão (y = 0),
// centrado em x = z = 0 e com a "frente" virada para +z.

static void DesenharMesa(const glm::mat4& M)
{
    Caixa(M, MAT_MADEIRA, glm::vec3(-0.70f, 0.72f, -0.45f), glm::vec3(0.70f, 0.77f, 0.45f)); // tampo
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
        {
            glm::vec3 c(sx * 0.62f, 0.0f, sz * 0.37f);
            Caixa(M, MAT_MADEIRA, c + glm::vec3(-0.03f, 0.0f, -0.03f), c + glm::vec3(0.03f, 0.72f, 0.03f));
        }
}

static void DesenharCadeira(const glm::mat4& M)
{
    Caixa(M, MAT_MADEIRA, glm::vec3(-0.22f, 0.43f, -0.22f), glm::vec3(0.22f, 0.47f, 0.22f)); // assento
    Caixa(M, MAT_MADEIRA, glm::vec3(-0.22f, 0.47f, -0.22f), glm::vec3(0.22f, 0.95f, -0.18f)); // encosto
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
        {
            glm::vec3 c(sx * 0.19f, 0.0f, sz * 0.19f);
            Caixa(M, MAT_MADEIRA, c + glm::vec3(-0.02f, 0.0f, -0.02f), c + glm::vec3(0.02f, 0.43f, 0.02f));
        }
}

static void DesenharSofa(const glm::mat4& M)
{
    Caixa(M, MAT_TECIDO, glm::vec3(-1.00f, 0.00f, -0.42f), glm::vec3( 1.00f, 0.42f,  0.42f)); // assento
    Caixa(M, MAT_TECIDO, glm::vec3(-1.00f, 0.42f, -0.42f), glm::vec3( 1.00f, 0.85f, -0.20f)); // encosto
    Caixa(M, MAT_TECIDO, glm::vec3(-1.00f, 0.42f, -0.20f), glm::vec3(-0.84f, 0.62f,  0.42f)); // braço esq.
    Caixa(M, MAT_TECIDO, glm::vec3( 0.84f, 0.42f, -0.20f), glm::vec3( 1.00f, 0.62f,  0.42f)); // braço dir.
}

static void DesenharCama(const glm::mat4& M)
{
    // Cabeceira em -z
    Caixa(M, MAT_MADEIRA,      glm::vec3(-0.80f, 0.00f, -1.05f), glm::vec3(0.80f, 0.30f,  1.05f)); // estrutura
    Caixa(M, MAT_MADEIRA,      glm::vec3(-0.80f, 0.00f, -1.12f), glm::vec3(0.80f, 1.00f, -1.05f)); // cabeceira
    Caixa(M, MAT_TECIDO,       glm::vec3(-0.75f, 0.30f, -1.00f), glm::vec3(0.75f, 0.50f,  1.00f)); // colchão
    Caixa(M, MAT_TECIDO_CLARO, glm::vec3(-0.65f, 0.50f, -0.95f), glm::vec3(-0.08f, 0.60f, -0.60f)); // travesseiros
    Caixa(M, MAT_TECIDO_CLARO, glm::vec3( 0.08f, 0.50f, -0.95f), glm::vec3( 0.65f, 0.60f, -0.60f));
}

static void DesenharGeladeira(const glm::mat4& M)
{
    Caixa(M, MAT_METAL, glm::vec3(-0.35f, 0.00f, -0.35f), glm::vec3(0.35f, 1.80f, 0.35f));
    Caixa(M, MAT_METAL, glm::vec3( 0.24f, 0.90f,  0.35f), glm::vec3(0.28f, 1.50f, 0.39f)); // puxador
}

static void DesenharBalcao(const glm::mat4& M)
{
    Caixa(M, MAT_MADEIRA,  glm::vec3(-0.90f, 0.00f, -0.30f), glm::vec3(0.90f, 0.85f, 0.30f));
    Caixa(M, MAT_CERAMICA, glm::vec3(-0.92f, 0.85f, -0.32f), glm::vec3(0.92f, 0.90f, 0.32f)); // tampo
}

static void DesenharVasoSanitario(const glm::mat4& M)
{
    Caixa(M, MAT_CERAMICA, glm::vec3(-0.18f, 0.00f, -0.20f), glm::vec3(0.18f, 0.40f,  0.25f)); // base
    Caixa(M, MAT_CERAMICA, glm::vec3(-0.20f, 0.40f, -0.30f), glm::vec3(0.20f, 0.75f, -0.12f)); // caixa d'água
    Caixa(M, MAT_CERAMICA, glm::vec3(-0.19f, 0.40f, -0.12f), glm::vec3(0.19f, 0.43f,  0.27f)); // assento
}

static void DesenharPia(const glm::mat4& M)
{
    Caixa(M, MAT_CERAMICA, glm::vec3(-0.08f, 0.00f, -0.15f), glm::vec3(0.08f, 0.80f, 0.00f)); // coluna
    Caixa(M, MAT_CERAMICA, glm::vec3(-0.30f, 0.80f, -0.25f), glm::vec3(0.30f, 0.90f, 0.15f)); // cuba
}

static void DesenharLuminaria(const glm::mat4& M)
{
    Caixa(M, MAT_METAL, glm::vec3(-0.15f, 0.00f, -0.15f), glm::vec3(0.15f, 0.03f, 0.15f)); // pé
    Caixa(M, MAT_METAL, glm::vec3(-0.02f, 0.03f, -0.02f), glm::vec3(0.02f, ALTURA_LAMPADA - 0.1f, 0.02f)); // haste

    // Cúpula: esfera (sphere.obj tem raio 1)
    glm::mat4 model = M * Matrix_Translate(0.0f, ALTURA_LAMPADA, 0.0f) * Matrix_Scale(0.15f, 0.15f, 0.15f);
    EnviarModelo(model, MAT_LAMPADA, glm::vec3(1.0f));
    DrawVirtualObject("the_sphere");
}

static void DesenharEstatua(const glm::mat4& M)
{
    // Ajusta o coelho para ter 0.3 m de altura, com a base em y = 0.
    glm::vec3 bmin, bmax;
    GetVirtualObjectBBox("the_bunny", bmin, bmax);
    float s = 0.3f / (bmax.y - bmin.y);
    glm::mat4 model = M
                    * Matrix_Scale(s, s, s)
                    * Matrix_Translate(-(bmin.x + bmax.x) / 2.0f, -bmin.y, -(bmin.z + bmax.z) / 2.0f);
    EnviarModelo(model, MAT_ESTATUA, glm::vec3(1.0f));
    DrawVirtualObject("the_bunny");
}

static glm::mat4 MatrizDoMovel(const Movel& m)
{
    return Matrix_Translate(m.posicao.x, m.posicao.y, m.posicao.z) * Matrix_Rotate_Y(m.rotacao_y);
}

static void DesenharMovel(const Movel& m)
{
    glm::mat4 M = MatrizDoMovel(m);
    switch (m.tipo)
    {
        case MESA:           DesenharMesa(M);          break;
        case CADEIRA:        DesenharCadeira(M);       break;
        case SOFA:           DesenharSofa(M);          break;
        case CAMA:           DesenharCama(M);          break;
        case GELADEIRA:      DesenharGeladeira(M);     break;
        case BALCAO:         DesenharBalcao(M);        break;
        case VASO_SANITARIO: DesenharVasoSanitario(M); break;
        case PIA:            DesenharPia(M);           break;
        case LUMINARIA:      DesenharLuminaria(M);     break;
        case ESTATUA:        DesenharEstatua(M);       break;
    }
}

// ---------------------------------------------------------------------------

void Cena_Inicializar()
{
    g_Moveis.clear();

    // Sala de jantar: mesa com três cadeiras (instâncias do mesmo móvel)
    g_Moveis.push_back({MESA,    glm::vec3(-3.5f, 0.0f, 2.0f),  0.0f});
    g_Moveis.push_back({CADEIRA, glm::vec3(-3.5f, 0.0f, 1.35f), 0.0f});
    g_Moveis.push_back({CADEIRA, glm::vec3(-3.5f, 0.0f, 2.65f), PI});
    g_Moveis.push_back({CADEIRA, glm::vec3(-4.45f, 0.0f, 2.0f), PI / 2.0f});
    g_Moveis.push_back({ESTATUA, glm::vec3(-3.5f, 0.77f, 2.0f), 0.4f}); // em cima da mesa

    // Sala de estar
    g_Moveis.push_back({SOFA,      glm::vec3(-3.0f, 0.0f, -3.45f), 0.0f});
    g_Moveis.push_back({LUMINARIA, glm::vec3(-5.5f, 0.0f, -3.5f),  0.0f});

    // Cozinha
    g_Moveis.push_back({BALCAO,    glm::vec3(-0.9f, 0.0f, -3.68f), 0.0f});
    g_Moveis.push_back({GELADEIRA, glm::vec3( 0.4f, 0.0f, -3.55f), 0.0f});

    // Quarto
    g_Moveis.push_back({CAMA, glm::vec3(4.5f, 0.0f, -2.85f), 0.0f});

    // Banheiro
    g_Moveis.push_back({VASO_SANITARIO, glm::vec3(5.45f, 0.0f, 3.0f),  -PI / 2.0f});
    g_Moveis.push_back({PIA,            glm::vec3(3.0f,  0.0f, 3.85f), PI});
}

glm::vec3 Cena_PosicaoLampada()
{
    for (const Movel& m : g_Moveis)
        if (m.tipo == LUMINARIA)
        {
            glm::vec4 p = MatrizDoMovel(m) * glm::vec4(0.0f, ALTURA_LAMPADA, 0.0f, 1.0f);
            return glm::vec3(p.x, p.y, p.z);
        }
    return glm::vec3(0.0f, -100.0f, 0.0f); // sem luminária: luz longe da casa
}

void Cena_Desenhar()
{
    // Luz do sol (direcional): sentido de um ponto da superfície PARA o sol.
    // Vem do lado norte (-z), onde fica a janela da sala.
    glm::vec3 sol = glm::vec3(0.35f, 1.0f, -0.6f);
    glUniform4f(g_sun_direction_uniform, sol.x, sol.y, sol.z, 0.0f);

    glm::vec3 lampada = Cena_PosicaoLampada();
    glUniform4f(g_lamp_position_uniform, lampada.x, lampada.y, lampada.z, 1.0f);

    DesenharCasa();
    for (const Movel& m : g_Moveis)
        DesenharMovel(m);
}
