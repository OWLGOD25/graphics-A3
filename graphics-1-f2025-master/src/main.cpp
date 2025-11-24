#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Window.h"
#include "Shader.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

// Graphics Assignment 3
// Name: Tyron Fajardo
// Student #: 101542713

// ------------------------------------------------------------
// Simple math/OBJ data structures
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
    std::vector<Vec3> positions; // "v"  lines
    std::vector<Vec2> tcoords;   // "vt" lines
    std::vector<Vec3> normals;   // "vn" lines
    std::vector<Face> faces;     // "f"  lines
};

struct Vertex
{
    Vec3 position;
    Vec2 uv;
    Vec3 normal;
};

// ------------------------------------------------------------
// Very small OBJ loader for v/vt/vn/f (triangles)
// ------------------------------------------------------------
bool LoadOBJ(const std::string& path, ObjData& out)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open OBJ file: " << path << "\n";
        return false;
    }

    out.positions.clear();
    out.tcoords.clear();
    out.normals.clear();
    out.faces.clear();

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream iss(line);
        std::string type;
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
                std::string token;
                iss >> token; // e.g. "2/1/1"
                if (token.empty())
                    break;

                size_t s1 = token.find('/');
                size_t s2 = token.find('/', s1 + 1);

                int vi = 0;
                int vti = 0;
                int vni = 0;

                if (s1 == std::string::npos)
                {
                    // Only vertex index present
                    vi = std::stoi(token);
                }
                else
                {
                    std::string vStr = token.substr(0, s1);
                    std::string vtStr;
                    std::string vnStr;

                    if (s2 == std::string::npos)
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

                    if (!vStr.empty())  vi = std::stoi(vStr);
                    if (!vtStr.empty()) vti = std::stoi(vtStr);
                    if (!vnStr.empty()) vni = std::stoi(vnStr);
                }

                f.v[i] = vi;
                f.vt[i] = vti;
                f.vn[i] = vni;
            }

            out.faces.push_back(f);
        }
    }

    std::cout << "Loaded OBJ: " << path << "\n";
    std::cout << "  positions: " << out.positions.size() << "\n";
    std::cout << "  tcoords:   " << out.tcoords.size() << "\n";
    std::cout << "  normals:   " << out.normals.size() << "\n";
    std::cout << "  faces:     " << out.faces.size() << "\n";

    return true;
}

// ------------------------------------------------------------
// Convert OBJ data (indexed) to flat OpenGL vertices
// ------------------------------------------------------------
std::vector<Vertex> BuildVerticesFromObj(const ObjData& obj)
{
    std::vector<Vertex> verts;
    verts.reserve(obj.faces.size() * 3);

    for (const Face& f : obj.faces)
    {
        for (int i = 0; i < 3; ++i)
        {
            int vi = f.v[i] - 1; // OBJ indices start at 1
            int vti = f.vt[i] - 1;
            int vni = f.vn[i] - 1;

            Vertex v = {};

            if (vi >= 0 && vi < (int)obj.positions.size())
                v.position = obj.positions[vi];
            else
                v.position = { 0.f, 0.f, 0.f };

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

    std::cout << "Built " << verts.size() << " vertices from OBJ.\n";
    return verts;
}

// ------------------------------------------------------------
// Shaders (inline) - vertex + 2 fragment shaders
// ------------------------------------------------------------

static const char* vertexSrc = R"(
#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec2 vTexCoord;
out vec3 vNormal;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    vTexCoord = aTexCoord;
    vNormal   = aNormal;
}
)";

// Fragment: visualize texture coordinates as color
static const char* fragUVSrc = R"(
#version 430 core
in vec2 vTexCoord;
out vec4 FragColor;

void main()
{
    // UV in [0,1] mapped to color
    FragColor = vec4(vTexCoord, 0.0, 1.0);
}
)";

// Fragment: visualize normals as color
static const char* fragNormalSrc = R"(
#version 430 core
in vec3 vNormal;
out vec4 FragColor;

void main()
{
    vec3 n = normalize(vNormal);
    // map [-1,1] to [0,1]
    FragColor = vec4(n * 0.5 + 0.5, 1.0);
}
)";

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
int main()
{
    std::cout << "Program starting...\n";

    // Create window/context using class-provided helper
    CreateWindow(800, 800, "Graphics Assignment 3 - OBJ Viewer");

    // Load OBJ
    ObjData obj;
    if (!LoadOBJ("plane.obj", obj))
    {
        std::cerr << "ERROR: Could not load plane.obj. "
            << "Make sure it is in the same folder as the .exe (Debug folder).\n";
        std::cout << "Press Enter to exit...\n";
        std::cin.get();
        DestroyWindow();
        return -1;
    }

    // Convert OBJ to flat vertex array
    std::vector<Vertex> vertices = BuildVerticesFromObj(obj);
    if (vertices.empty())
    {
        std::cerr << "ERROR: OBJ has no vertices after conversion.\n";
        std::cout << "Press Enter to exit...\n";
        std::cin.get();
        DestroyWindow();
        return -1;
    }

    // Create VAO/VBO
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
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

    // Enable depth testing (not strictly needed for a single plane, but good practice)
    glEnable(GL_DEPTH_TEST);

    // Create shaders
    Shader shaderUV;
    Shader shaderNormal;
    std::string err;

    if (!shaderUV.CreateFromSource(vertexSrc, fragUVSrc, err))
    {
        std::cerr << "UV shader error:\n" << err << "\n";
        std::cout << "Press Enter to exit...\n";
        std::cin.get();
        DestroyWindow();
        return -1;
    }

    if (!shaderNormal.CreateFromSource(vertexSrc, fragNormalSrc, err))
    {
        std::cerr << "Normal shader error:\n" << err << "\n";
        std::cout << "Press Enter to exit...\n";
        std::cin.get();
        DestroyWindow();
        return -1;
    }

    std::cout << "Initialization complete. "
        << "Press 1 to view UVs, 2 to view normals.\n";

    // Start in UV mode (1)
    int mode = 1;
    GLFWwindow* window = glfwGetCurrentContext();

    // Render loop
    while (!WindowShouldClose())
    {
        // Simple key input: 1 = UV mode, 2 = Normal mode
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
            mode = 1;
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
            mode = 2;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (mode == 1)
            shaderUV.Use();
        else
            shaderNormal.Use();

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
        glBindVertexArray(0);

        Loop(); // from Window.h (swap buffers + poll events)
    }

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    shaderUV.Destroy();
    shaderNormal.Destroy();
    DestroyWindow();

    std::cout << "Program finished. Press Enter to exit...\n";
    std::cin.get();
    return 0;
}
