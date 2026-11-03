#version 330 core

// Atributos de vértice recebidos como entrada ("in") pelo Vertex Shader.
// Veja a função BuildTrianglesAndAddToVirtualScene() em "main.cpp".
layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Material do objeto (mesmos valores de shader_fragment.glsl e cena.cpp)
#define MAT_ESTATUA 10
uniform int object_id;

// Fontes de luz (enviadas por Cena_Desenhar() em cena.cpp)
uniform vec4 sun_direction;  // sentido do ponto para o sol
uniform vec4 lamp_position;  // posição da lâmpada (luz pontual)

// Saídas interpoladas pelo rasterizador
out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec4 normal_model;
out vec2 texcoords;

// Iluminação calculada por vértice (interpolação de Gouraud). Só é usada
// pela estátua; nos outros objetos a iluminação é feita por pixel.
out vec3 gouraud_diffuse;
out vec3 gouraud_specular;

// Mesmas cores/constantes de luz usadas em shader_fragment.glsl
const vec3  SUN_COLOR  = vec3(1.00, 0.95, 0.85);
const vec3  LAMP_COLOR = vec3(1.00, 0.75, 0.45);
const vec3  AMBIENT    = vec3(0.22, 0.24, 0.28);

void main()
{
    gl_Position = projection * view * model * model_coefficients;

    position_world = model * model_coefficients;
    position_model = model_coefficients;

    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;
    normal_model = normal_coefficients;
    normal_model.w = 0.0;

    texcoords = texture_coefficients;

    gouraud_diffuse  = vec3(0.0);
    gouraud_specular = vec3(0.0);

    if (object_id == MAT_ESTATUA)
    {
        // Modelo de Blinn-Phong avaliado nos VÉRTICES (Gouraud).
        vec4 camera_position = inverse(view) * vec4(0.0, 0.0, 0.0, 1.0);
        vec4 n = normalize(normal);
        vec4 v = normalize(camera_position - position_world);

        // Sol
        vec4 ls = normalize(sun_direction);
        vec4 hs = normalize(ls + v);
        gouraud_diffuse  += SUN_COLOR * max(0.0, dot(n, ls));
        gouraud_specular += SUN_COLOR * pow(max(0.0, dot(n, hs)), 30.0);

        // Lâmpada
        vec4  dl  = lamp_position - position_world;
        float d   = length(dl);
        float att = 1.0 / (1.0 + 0.7 * d * d);
        vec4  ll  = dl / d;
        vec4  hl  = normalize(ll + v);
        gouraud_diffuse  += LAMP_COLOR * att * max(0.0, dot(n, ll));
        gouraud_specular += LAMP_COLOR * att * pow(max(0.0, dot(n, hl)), 30.0);

        gouraud_diffuse += AMBIENT;
    }
}
