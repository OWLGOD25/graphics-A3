#pragma once
#include <string>
#include <glad/glad.h>

class Shader
{
public:
    Shader() : ID(0) {}
    // build shader from source strings
    bool CreateFromSource(const char* vertexSrc, const char* fragmentSrc, std::string& errorOut);
    void Use() const { glUseProgram(ID); }
    GLuint GetID() const { return ID; }
    void Destroy() { if (ID) { glDeleteProgram(ID); ID = 0; } }

private:
    GLuint ID;
    bool CompileShader(GLuint shader, const char* src, std::string& errorOut);
};

