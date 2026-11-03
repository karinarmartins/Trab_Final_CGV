#version 330 core

// Atributos interpolados pelo rasterizador (veja shader_vertex.glsl)
in vec4 position_world;
in vec4 position_model;
in vec4 normal;
in vec4 normal_model;
in vec2 texcoords;
in vec3 gouraud_diffuse;
in vec3 gouraud_specular;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Materiais. DEVEM ter os mesmos valores do enum Material em cena.cpp.
#define MAT_PISO_MADEIRA   0
#define MAT_PISO_AZULEJO   1
#define MAT_PAREDE_INTERNA 2
#define MAT_PAREDE_TIJOLO  3
#define MAT_MADEIRA        4
#define MAT_TECIDO         5
#define MAT_TECIDO_CLARO   6
#define MAT_CERAMICA       7
#define MAT_METAL          8
#define MAT_TERRENO        9
#define MAT_ESTATUA        10
#define MAT_LAMPADA        11
uniform int object_id;

// Tamanho real (em metros) da peça sendo desenhada. Usado para repetir a
// textura conforme o tamanho, em vez de esticar a imagem.
uniform vec3 object_scale;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Fontes de luz
uniform vec4 sun_direction; // sentido do ponto para o sol (luz direcional)
uniform vec4 lamp_position; // posição da lâmpada (luz pontual)

// Imagens de textura (ordem de carregamento em main.cpp)
uniform sampler2D TextureImage0; // piso de madeira (laminate_floor_02)
uniform sampler2D TextureImage1; // azulejo (floor_tiles_06)
uniform sampler2D TextureImage2; // reboco pintado (painted_plaster_wall)
uniform sampler2D TextureImage3; // tijolo (red_brick)
uniform sampler2D TextureImage4; // madeira dos móveis (wood_table_001)
uniform sampler2D TextureImage5; // tecido (fabric_pattern_07)
uniform sampler2D TextureImage6; // terreno (rocky_terrain_02)

out vec4 color;

#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

const vec3 SUN_COLOR  = vec3(1.00, 0.95, 0.85);
const vec3 LAMP_COLOR = vec3(1.00, 0.75, 0.45);
const vec3 AMBIENT    = vec3(0.22, 0.24, 0.28);

// Projeção planar escolhida pelo eixo dominante da normal (em coordenadas do
// modelo). Para as caixas (cube.obj) isso dá uma projeção planar por face.
// Multiplicamos pela escala da peça para obter coordenadas em metros, e
// dividimos pelo tamanho (em metros) que uma repetição da imagem deve ocupar.
vec2 ProjecaoPlanar(float metros_por_repeticao)
{
    vec3 p = position_model.xyz * object_scale;
    vec3 a = abs(normal_model.xyz);
    vec2 uv;
    if (a.x >= a.y && a.x >= a.z)      uv = p.zy;
    else if (a.y >= a.x && a.y >= a.z) uv = p.xz;
    else                               uv = p.xy;
    return uv / metros_por_repeticao;
}

// Ruído simples para texturas procedurais
float Hash(vec3 p)
{
    return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453);
}

void main()
{
    vec4 camera_position = inverse(view) * vec4(0.0, 0.0, 0.0, 1.0);

    vec4 p = position_world;
    vec4 n = normalize(normal);
    vec4 v = normalize(camera_position - p);

    // Propriedades do material
    vec3  Kd = vec3(0.0);   // refletância difusa (vem da textura)
    vec3  Ks = vec3(0.0);   // refletância especular
    float q  = 1.0;         // expoente especular
    int   modelo = 0;       // 0 = Lambert, 1 = Blinn-Phong, 2 = Gouraud, 3 = emissivo

    if (object_id == MAT_PISO_MADEIRA)
    {
        Kd = texture(TextureImage0, ProjecaoPlanar(1.5)).rgb;
        Ks = vec3(0.15); q = 20.0; modelo = 1;
    }
    else if (object_id == MAT_PISO_AZULEJO)
    {
        Kd = texture(TextureImage1, ProjecaoPlanar(1.0)).rgb;
        Ks = vec3(0.35); q = 60.0; modelo = 1;
    }
    else if (object_id == MAT_PAREDE_INTERNA)
    {
        Kd = texture(TextureImage2, ProjecaoPlanar(2.0)).rgb * vec3(1.0, 0.95, 0.85);
        modelo = 0;
    }
    else if (object_id == MAT_PAREDE_TIJOLO)
    {
        Kd = texture(TextureImage3, ProjecaoPlanar(1.5)).rgb;
        modelo = 0;
    }
    else if (object_id == MAT_MADEIRA)
    {
        Kd = texture(TextureImage4, ProjecaoPlanar(1.0)).rgb;
        Ks = vec3(0.25); q = 25.0; modelo = 1;
    }
    else if (object_id == MAT_TECIDO)
    {
        Kd = texture(TextureImage5, ProjecaoPlanar(0.5)).rgb;
        modelo = 0;
    }
    else if (object_id == MAT_TECIDO_CLARO)
    {
        Kd = mix(texture(TextureImage5, ProjecaoPlanar(0.5)).rgb, vec3(0.9), 0.75);
        modelo = 0;
    }
    else if (object_id == MAT_CERAMICA)
    {
        // Textura procedural: branco com pequenas manchas aleatórias
        vec3 cel = floor(position_model.xyz * object_scale * 80.0);
        Kd = vec3(0.85, 0.86, 0.84) * (0.93 + 0.07 * Hash(cel));
        Ks = vec3(0.6); q = 80.0; modelo = 1;
    }
    else if (object_id == MAT_METAL)
    {
        // Textura procedural: metal escovado (listras finas horizontais)
        float y = position_model.y * object_scale.y;
        float listras = 0.5 + 0.5 * sin(y * 900.0 + 3.0 * Hash(vec3(floor(y * 300.0))));
        Kd = vec3(0.55, 0.57, 0.60) * (0.9 + 0.1 * listras);
        Ks = vec3(0.7); q = 50.0; modelo = 1;
    }
    else if (object_id == MAT_TERRENO)
    {
        Kd = texture(TextureImage6, ProjecaoPlanar(3.0)).rgb;
        modelo = 0;
    }
    else if (object_id == MAT_ESTATUA)
    {
        // Projeção planar XY normalizada pela bounding box (como no Lab 5)
        float U = (position_model.x - bbox_min.x) / (bbox_max.x - bbox_min.x);
        float V = (position_model.y - bbox_min.y) / (bbox_max.y - bbox_min.y);
        Kd = texture(TextureImage6, vec2(U, V)).rgb;
        Ks = vec3(0.3); modelo = 2;
    }
    else if (object_id == MAT_LAMPADA)
    {
        // Projeção esférica (como no Lab 5) da textura de tecido: cúpula
        vec4 c = (bbox_min + bbox_max) / 2.0;
        vec4 d = position_model - c;
        float U = (atan(d.x, d.z) + M_PI) / (2.0 * M_PI);
        float V = (asin(d.y / length(d)) + M_PI_2) / M_PI;
        Kd = mix(texture(TextureImage5, vec2(U, V)).rgb, vec3(1.0, 0.85, 0.6), 0.6);
        modelo = 3;
    }

    vec3 cor;

    if (modelo == 2)
    {
        // Gouraud: iluminação já calculada por vértice e interpolada
        cor = Kd * gouraud_diffuse + Ks * gouraud_specular;
    }
    else if (modelo == 3)
    {
        // Lâmpada: emite luz própria, não depende das outras fontes
        cor = Kd * 1.3;
    }
    else
    {
        // Sol (luz direcional)
        vec4  ls = normalize(sun_direction);
        float difuso_sol = max(0.0, dot(n, ls));

        // Lâmpada (luz pontual com atenuação pela distância)
        vec4  dl  = lamp_position - p;
        float d   = length(dl);
        float att = 1.0 / (1.0 + 0.7 * d * d);
        vec4  ll  = dl / d;
        float difuso_lamp = max(0.0, dot(n, ll));

        // Termo difuso de Lambert + termo ambiente
        cor = Kd * (AMBIENT + SUN_COLOR * difuso_sol + LAMP_COLOR * att * difuso_lamp);

        if (modelo == 1)
        {
            // Blinn-Phong: soma o termo especular (vetor "half-way" h)
            vec4 hs = normalize(ls + v);
            vec4 hl = normalize(ll + v);
            float esp_sol  = (difuso_sol  > 0.0) ? pow(max(0.0, dot(n, hs)), q) : 0.0;
            float esp_lamp = (difuso_lamp > 0.0) ? pow(max(0.0, dot(n, hl)), q) : 0.0;
            cor += Ks * (SUN_COLOR * esp_sol + LAMP_COLOR * att * esp_lamp);
        }
    }

    color.rgb = cor;
    color.a = 1.0;

    // Correção gamma, considerando monitor sRGB.
    color.rgb = pow(color.rgb, vec3(1.0, 1.0, 1.0) / 2.2);
}
