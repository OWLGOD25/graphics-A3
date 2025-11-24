#include "Shader.h"
#include "Shader.h"
#include <vector>
#include <iostream>

bool Shader::CompileShader(GLuint shader, const char* src, std::string& errorOut)
{
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> logBuf(logLen ? logLen : 1);
        glGetShaderInfoLog(shader, logLen, nullptr, logBuf.data());
        errorOut = std::string(logBuf.data());
        return false;
    }
    return true;
}

bool Shader::CreateFromSource(const char* vertexSrc, const char* fragmentSrc, std::string& errorOut)
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    if (!CompileShader(vs, vertexSrc, errorOut))
    {
        glDeleteShader(vs);
        return false;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    if (!CompileShader(fs, fragmentSrc, errorOut))
    {
        glDeleteShader(vs);
        glDeleteShader(fs);
        return false;
    }

    ID = glCreateProgram();
    glAttachShader(ID, vs);
    glAttachShader(ID, fs);
    glLinkProgram(ID);

    GLint success = 0;
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLint logLen = 0;
        glGetProgramiv(ID, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> logBuf(logLen ? logLen : 1);
        glGetProgramInfoLog(ID, logLen, nullptr, logBuf.data());
        errorOut = std::string(logBuf.data());
        glDeleteShader(vs);
        glDeleteShader(fs);
        glDeleteProgram(ID);
        ID = 0;
        return false;
    }

    // shaders can be deleted after linking
    glDetachShader(ID, vs);
    glDetachShader(ID, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return true;
}