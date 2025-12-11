#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Window.h"
#include "Shader.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>    // for sin, cos, tan, sqrt

using namespace std;

// ============================================================
// Graphics Final Project - Tyron Fajardo (101542713)
// ============================================================

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;

// ------------------------------------------------------------
// Simple math/OBJ data structures (from Assignment 3)
// ------------------------------------------------------------

struct Vec2
{
    float x, y;
};

struct Vec3
{
    float x, y, z;
};

struct Face
{
    int v[3];   // position indices
    int vt[3];  // texcoord indices
    int vn[3];  // normal indices
};

struct ObjData
{
    vector<Vec3> positions; // "v"  lines
    vector<Vec2> tcoords;   // "vt" lines
    vector<Vec3> normals;   // "vn" lines
    vector<Face> faces;     // "f"  lines
};

struct Vertex
{
    Vec3 position;
    Vec2 uv;
    Vec3 normal;
};

// ------------------------------------------------------------
// Tiny vector math helpers (no GLM)
// ------------------------------------------------------------

Vec3 operator+(const Vec3& a, const Vec3& b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec3 operator-(const Vec3& a, const Vec3& b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3 operator*(const Vec3& a, float s)
{
    return { a.x * s, a.y * s, a.z * s };
}

float Dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

Vec3 Normalize(const Vec3& v)
{
    float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
    if (len2 <= 0.0f) return { 0.0f, 0.0f, 0.0f };
    float invLen = 1.0f / sqrtf(len2);
    return { v.x * invLen, v.y * invLen, v.z * invLen };
}

float DegToRad(float deg)
{
    return deg * 3.14159265f / 180.0f;
}

// ------------------------------------------------------------
// Very small OBJ loader for v/vt/vn/f (triangles) - unchanged
// ------------------------------------------------------------
bool LoadOBJ(const string& path, ObjData& out)
{
    ifstream file(path);
    if (!file.is_open())
    {
        cerr << "Failed to open OBJ file: " << path << "\n";
        return false;
    }

    out.positions.clear();
    out.tcoords.clear();
    out.normals.clear();
    out.faces.clear();

    string line;
    while (getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        istringstream iss(line);
        string type;
        iss >> type;

        if (type == "v")
        {
            Vec3 p;
            iss >> p.x >> p.y >> p.z;
            out.positions.push_back(p);
        }
        else if (type == "vt")
        {
            Vec2 t;
            iss >> t.x >> t.y;
            out.tcoords.push_back(t);
        }
        else if (type == "vn")
        {
            Vec3 n;
            iss >> n.x >> n.y >> n.z;
            out.normals.push_back(n);
        }
        else if (type == "f")
        {
            Face f = {};

            for (int i = 0; i < 3; ++i)
            {
                string token;
                iss >> token; // e.g. "2/1/1"
                if (token.empty())
                    break;

                size_t s1 = token.find('/');
                size_t s2 = token.find('/', s1 + 1);

                int vi = 0;
                int vti = 0;
                int vni = 0;

                if (s1 == string::npos)
                {
                    // Only vertex index present
                    vi = stoi(token);
                }
                else
                {
                    string vStr = token.substr(0, s1);
                    string vtStr;
                    string vnStr;

                    if (s2 == string::npos)
                    {
                        // "v/vt"
                        vtStr = token.substr(s1 + 1);
                    }
                    else
                    {
                        // "v/vt/vn"
                        vtStr = token.substr(s1 + 1, s2 - (s1 + 1));
                        vnStr = token.substr(s2 + 1);
                    }

                    if (!vStr.empty())  vi = stoi(vStr);
                    if (!vtStr.empty()) vti = stoi(vtStr);
                    if (!vnStr.empty()) vni = stoi(vnStr);
                }

                f.v[i] = vi;
                f.vt[i] = vti;
                f.vn[i] = vni;
            }

            out.faces.push_back(f);
        }
    }

    cout << "Loaded OBJ: " << path << "\n";
    cout << "  positions: " << out.positions.size() << "\n";
    cout << "  tcoords:   " << out.tcoords.size() << "\n";
    cout << "  normals:   " << out.normals.size() << "\n";
    cout << "  faces:     " << out.faces.size() << "\n";

    return true;
}

// ------------------------------------------------------------
// Convert OBJ data (indexed) to flat OpenGL vertices
// (includes a scale factor to make the Bird bigger)
// ------------------------------------------------------------
vector<Vertex> BuildVerticesFromObj(const ObjData& obj)
{
    vector<Vertex> verts;
    verts.reserve(obj.faces.size() * 3);

    const float scale = 2.5f;   // <--- tweak this if Bird is too small/big

    for (const Face& f : obj.faces)
    {
        for (int i = 0; i < 3; ++i)
        {
            int vi = f.v[i] - 1; // OBJ indices start at 1
            int vti = f.vt[i] - 1;
            int vni = f.vn[i] - 1;

            Vertex v = {};

            if (vi >= 0 && vi < (int)obj.positions.size())
            {
                Vec3 p = obj.positions[vi];
                p.x *= scale;
                p.y *= scale;
                p.z *= scale;
                v.position = p;
            }
            else
            {
                v.position = { 0.f, 0.f, 0.f };
            }

            if (vti >= 0 && vti < (int)obj.tcoords.size())
                v.uv = obj.tcoords[vti];
            else
                v.uv = { 0.f, 0.f };

            if (vni >= 0 && vni < (int)obj.normals.size())
                v.normal = obj.normals[vni];
            else
                v.normal = { 0.f, 0.f, 1.f };

            verts.push_back(v);
        }
    }

    cout << "Built " << verts.size() << " vertices from OBJ.\n";
    return verts;
}

// ------------------------------------------------------------
// Simple 4x4 matrix helpers (row-major, we upload with GL_FALSE)
// ------------------------------------------------------------
void MakeIdentity(float m[16])
{
    for (int i = 0; i < 16; ++i) m[i] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void MakePerspective(float fovDeg, float aspect, float zNear, float zFar, float m[16])
{
    float fovRad = DegToRad(fovDeg);
    float f = 1.0f / tanf(fovRad * 0.5f);

    for (int i = 0; i < 16; ++i) m[i] = 0.0f;

    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zFar + zNear) / (zNear - zFar);
    m[11] = -1.0f;
    m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
}

void MakeLookAt(const Vec3& eye, const Vec3& center, const Vec3& up, float m[16])
{
    Vec3 f = Normalize(center - eye);
    Vec3 s = Normalize(Cross(f, up));
    Vec3 u = Cross(s, f);

    // Row-major
    m[0] = s.x; m[1] = u.x; m[2] = -f.x; m[3] = 0.0f;
    m[4] = s.y; m[5] = u.y; m[6] = -f.y; m[7] = 0.0f;
    m[8] = s.z; m[9] = u.z; m[10] = -f.z; m[11] = 0.0f;
    m[12] = -Dot(s, eye);
    m[13] = -Dot(u, eye);
    m[14] = Dot(f, eye);
    m[15] = 1.0f;
}

// ------------------------------------------------------------
// Simple first-person style camera (WASD + arrows)
// ------------------------------------------------------------
struct Camera
{
    Vec3 position{ 0.0f, 2.0f, 8.0f };
    float yaw = -90.0f;  // facing -Z
    float pitch = 0.0f;
    float moveSpeed = 5.0f;
    float turnSpeed = 60.0f; // degrees per second

    Vec3 GetForward() const
    {
        Vec3 front;
        front.x = cosf(DegToRad(yaw)) * cosf(DegToRad(pitch));
        front.y = sinf(DegToRad(pitch));
        front.z = sinf(DegToRad(yaw)) * cosf(DegToRad(pitch));
        return Normalize(front);
    }

    Vec3 GetRight() const
    {
        return Normalize(Cross(GetForward(), { 0.0f, 1.0f, 0.0f }));
    }

    void GetViewMatrix(float out[16]) const
    {
        Vec3 forward = GetForward();
        Vec3 center = position + forward;
        MakeLookAt(position, center, { 0.0f, 1.0f, 0.0f }, out);
    }
};

void UpdateCameraFromInput(Camera& cam, float dt, GLFWwindow* window)
{
    // movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cam.position = cam.position + cam.GetForward() * (cam.moveSpeed * dt);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cam.position = cam.position - cam.GetForward() * (cam.moveSpeed * dt);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cam.position = cam.position - cam.GetRight() * (cam.moveSpeed * dt);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cam.position = cam.position + cam.GetRight() * (cam.moveSpeed * dt);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cam.position.y += cam.moveSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        cam.position.y -= cam.moveSpeed * dt;

    // looking
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        cam.yaw -= cam.turnSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        cam.yaw += cam.turnSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        cam.pitch += cam.turnSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        cam.pitch -= cam.turnSpeed * dt;

    if (cam.pitch > 89.0f)  cam.pitch = 89.0f;
    if (cam.pitch < -89.0f) cam.pitch = -89.0f;
}

// ------------------------------------------------------------
// Procedural checkerboard texture (safe fallback, no extra libs)
// ------------------------------------------------------------
GLuint CreateCheckerTexture()
{
    const int W = 64;
    const int H = 64;
    vector<unsigned char> data(W * H * 3);

    for (int y = 0; y < H; ++y)
    {
        for (int x = 0; x < W; ++x)
        {
            int idx = (y * W + x) * 3;
            int check = ((x / 8) + (y / 8)) % 2;
            unsigned char v = check ? 230 : 50;
            data[idx + 0] = v;
            data[idx + 1] = v;
            data[idx + 2] = v;
        }
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, W, H, 0,
        GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return tex;
}

/*
    NOTE (optional, if you want to use TEXTURE.jpg):

    If your lab framework has something like:

        void LoadImageFromFile(Image* img, const char* filename);
        void LoadTexture(Texture* tex, const Image& img);

    you could replace CreateCheckerTexture() with a loader that reads
    "TEXTURE.jpg" and uploads it. I'm leaving the checkerboard here
    to guarantee this file compiles and runs on your current setup.
*/

// ------------------------------------------------------------
// Phong vertex & fragment shaders (inline)
// ------------------------------------------------------------
static const char* phongVertexSrc = R"(
#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vFragPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;

    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vTexCoord = aTexCoord;

    gl_Position = uProjection * uView * worldPos;
}
)";

static const char* phongFragSrc = R"(
#version 430 core

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vFragPos;
out vec4 FragColor;

// Material / control
uniform sampler2D uDiffuseMap;
uniform vec3  uBaseColor;
uniform bool  uUseTexture;
uniform bool  uUseLighting;
uniform vec3  uViewPos;

// Directional light
struct DirectionalLight
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// Point light
struct PointLight
{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

// Spotlight
struct SpotLight
{
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

uniform DirectionalLight dirLight;
uniform PointLight       pointLight;
uniform SpotLight        spotLight;

vec3 GetBaseColor()
{
    if (uUseTexture)
    {
        return texture(uDiffuseMap, vTexCoord).rgb;
    }
    else
    {
        return uBaseColor;
    }
}

vec3 CalcDirectional(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 color)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    vec3 ambient  = light.ambient  * color;
    vec3 diffuse  = light.diffuse  * diff * color;
    vec3 specular = light.specular * spec;
    return ambient + diffuse + specular;
}

vec3 CalcPoint(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 color)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant +
                               light.linear * distance +
                               light.quadratic * distance * distance);

    vec3 ambient  = light.ambient  * color;
    vec3 diffuse  = light.diffuse  * diff * color;
    vec3 specular = light.specular * spec;

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}

vec3 CalcSpot(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 color)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant +
                               light.linear * distance +
                               light.quadratic * distance * distance);

    float theta   = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 ambient  = light.ambient  * color;
    vec3 diffuse  = light.diffuse  * diff * color;
    vec3 specular = light.specular * spec;

    ambient  *= attenuation * intensity;
    diffuse  *= attenuation * intensity;
    specular *= attenuation * intensity;

    return ambient + diffuse + specular;
}

void main()
{
    vec3 color = GetBaseColor();

    // Unlit option – used for ground plane
    if (!uUseLighting)
    {
        FragColor = vec4(color, 1.0);
        return;
    }

    vec3 norm    = normalize(vNormal);
    vec3 viewDir = normalize(uViewPos - vFragPos);

    vec3 result = vec3(0.0);

    result += CalcDirectional(dirLight, norm, viewDir, color);
    result += CalcPoint(pointLight, norm, vFragPos, viewDir, color);
    result += CalcSpot(spotLight, norm, vFragPos, viewDir, color);

    FragColor = vec4(result, 1.0);
}
)";

// ------------------------------------------------------------
// Ground plane VAO/VBO
// ------------------------------------------------------------
GLuint gGroundVAO = 0;
GLuint gGroundVBO = 0;

void CreateGroundPlane()
{
    float groundVertices[] = {
        // positions            // uv      // normal (up)
        -50.0f, 0.0f, -50.0f,   0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         50.0f, 0.0f, -50.0f,   1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         50.0f, 0.0f,  50.0f,   1.0f, 1.0f,  0.0f, 1.0f, 0.0f,

        -50.0f, 0.0f, -50.0f,   0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         50.0f, 0.0f,  50.0f,   1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        -50.0f, 0.0f,  50.0f,   0.0f, 1.0f,  0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &gGroundVAO);
    glGenBuffers(1, &gGroundVBO);

    glBindVertexArray(gGroundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gGroundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices),
        groundVertices, GL_STATIC_DRAW);

    GLsizei stride = (3 + 2 + 3) * sizeof(float);

    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

    glEnableVertexAttribArray(1); // uv
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2); // normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));

    glBindVertexArray(0);
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
int main()
{
    cout << "Program starting...\n";

    // Create window/context using class-provided helper
    CreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Graphics Final Project - Lighting");

    // --------------------------------------------------------
    // Load custom OBJ ("Bird.obj")
    // --------------------------------------------------------
    ObjData obj;
    if (!LoadOBJ("Bird.obj", obj))
    {
        cerr << "ERROR: Could not load Bird.obj. "
            << "Make sure it is in the same folder as the .exe.\n";
        cout << "Press Enter to exit...\n";
        cin.get();
        DestroyWindow();
        return -1;
    }

    // Convert OBJ to flat vertex array
    vector<Vertex> vertices = BuildVerticesFromObj(obj);
    if (vertices.empty())
    {
        cerr << "ERROR: OBJ has no vertices after conversion.\n";
        cout << "Press Enter to exit...\n";
        cin.get();
        DestroyWindow();
        return -1;
    }

    // --------------------------------------------------------
    // Create VAO/VBO for Bird mesh
    // --------------------------------------------------------
    GLuint birdVAO = 0, birdVBO = 0;
    glGenVertexArrays(1, &birdVAO);
    glGenBuffers(1, &birdVBO);

    glBindVertexArray(birdVAO);
    glBindBuffer(GL_ARRAY_BUFFER, birdVBO);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW);

    // position: location 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, position));

    // texcoord: location 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, uv));

    // normal: location 2
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, normal));

    glBindVertexArray(0);

    // Ground plane
    CreateGroundPlane();

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // --------------------------------------------------------
    // Create Phong shader
    // --------------------------------------------------------
    Shader phongShader;
    string err;
    if (!phongShader.CreateFromSource(phongVertexSrc, phongFragSrc, err))
    {
        cerr << "Phong shader error:\n" << err << "\n";
        cout << "Press Enter to exit...\n";
        cin.get();
        DestroyWindow();
        return -1;
    }

    // --------------------------------------------------------
    // Create Bird texture (checkerboard for now)
    // --------------------------------------------------------
    GLuint birdTexture = CreateCheckerTexture();

    GLFWwindow* window = glfwGetCurrentContext();
    Camera camera;

    float lastTime = (float)glfwGetTime();

    cout << "Controls:\n";
    cout << "  WASD = move\n";
    cout << "  SPACE / LeftCtrl = up/down\n";
    cout << "  Arrow keys = look around\n";

    // --------------------------------------------------------
    // Render loop
    // --------------------------------------------------------
    while (!WindowShouldClose())
    {
        float currentTime = (float)glfwGetTime();
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        UpdateCameraFromInput(camera, dt, window);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        phongShader.Use();

        GLint prog = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

        // ----------------------------------------------------
        // Matrices
        // ----------------------------------------------------
        float view[16];
        float projection[16];
        float groundModel[16];
        float birdModel[16];

        camera.GetViewMatrix(view);
        MakePerspective(60.0f,
            (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
            0.1f, 100.0f, projection);
        MakeIdentity(groundModel);
        MakeIdentity(birdModel); // Bird at origin

        glUniformMatrix4fv(glGetUniformLocation(prog, "uView"),
            1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(prog, "uProjection"),
            1, GL_FALSE, projection);

        // ----------------------------------------------------
        // Lighting setup (visually distinct)
// ----------------------------------------------------
        // Directional light towards (-1, -1, -1) - soft white "sun"
        glUniform3f(glGetUniformLocation(prog, "dirLight.direction"),
            -1.0f, -1.0f, -1.0f);
        glUniform3f(glGetUniformLocation(prog, "dirLight.ambient"),
            0.15f, 0.15f, 0.15f);
        glUniform3f(glGetUniformLocation(prog, "dirLight.diffuse"),
            0.4f, 0.4f, 0.4f);
        glUniform3f(glGetUniformLocation(prog, "dirLight.specular"),
            0.5f, 0.5f, 0.5f);

        // Point light animated in a circle (radius 10, height 5) - BRIGHT RED
        float radius = 10.0f;
        float speed = 1.0f;
        float lx = cosf(currentTime * speed) * radius;
        float lz = sinf(currentTime * speed) * radius;
        float ly = 5.0f;

        glUniform3f(glGetUniformLocation(prog, "pointLight.position"),
            lx, ly, lz);
        glUniform3f(glGetUniformLocation(prog, "pointLight.ambient"),
            0.02f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(prog, "pointLight.diffuse"),
            1.0f, 0.2f, 0.2f);     // RED – easy to see moving
        glUniform3f(glGetUniformLocation(prog, "pointLight.specular"),
            1.0f, 0.2f, 0.2f);
        glUniform1f(glGetUniformLocation(prog, "pointLight.constant"), 1.0f);
        glUniform1f(glGetUniformLocation(prog, "pointLight.linear"), 0.09f);
        glUniform1f(glGetUniformLocation(prog, "pointLight.quadratic"), 0.032f);

        // Spotlight 5 units above Bird, pointing downward - WARM YELLOW CONE
        glUniform3f(glGetUniformLocation(prog, "spotLight.position"),
            0.0f, 5.0f, 0.0f);
        glUniform3f(glGetUniformLocation(prog, "spotLight.direction"),
            0.0f, -1.0f, 0.0f);
        glUniform3f(glGetUniformLocation(prog, "spotLight.ambient"),
            0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(prog, "spotLight.diffuse"),
            1.0f, 1.0f, 0.7f);     // warm yellowish
        glUniform3f(glGetUniformLocation(prog, "spotLight.specular"),
            1.0f, 1.0f, 0.9f);

        float innerCut = cosf(DegToRad(10.0f));
        float outerCut = cosf(DegToRad(14.0f));
        glUniform1f(glGetUniformLocation(prog, "spotLight.cutOff"), innerCut);
        glUniform1f(glGetUniformLocation(prog, "spotLight.outerCutOff"), outerCut);
        glUniform1f(glGetUniformLocation(prog, "spotLight.constant"), 1.0f);
        glUniform1f(glGetUniformLocation(prog, "spotLight.linear"), 0.09f);
        glUniform1f(glGetUniformLocation(prog, "spotLight.quadratic"), 0.032f);

        // Camera position for specular
        glUniform3f(glGetUniformLocation(prog, "uViewPos"),
            camera.position.x, camera.position.y, camera.position.z);

        // Bind texture sampler to unit 0
        glUniform1i(glGetUniformLocation(prog, "uDiffuseMap"), 0);

        // ----------------------------------------------------
        // Draw ground (flat grey, no lighting)
// ----------------------------------------------------
        glUniformMatrix4fv(glGetUniformLocation(prog, "uModel"),
            1, GL_FALSE, groundModel);
        glUniform1i(glGetUniformLocation(prog, "uUseTexture"), GL_FALSE);
        glUniform1i(glGetUniformLocation(prog, "uUseLighting"), GL_FALSE);
        glUniform3f(glGetUniformLocation(prog, "uBaseColor"),
            0.5f, 0.5f, 0.5f); // grey

        glBindVertexArray(gGroundVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // ----------------------------------------------------
        // Draw Bird mesh (textured, lit)
// ----------------------------------------------------
        glUniformMatrix4fv(glGetUniformLocation(prog, "uModel"),
            1, GL_FALSE, birdModel);
        glUniform1i(glGetUniformLocation(prog, "uUseTexture"), GL_TRUE);
        glUniform1i(glGetUniformLocation(prog, "uUseLighting"), GL_TRUE);
        glUniform3f(glGetUniformLocation(prog, "uBaseColor"),
            1.0f, 1.0f, 1.0f); // multiplied with texture

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, birdTexture);

        glBindVertexArray(birdVAO);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size());
        glBindVertexArray(0);

        // Finish frame
        Loop();
    }

    // Cleanup
    glDeleteBuffers(1, &birdVBO);
    glDeleteVertexArrays(1, &birdVAO);
    glDeleteBuffers(1, &gGroundVBO);
    glDeleteVertexArrays(1, &gGroundVAO);
    glDeleteTextures(1, &birdTexture);

    phongShader.Destroy();
    DestroyWindow();

    cout << "Program finished. Press Enter to exit...\n";
    cin.get();
    return 0;
}
