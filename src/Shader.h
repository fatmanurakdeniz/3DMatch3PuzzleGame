#pragma once
#include <string>

class Shader {
public:
    unsigned int id = 0;
    Shader() = default;
    Shader(const char* vertPath, const char* fragPath);
    void use() const;
    void setVec4(const char* name, float r, float g, float b, float a) const;
    void setMat4(const char* name, const float* mat) const;
    void setFloat(const char* name, float v) const;
    void setInt(const char* name, int v) const;
private:
    unsigned int compile(unsigned int type, const std::string& src);
};
