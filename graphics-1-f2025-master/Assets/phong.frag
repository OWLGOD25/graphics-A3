#version 460 core

in vec3 position;
in vec3 normal;

out vec4 FragColor;

uniform vec3 u_lightPosition1;
uniform vec3 u_lightColor1;


uniform vec3 u_lightPosition2;
uniform vec3 u_lightColor2;

void main()
{
    // Extra practice: add ambient and specular components based on LearnOpenGL Basic Lighting article!!! 

    vec3 N = normalize(normal);
    vec3 L1 = normalize(u_lightPosition1 - position);
    float dotNL1 = max(dot(N, L1), 0.0);

    vec3 L2 = normalize(u_lightPosition2 - position);
    float dotNL2 = max(dot(N, L2), 0.0);


    //step1
    vec3 diffuse1 = u_lightColor1 * dotNL1;
    //vec3 diffuse = N;

    vec3 diffuse2 = u_lightColor2 * dotNL2;

    vec3 diffuse = diffuse1 + diffuse2;

    FragColor = vec4(diffuse, 1.0);
}