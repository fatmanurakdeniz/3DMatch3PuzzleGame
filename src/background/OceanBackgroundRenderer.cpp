#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_GLCOREARB
#include "background/OceanBackgroundRenderer.h"

#include "Shader.h"
#include "background/BackgroundCamera.h"
#include "background/BackgroundModel.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <jpeglib.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

void setShaderVec3(Shader &shader, const char *name, const glm::vec3 &value) {
  shader.setVec3(name, value.x, value.y, value.z);
}


struct FloorMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint sandTexture = 0;
    GLsizei indexCount = 0;
};

struct TransparentMeshes {
    GLuint surfaceVao = 0;
    GLuint surfaceVbo = 0;
    GLuint surfaceEbo = 0;
    GLsizei surfaceIndexCount = 0;
    GLuint quadVao = 0;
    GLuint quadVbo = 0;
    GLuint waterNormalTexture = 0;
};

struct FullscreenMesh {
    GLuint vao = 0;
};

struct ParticleMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLsizei count = 0;
};

struct InstancedPebbles {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint instanceVbo = 0;
    GLsizei indexCount = 0;
    GLsizei instanceCount = 0;
};

struct ShellMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
};

struct ContactShadowMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLsizei vertexCount = 0;
};

struct ReefFishMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
};

struct SchoolFish {
    glm::vec3 position;
    glm::vec3 velocity;
    float phase = 0.0f;
    float scale = 1.0f;
};

struct DistantFishSchool {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint instanceVbo = 0;
    GLuint phaseVbo = 0;
    GLsizei indexCount = 0;
    std::vector<SchoolFish> fish;
};

GLuint loadJpegTexture(const std::string& path) {
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) {
        throw std::runtime_error("Could not open texture: " + path);
    }

    jpeg_decompress_struct info{};
    jpeg_error_mgr error{};
    info.err = jpeg_std_error(&error);
    jpeg_create_decompress(&info);
    jpeg_stdio_src(&info, file);
    jpeg_read_header(&info, TRUE);
    jpeg_start_decompress(&info);

    const int width = static_cast<int>(info.output_width);
    const int height = static_cast<int>(info.output_height);
    const int channels = static_cast<int>(info.output_components);
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * channels);

    while (info.output_scanline < info.output_height) {
        unsigned char* row = pixels.data()
            + static_cast<size_t>(info.output_scanline) * width * channels;
        jpeg_read_scanlines(&info, &row, 1);
    }

    jpeg_finish_decompress(&info);
    jpeg_destroy_decompress(&info);
    std::fclose(file);

    const GLenum format = channels == 1 ? GL_RED : (channels == 4 ? GL_RGBA : GL_RGB);
    const GLint internalFormat = channels == 1 ? GL_R8 : (channels == 4 ? GL_RGBA : GL_RGB);
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

FloorMesh createFloorMesh() {
    constexpr float size = 100.0f;
    constexpr int resolution = 160;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve((resolution + 1) * (resolution + 1) * 8);
    indices.reserve(resolution * resolution * 6);
    for (int z = 0; z <= resolution; ++z) {
        for (int x = 0; x <= resolution; ++x) {
            const float u = static_cast<float>(x) / resolution;
            const float v = static_cast<float>(z) / resolution;
            vertices.insert(vertices.end(), {
                (u * 2.0f - 1.0f) * size, 0.0f, (v * 2.0f - 1.0f) * size,
                0.0f, 1.0f, 0.0f, u * 50.0f, v * 50.0f
            });
        }
    }
    const unsigned int row = resolution + 1;
    for (int z = 0; z < resolution; ++z) {
        for (int x = 0; x < resolution; ++x) {
            const unsigned int a = z * row + x;
            const unsigned int b = a + 1;
            const unsigned int c = a + row;
            const unsigned int d = c + 1;
            indices.insert(indices.end(), {a, d, b, a, c, d});
        }
    }

    FloorMesh mesh;
    mesh.indexCount = static_cast<GLsizei>(indices.size());
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    constexpr GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    mesh.sandTexture = loadJpegTexture("assets/background/textures/sand_diffuse.jpg");

    return mesh;
}

FullscreenMesh createFullscreenMesh() {
    FullscreenMesh mesh;
    glGenVertexArrays(1, &mesh.vao);
    return mesh;
}

ParticleMesh createParticleMesh() {
    constexpr int particleCount = 1200;
    std::vector<float> particles;
    particles.reserve(particleCount * 4);

    for (int i = 0; i < particleCount; ++i) {
        const float fi = static_cast<float>(i);
        const float x = std::fmod(std::sin(fi * 12.9898f) * 43758.5453f, 1.0f);
        const float y = std::fmod(std::sin(fi * 78.233f) * 23421.631f, 1.0f);
        const float z = std::fmod(std::sin(fi * 39.425f) * 19341.173f, 1.0f);
        const float phase = std::fmod(std::sin(fi * 19.173f) * 11731.743f, 1.0f);
        particles.push_back((x - std::floor(x)) * 46.0f - 23.0f);
        particles.push_back((y - std::floor(y)) * 7.2f + 0.4f);
        particles.push_back(-(z - std::floor(z)) * 55.0f + 8.0f);
        particles.push_back((phase - std::floor(phase)) * 6.2831853f);
    }

    ParticleMesh mesh;
    mesh.count = particleCount;
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(float), particles.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return mesh;
}

InstancedPebbles createInstancedPebbles() {
    constexpr float vertices[] = {
         0.0f, 0.65f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.0f,-0.45f, 0.0f,  0.0f,-1.0f, 0.0f,
         0.7f, 0.0f, 0.0f,  1.0f, 0.2f, 0.0f,
        -0.7f, 0.0f, 0.0f, -1.0f, 0.2f, 0.0f,
         0.0f, 0.0f, 0.7f,  0.0f, 0.2f, 1.0f,
         0.0f, 0.0f,-0.7f,  0.0f, 0.2f,-1.0f
    };
    constexpr unsigned int indices[] = {
        0,2,4, 0,4,3, 0,3,5, 0,5,2,
        1,4,2, 1,3,4, 1,5,3, 1,2,5
    };
    std::vector<glm::mat4> instances;
    instances.reserve(180);
    for (int i = 0; i < 180; ++i) {
        const float fi = static_cast<float>(i);
        const float rx = std::fmod(std::sin(fi * 12.9898f) * 43758.5453f, 1.0f);
        const float rz = std::fmod(std::sin(fi * 78.233f) * 23421.631f, 1.0f);
        const float rs = std::fmod(std::sin(fi * 39.425f) * 19341.173f, 1.0f);
        const float x = (rx - std::floor(rx)) * 30.0f - 15.0f;
        const float z = -(rz - std::floor(rz)) * 30.0f + 2.0f;
        if (std::abs(x) < 3.0f && z > -16.0f && i % 3 != 0) {
            continue;
        }
        const float scale = 0.08f + (rs - std::floor(rs)) * 0.22f;
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(x, -0.08f, z));
        model = glm::rotate(model, fi * 1.37f, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(scale, scale * 0.65f, scale * 0.9f));
        instances.push_back(model);
    }

    InstancedPebbles mesh;
    mesh.indexCount = static_cast<GLsizei>(sizeof(indices) / sizeof(indices[0]));
    mesh.instanceCount = static_cast<GLsizei>(instances.size());
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);
    glGenBuffers(1, &mesh.instanceVbo);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(glm::mat4), instances.data(), GL_STATIC_DRAW);
    for (int column = 0; column < 4; ++column) {
        glVertexAttribPointer(2 + column, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void*>(column * sizeof(glm::vec4)));
        glEnableVertexAttribArray(2 + column);
        glVertexAttribDivisor(2 + column, 1);
    }
    glBindVertexArray(0);
    return mesh;
}

ShellMesh createShellMesh(int type) {
    std::vector<glm::vec3> positions;
    std::vector<unsigned int> indices;

    if (type == 0) {
        constexpr int pathSegments = 30;
        constexpr int tubeSegments = 12;
        for (int path = 0; path <= pathSegments; ++path) {
            const float t = static_cast<float>(path) / pathSegments;
            const float angle = t * glm::two_pi<float>() * 2.15f;
            const float radius = 0.10f + t * 0.66f;
            const glm::vec3 center(
                std::cos(angle) * radius,
                0.10f + (1.0f - t) * 0.22f + std::sin(angle * 3.0f) * 0.015f,
                std::sin(angle) * radius * 0.78f
            );
            const float tubeRadius = 0.18f - t * 0.075f;
            for (int segment = 0; segment < tubeSegments; ++segment) {
                const float tubeAngle = glm::two_pi<float>() * segment / tubeSegments;
                const float rib = 1.0f + std::sin(angle * 7.0f + tubeAngle) * 0.08f;
                positions.push_back(center + glm::vec3(
                    std::cos(tubeAngle) * tubeRadius * rib,
                    std::sin(tubeAngle) * tubeRadius * 0.72f * rib,
                    std::cos(tubeAngle + 0.55f) * tubeRadius * 0.28f
                ));
            }
        }
        for (int path = 0; path < pathSegments; ++path) {
            for (int segment = 0; segment < tubeSegments; ++segment) {
                const unsigned int a = path * tubeSegments + segment;
                const unsigned int b = path * tubeSegments + (segment + 1) % tubeSegments;
                const unsigned int c = (path + 1) * tubeSegments + segment;
                const unsigned int d = (path + 1) * tubeSegments + (segment + 1) % tubeSegments;
                indices.insert(indices.end(), {a, b, d, a, d, c});
            }
        }
    } else if (type == 1 || type == 2) {
        constexpr int radialSegments = 15;
        constexpr int angleSegments = 28;
        const float ribCount = type == 1 ? 11.0f : 15.0f;
        for (int radial = 0; radial <= radialSegments; ++radial) {
            const float r = static_cast<float>(radial) / radialSegments;
            for (int segment = 0; segment <= angleSegments; ++segment) {
                const float u = static_cast<float>(segment) / angleSegments;
                const float angle = -1.42f + u * 2.84f;
                const float asymmetry = 1.0f + 0.055f * std::sin(angle * 3.3f + type);
                const float scallopedEdge = r > 0.82f
                    ? 1.0f + std::sin(angle * ribCount) * (type == 2 ? 0.055f : 0.025f)
                    : 1.0f;
                const float rr = r * asymmetry * scallopedEdge;
                const float rib = std::pow(r, 1.35f) * (0.025f + (type == 2 ? 0.030f : 0.018f))
                    * std::cos(angle * ribCount);
                const float dome = std::sin(r * glm::pi<float>()) * (type == 2 ? 0.30f : 0.38f);
                positions.push_back({
                    std::sin(angle) * rr,
                    0.035f + dome + rib + std::sin(angle * 2.1f) * 0.012f,
                    std::cos(angle) * rr * (type == 2 ? 0.88f : 0.76f) - 0.12f
                });
            }
        }
        const int row = angleSegments + 1;
        for (int radial = 0; radial < radialSegments; ++radial) {
            for (int segment = 0; segment < angleSegments; ++segment) {
                const unsigned int a = radial * row + segment;
                const unsigned int b = a + 1;
                const unsigned int c = a + row;
                const unsigned int d = c + 1;
                indices.insert(indices.end(), {a, d, b, a, c, d});
            }
        }
    } else {
        constexpr int rows = 7;
        constexpr int columns = 10;
        for (int row = 0; row <= rows; ++row) {
            const float v = static_cast<float>(row) / rows;
            for (int column = 0; column <= columns; ++column) {
                const float u = static_cast<float>(column) / columns;
                const float jagged = std::sin(column * 2.7f + row * 1.3f) * 0.045f;
                positions.push_back({
                    (u - 0.5f) * (1.35f - v * 0.18f) + jagged,
                    0.035f + std::sin(u * glm::pi<float>()) * 0.15f + std::sin(v * 9.0f) * 0.018f,
                    (v - 0.5f) * 0.82f + std::sin(u * 8.0f) * 0.035f
                });
            }
        }
        const int rowSize = columns + 1;
        for (int row = 0; row < rows; ++row) {
            for (int column = 0; column < columns; ++column) {
                if ((row == rows - 1 && column % 3 == 0) || (column == columns - 1 && row % 2 == 0)) {
                    continue;
                }
                const unsigned int a = row * rowSize + column;
                const unsigned int b = a + 1;
                const unsigned int c = a + rowSize;
                const unsigned int d = c + 1;
                indices.insert(indices.end(), {a, d, b, a, c, d});
            }
        }
    }

    std::vector<glm::vec3> normals(positions.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < indices.size(); i += 3) {
        const unsigned int a = indices[i];
        const unsigned int b = indices[i + 1];
        const unsigned int c = indices[i + 2];
        const glm::vec3 normal = glm::cross(positions[b] - positions[a], positions[c] - positions[a]);
        normals[a] += normal;
        normals[b] += normal;
        normals[c] += normal;
    }
    std::vector<float> vertices;
    vertices.reserve(positions.size() * 6);
    for (size_t i = 0; i < positions.size(); ++i) {
        const glm::vec3 normal = glm::length(normals[i]) > 0.0001f
            ? glm::normalize(normals[i])
            : glm::vec3(0.0f, 1.0f, 0.0f);
        vertices.insert(vertices.end(), {
            positions[i].x, positions[i].y, positions[i].z,
            normal.x, normal.y, normal.z
        });
    }

    ShellMesh mesh;
    mesh.indexCount = static_cast<GLsizei>(indices.size());
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return mesh;
}

ShellMesh createCrabMesh() {
    std::vector<glm::vec3> positions;
    std::vector<unsigned int> indices;

    const auto appendEllipsoid = [&positions, &indices](
        const glm::vec3& center, const glm::vec3& radius, int rings, int segments
    ) {
        const unsigned int base = static_cast<unsigned int>(positions.size());
        for (int ring = 0; ring <= rings; ++ring) {
            const float v = static_cast<float>(ring) / rings;
            const float phi = v * glm::pi<float>();
            for (int segment = 0; segment <= segments; ++segment) {
                const float u = static_cast<float>(segment) / segments;
                const float theta = u * glm::two_pi<float>();
                positions.push_back(center + glm::vec3(
                    std::sin(phi) * std::cos(theta) * radius.x,
                    std::cos(phi) * radius.y,
                    std::sin(phi) * std::sin(theta) * radius.z
                ));
            }
        }
        const int row = segments + 1;
        for (int ring = 0; ring < rings; ++ring) {
            for (int segment = 0; segment < segments; ++segment) {
                const unsigned int a = base + ring * row + segment;
                const unsigned int b = a + 1;
                const unsigned int c = a + row;
                const unsigned int d = c + 1;
                indices.insert(indices.end(), {a, d, b, a, c, d});
            }
        }
    };

    const auto appendLimb = [&positions, &indices](
        const glm::vec3& start, const glm::vec3& end, float width
    ) {
        const glm::vec3 direction = glm::normalize(end - start);
        const glm::vec3 side = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f))) * width;
        const glm::vec3 up(0.0f, width * 0.55f, 0.0f);
        const unsigned int base = static_cast<unsigned int>(positions.size());
        positions.insert(positions.end(), {
            start - side - up, start + side - up, start + side + up, start - side + up,
            end - side - up, end + side - up, end + side + up, end - side + up
        });
        constexpr unsigned int local[] = {
            0,1,2,0,2,3, 4,6,5,4,7,6,
            0,4,5,0,5,1, 1,5,6,1,6,2,
            2,6,7,2,7,3, 3,7,4,3,4,0
        };
        for (unsigned int index : local) {
            indices.push_back(base + index);
        }
    };

    appendEllipsoid({0.0f, 0.22f, 0.0f}, {0.72f, 0.24f, 0.48f}, 8, 16);
    for (int sideSign : {-1, 1}) {
        for (int leg = 0; leg < 4; ++leg) {
            const float z = -0.34f + leg * 0.22f;
            const glm::vec3 hip(sideSign * 0.48f, 0.20f, z);
            const glm::vec3 knee(sideSign * (0.82f + leg * 0.035f), 0.11f, z + (leg - 1.5f) * 0.08f);
            const glm::vec3 foot(sideSign * (1.08f + leg * 0.04f), 0.035f, z + (leg - 1.5f) * 0.16f);
            appendLimb(hip, knee, 0.075f);
            appendLimb(knee, foot, 0.055f);
        }
        const glm::vec3 armStart(sideSign * 0.45f, 0.25f, -0.30f);
        const glm::vec3 armEnd(sideSign * 0.92f, 0.30f, -0.62f);
        appendLimb(armStart, armEnd, 0.10f);
        appendEllipsoid(armEnd + glm::vec3(sideSign * 0.18f, 0.02f, -0.08f), {0.28f, 0.16f, 0.22f}, 5, 10);
        appendLimb(
            armEnd + glm::vec3(sideSign * 0.22f, 0.05f, -0.10f),
            armEnd + glm::vec3(sideSign * 0.48f, 0.10f, -0.25f),
            0.055f
        );
    }

    std::vector<glm::vec3> normals(positions.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < indices.size(); i += 3) {
        const unsigned int a = indices[i];
        const unsigned int b = indices[i + 1];
        const unsigned int c = indices[i + 2];
        const glm::vec3 normal = glm::cross(positions[b] - positions[a], positions[c] - positions[a]);
        normals[a] += normal;
        normals[b] += normal;
        normals[c] += normal;
    }
    std::vector<float> vertices;
    vertices.reserve(positions.size() * 6);
    for (size_t i = 0; i < positions.size(); ++i) {
        const glm::vec3 normal = glm::length(normals[i]) > 0.0001f
            ? glm::normalize(normals[i])
            : glm::vec3(0.0f, 1.0f, 0.0f);
        vertices.insert(vertices.end(), {
            positions[i].x, positions[i].y, positions[i].z,
            normal.x, normal.y, normal.z
        });
    }

    ShellMesh mesh;
    mesh.indexCount = static_cast<GLsizei>(indices.size());
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return mesh;
}

ShellMesh createCoralMesh(int type) {
    std::vector<glm::vec3> positions;
    std::vector<unsigned int> indices;

    const auto appendLimb = [&positions, &indices](
        const glm::vec3& start, const glm::vec3& end, float startWidth, float endWidth
    ) {
        const glm::vec3 direction = glm::normalize(end - start);
        glm::vec3 side = glm::cross(direction, glm::vec3(0.0f, 0.0f, 1.0f));
        if (glm::length(side) < 0.01f) side = glm::cross(direction, glm::vec3(1.0f, 0.0f, 0.0f));
        side = glm::normalize(side);
        const glm::vec3 forward = glm::normalize(glm::cross(side, direction));
        const unsigned int base = static_cast<unsigned int>(positions.size());
        for (int ring = 0; ring < 2; ++ring) {
            const glm::vec3 center = ring == 0 ? start : end;
            const float width = ring == 0 ? startWidth : endWidth;
            for (int segment = 0; segment < 8; ++segment) {
                const float angle = glm::two_pi<float>() * segment / 8.0f;
                positions.push_back(center + side * std::cos(angle) * width + forward * std::sin(angle) * width);
            }
        }
        for (int segment = 0; segment < 8; ++segment) {
            const unsigned int a = base + segment;
            const unsigned int b = base + (segment + 1) % 8;
            const unsigned int c = base + 8 + segment;
            const unsigned int d = base + 8 + (segment + 1) % 8;
            indices.insert(indices.end(), {a, b, d, a, d, c});
        }
    };

    if (type == 0) {
        appendLimb({0,0,0}, {0,1.5f,0}, 0.16f, 0.10f);
        for (int branch = 0; branch < 9; ++branch) {
            const float angle = branch * 2.399f;
            const float height = 0.35f + (branch % 4) * 0.26f;
            const glm::vec3 root(0.0f, height, 0.0f);
            const glm::vec3 middle(
                std::cos(angle) * (0.35f + (branch % 3) * 0.07f),
                height + 0.32f,
                std::sin(angle) * (0.35f + (branch % 3) * 0.07f)
            );
            const glm::vec3 tip = middle + glm::vec3(
                std::cos(angle + 0.25f) * 0.24f,
                0.38f + (branch % 2) * 0.10f,
                std::sin(angle + 0.25f) * 0.24f
            );
            appendLimb(root, middle, 0.095f, 0.060f);
            appendLimb(middle, tip, 0.060f, 0.025f);
        }
    } else if (type == 1) {
        for (int branch = 0; branch < 13; ++branch) {
            const float t = static_cast<float>(branch) / 12.0f;
            const float x = (t - 0.5f) * 1.9f;
            const glm::vec3 root(0.0f, 0.05f, 0.0f);
            const glm::vec3 tip(x, 1.15f + std::sin(t * glm::pi<float>()) * 0.48f, std::sin(branch * 1.7f) * 0.055f);
            appendLimb(root, tip, 0.075f, 0.020f);
            if (branch > 0) {
                const float previousT = static_cast<float>(branch - 1) / 12.0f;
                appendLimb(
                    {(previousT - 0.5f) * 1.9f, 0.72f, 0.0f},
                    {x, 0.78f, 0.0f},
                    0.030f,
                    0.025f
                );
            }
        }
    } else if (type == 2) {
        constexpr int rings = 14;
        constexpr int segments = 24;
        for (int ring = 0; ring <= rings; ++ring) {
            const float v = static_cast<float>(ring) / rings;
            const float phi = v * glm::pi<float>();
            for (int segment = 0; segment <= segments; ++segment) {
                const float u = static_cast<float>(segment) / segments;
                const float theta = u * glm::two_pi<float>();
                const float ridge = 1.0f + std::sin(theta * 8.0f + phi * 5.0f) * 0.07f;
                positions.push_back({
                    std::sin(phi) * std::cos(theta) * 0.82f * ridge,
                    0.14f + std::cos(phi) * 0.46f + 0.38f,
                    std::sin(phi) * std::sin(theta) * 0.65f * ridge
                });
            }
        }
        const int row = segments + 1;
        for (int ring = 0; ring < rings; ++ring) {
            for (int segment = 0; segment < segments; ++segment) {
                const unsigned int a = ring * row + segment;
                const unsigned int b = a + 1;
                const unsigned int c = a + row;
                const unsigned int d = c + 1;
                indices.insert(indices.end(), {a, d, b, a, c, d});
            }
        }
    } else {
        for (int blade = 0; blade < 11; ++blade) {
            const float angle = blade * 2.11f;
            const float x = std::cos(angle) * (0.10f + (blade % 4) * 0.07f);
            const float z = std::sin(angle) * (0.10f + (blade % 4) * 0.07f);
            const glm::vec3 root(x, 0.0f, z);
            const glm::vec3 middle(x + std::sin(angle) * 0.18f, 0.65f + (blade % 3) * 0.16f, z);
            const glm::vec3 tip(middle.x + std::cos(angle) * 0.18f, middle.y + 0.55f, middle.z + std::sin(angle) * 0.12f);
            appendLimb(root, middle, 0.055f, 0.035f);
            appendLimb(middle, tip, 0.035f, 0.010f);
        }
    }

    std::vector<glm::vec3> normals(positions.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < indices.size(); i += 3) {
        const unsigned int a = indices[i];
        const unsigned int b = indices[i + 1];
        const unsigned int c = indices[i + 2];
        const glm::vec3 normal = glm::cross(positions[b] - positions[a], positions[c] - positions[a]);
        normals[a] += normal;
        normals[b] += normal;
        normals[c] += normal;
    }
    std::vector<float> vertices;
    vertices.reserve(positions.size() * 6);
    for (size_t i = 0; i < positions.size(); ++i) {
        const glm::vec3 normal = glm::length(normals[i]) > 0.0001f
            ? glm::normalize(normals[i])
            : glm::vec3(0.0f, 1.0f, 0.0f);
        vertices.insert(vertices.end(), {
            positions[i].x, positions[i].y, positions[i].z,
            normal.x, normal.y, normal.z
        });
    }
    ShellMesh mesh;
    mesh.indexCount = static_cast<GLsizei>(indices.size());
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return mesh;
}

ContactShadowMesh createContactShadowMesh() {
    constexpr int segments = 32;
    std::vector<float> vertices;
    vertices.reserve(segments * 3 * 2);
    for (int segment = 0; segment < segments; ++segment) {
        const float a = glm::two_pi<float>() * segment / segments;
        const float b = glm::two_pi<float>() * (segment + 1) / segments;
        vertices.insert(vertices.end(), {
            0.0f, 0.0f,
            std::cos(a), std::sin(a),
            std::cos(b), std::sin(b)
        });
    }
    ContactShadowMesh mesh;
    mesh.vertexCount = static_cast<GLsizei>(vertices.size() / 2);
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return mesh;
}

ReefFishMesh createReefFishMesh() {
    constexpr int bodyRings = 14;
    constexpr int radialSegments = 12;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (int ring = 0; ring <= bodyRings; ++ring) {
        const float t = static_cast<float>(ring) / bodyRings;
        const float x = -0.82f + t * 1.64f;
        const float profile = std::sin(glm::pi<float>() * t);
        const float headFullness = 0.82f + 0.18f * t;
        for (int segment = 0; segment < radialSegments; ++segment) {
            const float angle = glm::two_pi<float>() * segment / radialSegments;
            const float y = std::cos(angle) * profile * 0.46f * headFullness;
            const float z = std::sin(angle) * profile * 0.25f;
            glm::vec3 normal(
                std::cos(glm::pi<float>() * t) * 0.35f,
                std::cos(angle),
                std::sin(angle) * 1.7f
            );
            normal = glm::normalize(normal);
            vertices.insert(vertices.end(), {x, y, z, normal.x, normal.y, normal.z});
        }
    }
    for (int ring = 0; ring < bodyRings; ++ring) {
        for (int segment = 0; segment < radialSegments; ++segment) {
            const unsigned int a = ring * radialSegments + segment;
            const unsigned int b = ring * radialSegments + (segment + 1) % radialSegments;
            const unsigned int c = (ring + 1) * radialSegments + segment;
            const unsigned int d = (ring + 1) * radialSegments + (segment + 1) % radialSegments;
            indices.insert(indices.end(), {a, b, d, a, d, c});
        }
    }

    const auto appendTriangle = [&vertices, &indices](
        const glm::vec3& a, const glm::vec3& b, const glm::vec3& c
    ) {
        const glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
        const unsigned int base = static_cast<unsigned int>(vertices.size() / 6);
        for (const glm::vec3& point : {a, b, c}) {
            vertices.insert(vertices.end(), {point.x, point.y, point.z, normal.x, normal.y, normal.z});
        }
        indices.insert(indices.end(), {base, base + 1, base + 2});

        const unsigned int back = static_cast<unsigned int>(vertices.size() / 6);
        for (const glm::vec3& point : {c, b, a}) {
            vertices.insert(vertices.end(), {point.x, point.y, point.z, -normal.x, -normal.y, -normal.z});
        }
        indices.insert(indices.end(), {back, back + 1, back + 2});
    };

    appendTriangle({-0.68f, 0.0f, 0.0f}, {-1.38f, 0.72f, 0.0f}, {-1.18f, 0.0f, 0.0f});
    appendTriangle({-0.68f, 0.0f, 0.0f}, {-1.18f, 0.0f, 0.0f}, {-1.38f,-0.72f, 0.0f});
    appendTriangle({-0.18f, 0.32f, 0.0f}, {0.34f, 0.84f, 0.0f}, {0.54f, 0.25f, 0.0f});
    appendTriangle({-0.04f,-0.25f, 0.0f}, {0.28f,-0.55f, 0.0f}, {0.48f,-0.20f, 0.0f});
    appendTriangle({0.18f,-0.04f, 0.16f}, {-0.18f,-0.48f, 0.62f}, {0.48f,-0.18f, 0.18f});
    appendTriangle({0.18f,-0.04f,-0.16f}, {0.48f,-0.18f,-0.18f}, {-0.18f,-0.48f,-0.62f});

    ReefFishMesh mesh;
    mesh.indexCount = static_cast<GLsizei>(indices.size());
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return mesh;
}

DistantFishSchool createDistantFishSchool() {
    constexpr float vertices[] = {
         0.75f, 0.00f, 0.00f,  1.0f, 0.2f, 0.0f,
         0.05f, 0.30f, 0.00f,  0.0f, 1.0f, 0.0f,
         0.05f,-0.30f, 0.00f,  0.0f,-1.0f, 0.0f,
         0.05f, 0.00f, 0.18f,  0.0f, 0.0f, 1.0f,
         0.05f, 0.00f,-0.18f,  0.0f, 0.0f,-1.0f,
        -0.62f, 0.00f, 0.00f, -1.0f, 0.0f, 0.0f,
        -1.05f, 0.40f, 0.00f, -0.5f, 0.7f, 0.0f,
        -1.05f,-0.40f, 0.00f, -0.5f,-0.7f, 0.0f
    };
    constexpr unsigned int indices[] = {
        0,1,3, 0,3,2, 0,2,4, 0,4,1,
        1,5,3, 3,5,2, 2,5,4, 4,5,1,
        5,6,7
    };

    DistantFishSchool school;
    school.indexCount = static_cast<GLsizei>(sizeof(indices) / sizeof(indices[0]));
    school.fish.reserve(72);
    for (int i = 0; i < 72; ++i) {
        const float fi = static_cast<float>(i);
        const float rx = std::sin(fi * 12.9898f) * 43758.5453f;
        const float ry = std::sin(fi * 34.1231f) * 17341.731f;
        const float rz = std::sin(fi * 78.233f) * 23421.631f;
        const float x = (rx - std::floor(rx)) * 24.0f - 12.0f;
        const float y = (ry - std::floor(ry)) * 3.2f + 2.7f;
        const float z = -(rz - std::floor(rz)) * 20.0f - 14.0f;
        glm::vec3 velocity(0.65f + std::sin(fi) * 0.12f, std::sin(fi * 1.7f) * 0.05f, std::cos(fi) * 0.18f);
        school.fish.push_back({glm::vec3(x, y, z), velocity, fi * 1.37f, 0.18f + (i % 7) * 0.012f});
    }

    glGenVertexArrays(1, &school.vao);
    glGenBuffers(1, &school.vbo);
    glGenBuffers(1, &school.ebo);
    glGenBuffers(1, &school.instanceVbo);
    glGenBuffers(1, &school.phaseVbo);
    glBindVertexArray(school.vao);
    glBindBuffer(GL_ARRAY_BUFFER, school.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, school.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, school.instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, school.fish.size() * sizeof(glm::mat4), nullptr, GL_STREAM_DRAW);
    for (int column = 0; column < 4; ++column) {
        glVertexAttribPointer(2 + column, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void*>(column * sizeof(glm::vec4)));
        glEnableVertexAttribArray(2 + column);
        glVertexAttribDivisor(2 + column, 1);
    }
    glBindBuffer(GL_ARRAY_BUFFER, school.phaseVbo);
    glBufferData(GL_ARRAY_BUFFER, school.fish.size() * sizeof(float), nullptr, GL_STREAM_DRAW);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(float), nullptr);
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);
    glBindVertexArray(0);
    return school;
}

void updateDistantFishSchool(DistantFishSchool& school, float deltaTime) {
    const float dt = std::min(deltaTime, 0.033f);
    std::vector<glm::vec3> nextVelocities(school.fish.size());
    for (size_t i = 0; i < school.fish.size(); ++i) {
        glm::vec3 cohesion(0.0f);
        glm::vec3 alignment(0.0f);
        glm::vec3 separation(0.0f);
        int neighbors = 0;
        for (size_t j = 0; j < school.fish.size(); ++j) {
            if (i == j) {
                continue;
            }
            const glm::vec3 offset = school.fish[j].position - school.fish[i].position;
            const float distance = glm::length(offset);
            if (distance < 5.0f) {
                cohesion += school.fish[j].position;
                alignment += school.fish[j].velocity;
                ++neighbors;
                if (distance < 1.15f && distance > 0.001f) {
                    separation -= offset / (distance * distance);
                }
            }
        }

        glm::vec3 steering(0.0f);
        if (neighbors > 0) {
            cohesion = cohesion / static_cast<float>(neighbors) - school.fish[i].position;
            alignment = alignment / static_cast<float>(neighbors) - school.fish[i].velocity;
            steering += cohesion * 0.055f + alignment * 0.34f + separation * 0.62f;
        }
        const glm::vec3 center(0.0f, 4.2f, -24.0f);
        steering += (center - school.fish[i].position) * 0.018f;
        steering += glm::vec3(
            std::cos(school.fish[i].phase) * 0.035f,
            std::sin(school.fish[i].phase * 1.3f) * 0.018f,
            std::sin(school.fish[i].phase) * 0.035f
        );
        if (school.fish[i].position.y < 2.3f) steering.y += 0.45f;
        if (school.fish[i].position.y > 6.3f) steering.y -= 0.45f;
        if (school.fish[i].position.z > -12.0f) steering.z -= 0.40f;
        if (school.fish[i].position.z < -38.0f) steering.z += 0.40f;
        if (std::abs(school.fish[i].position.x) > 15.0f) steering.x -= glm::sign(school.fish[i].position.x) * 0.45f;

        glm::vec3 velocity = school.fish[i].velocity + steering * dt;
        const float speed = glm::length(velocity);
        velocity = glm::normalize(velocity) * glm::clamp(speed, 0.55f, 1.15f);
        nextVelocities[i] = velocity;
    }

    std::vector<glm::mat4> models;
    std::vector<float> phases;
    models.reserve(school.fish.size());
    phases.reserve(school.fish.size());
    for (size_t i = 0; i < school.fish.size(); ++i) {
        SchoolFish& fish = school.fish[i];
        fish.velocity = nextVelocities[i];
        fish.position += fish.velocity * dt;
        fish.phase += dt * (4.0f + static_cast<float>(i % 5) * 0.24f);
        const float yaw = std::atan2(-fish.velocity.z, fish.velocity.x);
        const float horizontalSpeed = glm::length(glm::vec2(fish.velocity.x, fish.velocity.z));
        const float pitch = std::atan2(fish.velocity.y, horizontalSpeed);
        glm::mat4 model(1.0f);
        model = glm::translate(model, fish.position);
        model = glm::rotate(model, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, pitch, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(fish.scale, fish.scale * 0.72f, fish.scale * 0.66f));
        models.push_back(model);
        phases.push_back(fish.phase);
    }
    glBindBuffer(GL_ARRAY_BUFFER, school.instanceVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, models.size() * sizeof(glm::mat4), models.data());
    glBindBuffer(GL_ARRAY_BUFFER, school.phaseVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, phases.size() * sizeof(float), phases.data());
}

TransparentMeshes createTransparentMeshes() {
    constexpr float surfaceSize = 100.0f;
    constexpr int surfaceResolution = 128;
    std::vector<float> surfaceVertices;
    std::vector<unsigned int> surfaceIndices;
    surfaceVertices.reserve((surfaceResolution + 1) * (surfaceResolution + 1) * 5);
    surfaceIndices.reserve(surfaceResolution * surfaceResolution * 6);

    for (int z = 0; z <= surfaceResolution; ++z) {
        for (int x = 0; x <= surfaceResolution; ++x) {
            const float u = static_cast<float>(x) / surfaceResolution;
            const float v = static_cast<float>(z) / surfaceResolution;
            surfaceVertices.push_back((u * 2.0f - 1.0f) * surfaceSize);
            surfaceVertices.push_back(10.0f);
            surfaceVertices.push_back((v * 2.0f - 1.0f) * surfaceSize);
            surfaceVertices.push_back(u * 30.0f);
            surfaceVertices.push_back(v * 30.0f);
        }
    }

    for (int z = 0; z < surfaceResolution; ++z) {
        for (int x = 0; x < surfaceResolution; ++x) {
            const unsigned int row = surfaceResolution + 1;
            const unsigned int topLeft = z * row + x;
            const unsigned int topRight = topLeft + 1;
            const unsigned int bottomLeft = topLeft + row;
            const unsigned int bottomRight = bottomLeft + 1;
            surfaceIndices.insert(surfaceIndices.end(), {
                topLeft, topRight, bottomRight,
                topLeft, bottomRight, bottomLeft
            });
        }
    }
    constexpr float quadVertices[] = {
        -0.5f, -1.0f, 0.0f, 0.0f, 0.0f,
         0.5f, -1.0f, 0.0f, 1.0f, 0.0f,
         0.5f,  0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, -1.0f, 0.0f, 0.0f, 0.0f,
         0.5f,  0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f,  0.0f, 0.0f, 0.0f, 1.0f
    };

    TransparentMeshes meshes;
    glGenVertexArrays(1, &meshes.surfaceVao);
    glGenBuffers(1, &meshes.surfaceVbo);
    glGenBuffers(1, &meshes.surfaceEbo);
    glBindVertexArray(meshes.surfaceVao);
    glBindBuffer(GL_ARRAY_BUFFER, meshes.surfaceVbo);
    glBufferData(GL_ARRAY_BUFFER, surfaceVertices.size() * sizeof(float), surfaceVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshes.surfaceEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, surfaceIndices.size() * sizeof(unsigned int), surfaceIndices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    meshes.surfaceIndexCount = static_cast<GLsizei>(surfaceIndices.size());

    glGenVertexArrays(1, &meshes.quadVao);
    glGenBuffers(1, &meshes.quadVbo);
    glBindVertexArray(meshes.quadVao);
    glBindBuffer(GL_ARRAY_BUFFER, meshes.quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    meshes.waterNormalTexture = loadJpegTexture("assets/background/textures/water_normal.jpg");
    return meshes;
}

void destroyFloorMesh(const FloorMesh& mesh) {
    glDeleteTextures(1, &mesh.sandTexture);
    glDeleteBuffers(1, &mesh.ebo);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteVertexArrays(1, &mesh.vao);
}

void destroyFullscreenMesh(const FullscreenMesh& mesh) {
    glDeleteVertexArrays(1, &mesh.vao);
}

void destroyParticleMesh(const ParticleMesh& mesh) {
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteVertexArrays(1, &mesh.vao);
}

void destroyInstancedPebbles(const InstancedPebbles& mesh) {
    glDeleteBuffers(1, &mesh.instanceVbo);
    glDeleteBuffers(1, &mesh.ebo);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteVertexArrays(1, &mesh.vao);
}

void destroyShellMesh(const ShellMesh& mesh) {
    glDeleteBuffers(1, &mesh.ebo);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteVertexArrays(1, &mesh.vao);
}

void destroyContactShadowMesh(const ContactShadowMesh& mesh) {
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteVertexArrays(1, &mesh.vao);
}

void destroyReefFishMesh(const ReefFishMesh& mesh) {
    glDeleteBuffers(1, &mesh.ebo);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteVertexArrays(1, &mesh.vao);
}

void destroyDistantFishSchool(const DistantFishSchool& school) {
    glDeleteBuffers(1, &school.phaseVbo);
    glDeleteBuffers(1, &school.instanceVbo);
    glDeleteBuffers(1, &school.ebo);
    glDeleteBuffers(1, &school.vbo);
    glDeleteVertexArrays(1, &school.vao);
}

void destroyTransparentMeshes(const TransparentMeshes& meshes) {
    glDeleteTextures(1, &meshes.waterNormalTexture);
    glDeleteBuffers(1, &meshes.quadVbo);
    glDeleteVertexArrays(1, &meshes.quadVao);
    glDeleteBuffers(1, &meshes.surfaceEbo);
    glDeleteBuffers(1, &meshes.surfaceVbo);
    glDeleteVertexArrays(1, &meshes.surfaceVao);
}


struct OceanBackgroundRenderer::State {
  ocean_bg::BackgroundCamera camera;

  std::unique_ptr<Shader> waterShader;
  std::unique_ptr<Shader> floorShader;
  std::unique_ptr<Shader> rockShader;
  std::unique_ptr<Shader> animalShader;
  std::unique_ptr<Shader> schoolFishShader;
  std::unique_ptr<Shader> decorShader;
  std::unique_ptr<Shader> contactShadowShader;
  std::unique_ptr<Shader> pebbleShader;
  std::unique_ptr<Shader> surfaceShader;
  std::unique_ptr<Shader> rayShader;
  std::unique_ptr<Shader> particleShader;
  std::unique_ptr<Shader> algaeShader;

  FullscreenMesh fullscreen;
  FloorMesh floor;
  std::unique_ptr<ocean_bg::BackgroundModel> clownfish;
  std::unique_ptr<ocean_bg::BackgroundModel> dolphin;
  std::unique_ptr<ocean_bg::BackgroundModel> starfish;
  std::unique_ptr<ocean_bg::BackgroundModel> seaweed;
  std::vector<std::unique_ptr<ocean_bg::BackgroundModel>> rockModels;
  TransparentMeshes transparentMeshes;
  ParticleMesh particles;
  InstancedPebbles pebbles;
  ShellMesh spiralShell;
  ShellMesh clamShell;
  ShellMesh scallopShell;
  ShellMesh brokenShell;
  ShellMesh crabMesh;
  ShellMesh branchingCoral;
  ShellMesh fanCoral;
  ShellMesh brainCoral;
  ShellMesh seaweedCoral;
  ContactShadowMesh contactShadow;
  ReefFishMesh reefFish;
  DistantFishSchool distantSchool;
  float lastFrame = 0.0f;

  State()
      : waterShader(std::make_unique<Shader>("shaders/background_full/water.vert", "shaders/background_full/water.frag")),
        floorShader(std::make_unique<Shader>("shaders/background_full/floor.vert", "shaders/background_full/floor.frag")),
        rockShader(std::make_unique<Shader>("shaders/background_full/rock.vert", "shaders/background_full/rock.frag")),
        animalShader(std::make_unique<Shader>("shaders/background_full/animal.vert", "shaders/background_full/animal.frag")),
        schoolFishShader(std::make_unique<Shader>("shaders/background_full/school_fish.vert", "shaders/background_full/school_fish.frag")),
        decorShader(std::make_unique<Shader>("shaders/background_full/decor.vert", "shaders/background_full/decor.frag")),
        contactShadowShader(std::make_unique<Shader>("shaders/background_full/contact_shadow.vert", "shaders/background_full/contact_shadow.frag")),
        pebbleShader(std::make_unique<Shader>("shaders/background_full/pebble.vert", "shaders/background_full/pebble.frag")),
        surfaceShader(std::make_unique<Shader>("shaders/background_full/surface.vert", "shaders/background_full/surface.frag")),
        rayShader(std::make_unique<Shader>("shaders/background_full/ray.vert", "shaders/background_full/ray.frag")),
        particleShader(std::make_unique<Shader>("shaders/background_full/particle.vert", "shaders/background_full/particle.frag")),
        algaeShader(std::make_unique<Shader>("shaders/background_full/algae.vert", "shaders/background_full/algae.frag")),
        fullscreen(createFullscreenMesh()),
        floor(createFloorMesh()),
        clownfish(std::make_unique<ocean_bg::BackgroundModel>("assets/background/models/proper_clownfish/model.fbx", true)),
        dolphin(std::make_unique<ocean_bg::BackgroundModel>("assets/background/models/proper_dolphin/model.fbx", true)),
        starfish(std::make_unique<ocean_bg::BackgroundModel>("assets/background/models/starfish/model.fbx", true)),
        seaweed(std::make_unique<ocean_bg::BackgroundModel>("assets/background/models/seaweed/seaweed.fbx", true)),
        transparentMeshes(createTransparentMeshes()),
        particles(createParticleMesh()),
        pebbles(createInstancedPebbles()),
        spiralShell(createShellMesh(0)),
        clamShell(createShellMesh(1)),
        scallopShell(createShellMesh(2)),
        brokenShell(createShellMesh(3)),
        crabMesh(createCrabMesh()),
        branchingCoral(createCoralMesh(0)),
        fanCoral(createCoralMesh(1)),
        brainCoral(createCoralMesh(2)),
        seaweedCoral(createCoralMesh(3)),
        contactShadow(createContactShadowMesh()),
        reefFish(createReefFishMesh()),
        distantSchool(createDistantFishSchool()),
        lastFrame(static_cast<float>(glfwGetTime())) {
    for (const std::string &path : {
             "assets/background/models/rocks/rock01.obj",
             "assets/background/models/rocks/rock02.obj",
             "assets/background/models/rocks/rock03.obj"}) {
      if (std::filesystem::exists(path)) {
        rockModels.push_back(std::make_unique<ocean_bg::BackgroundModel>(path));
      }
    }
    if (rockModels.empty()) {
      std::cerr << "No rock assets found for ocean background; continuing without rock models.\n";
    }

    floorShader->use();
    floorShader->setInt("sandTexture", 0);
    surfaceShader->use();
    surfaceShader->setInt("waterNormalTexture", 0);
  }

  ~State() {
    destroyFloorMesh(floor);
    destroyFullscreenMesh(fullscreen);
    destroyTransparentMeshes(transparentMeshes);
    destroyParticleMesh(particles);
    destroyInstancedPebbles(pebbles);
    destroyShellMesh(brokenShell);
    destroyShellMesh(scallopShell);
    destroyShellMesh(clamShell);
    destroyShellMesh(spiralShell);
    destroyShellMesh(crabMesh);
    destroyShellMesh(seaweedCoral);
    destroyShellMesh(brainCoral);
    destroyShellMesh(fanCoral);
    destroyShellMesh(branchingCoral);
    destroyContactShadowMesh(contactShadow);
    destroyReefFishMesh(reefFish);
    destroyDistantFishSchool(distantSchool);
  }
};

OceanBackgroundRenderer::OceanBackgroundRenderer() = default;
OceanBackgroundRenderer::~OceanBackgroundRenderer() = default;

void OceanBackgroundRenderer::init() { state = std::make_unique<State>(); }

void OceanBackgroundRenderer::render(float time, int framebufferWidth,
                                     int framebufferHeight) {
  if (!state || framebufferWidth <= 0 || framebufferHeight <= 0) {
    return;
  }

  State &s = *state;
  Shader &waterShader = *s.waterShader;
  Shader &floorShader = *s.floorShader;
  Shader &rockShader = *s.rockShader;
  Shader &animalShader = *s.animalShader;
  Shader &schoolFishShader = *s.schoolFishShader;
  Shader &decorShader = *s.decorShader;
  Shader &contactShadowShader = *s.contactShadowShader;
  Shader &pebbleShader = *s.pebbleShader;
  Shader &surfaceShader = *s.surfaceShader;
  Shader &rayShader = *s.rayShader;
  Shader &particleShader = *s.particleShader;
  Shader &algaeShader = *s.algaeShader;
  const FullscreenMesh &fullscreen = s.fullscreen;
  const FloorMesh &floor = s.floor;
  const ocean_bg::BackgroundModel &clownfish = *s.clownfish;
  const ocean_bg::BackgroundModel &dolphin = *s.dolphin;
  const ocean_bg::BackgroundModel &starfish = *s.starfish;
  const ocean_bg::BackgroundModel &seaweed = *s.seaweed;
  auto &rockModels = s.rockModels;
  const TransparentMeshes &transparentMeshes = s.transparentMeshes;
  const ParticleMesh &particles = s.particles;
  const InstancedPebbles &pebbles = s.pebbles;
  const ShellMesh &spiralShell = s.spiralShell;
  const ShellMesh &clamShell = s.clamShell;
  const ShellMesh &scallopShell = s.scallopShell;
  const ShellMesh &brokenShell = s.brokenShell;
  const ShellMesh &crabMesh = s.crabMesh;
  const ShellMesh &branchingCoral = s.branchingCoral;
  const ShellMesh &fanCoral = s.fanCoral;
  const ShellMesh &brainCoral = s.brainCoral;
  const ShellMesh &seaweedCoral = s.seaweedCoral;
  const ContactShadowMesh &contactShadow = s.contactShadow;
  const ReefFishMesh &reefFish = s.reefFish;
  DistantFishSchool &distantSchool = s.distantSchool;
  ocean_bg::BackgroundCamera &camera = s.camera;

  const float currentFrame = time;
  const float deltaTime = currentFrame - s.lastFrame;
  s.lastFrame = currentFrame;
  updateDistantFishSchool(distantSchool, deltaTime);

  const glm::mat4 model(1.0f);
  const glm::mat4 view = camera.viewMatrix();
  const float aspect = framebufferHeight > 0
      ? static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight)
      : 1.0f;
  const glm::mat4 projection = glm::perspective(glm::radians(camera.fov()), aspect, 0.1f, 180.0f);
  const glm::vec3 sunDirection = glm::normalize(glm::vec3(-0.35f, -1.0f, -0.2f));

  glDisable(GL_DEPTH_TEST);
  waterShader.use();
  waterShader.setFloat("time", currentFrame);
  setShaderVec3(waterShader, "sunDirection", sunDirection);
  glBindVertexArray(fullscreen.vao);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  glEnable(GL_DEPTH_TEST);
  floorShader.use();
  floorShader.setMat4("model", glm::value_ptr(model));
  floorShader.setMat4("view", glm::value_ptr(view));
  floorShader.setMat4("projection", glm::value_ptr(projection));
  setShaderVec3(floorShader, "cameraPos", camera.position());
  setShaderVec3(floorShader, "sunDirection", sunDirection);
  floorShader.setFloat("time", currentFrame);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, floor.sandTexture);
  glBindVertexArray(floor.vao);
  glDrawElements(GL_TRIANGLES, floor.indexCount, GL_UNSIGNED_INT, nullptr);

  pebbleShader.use();
  pebbleShader.setMat4("view", glm::value_ptr(view));
  pebbleShader.setMat4("projection", glm::value_ptr(projection));
  setShaderVec3(pebbleShader, "cameraPos", camera.position());
  setShaderVec3(pebbleShader, "sunDirection", sunDirection);
  pebbleShader.setFloat("time", currentFrame);
  glBindVertexArray(pebbles.vao);
  glDrawElementsInstanced(GL_TRIANGLES, pebbles.indexCount, GL_UNSIGNED_INT, nullptr, pebbles.instanceCount);

  rockShader.use();
  rockShader.setMat4("view", glm::value_ptr(view));
  rockShader.setMat4("projection", glm::value_ptr(projection));
  setShaderVec3(rockShader, "cameraPos", camera.position());
  setShaderVec3(rockShader, "sunDirection", sunDirection);
  rockShader.setFloat("time", currentFrame);
  constexpr glm::vec3 rockPositions[] = {
      {-5.8f, -0.62f, -1.0f}, {-4.2f, -0.56f, -1.8f},
      {-6.8f, -0.60f, -2.5f}, {-5.0f, -0.65f, -3.1f},
      { 5.5f, -0.60f,-10.5f}, { 7.0f, -0.64f,-11.8f},
      { 4.4f, -0.56f,-12.8f}, { 6.2f, -0.68f,-13.6f},
      {-7.5f, -0.70f,-24.0f}, {-5.2f, -0.72f,-25.5f}
  };
  constexpr glm::vec3 rockScales[] = {
      {1.25f,1.05f,1.15f},{0.72f,0.68f,0.78f},
      {0.58f,0.56f,0.62f},{0.92f,0.78f,0.82f},
      {1.15f,0.98f,1.05f},{0.68f,0.64f,0.72f},
      {0.54f,0.52f,0.60f},{0.85f,0.76f,0.78f},
      {0.62f,0.58f,0.66f},{0.46f,0.44f,0.50f}
  };
  for (int i = 0; i < 10 && !rockModels.empty(); ++i) {
      glm::mat4 rockModel(1.0f);
      rockModel = glm::translate(rockModel, rockPositions[i]);
      rockModel = glm::rotate(rockModel, glm::radians(static_cast<float>(i * 37 % 180)), glm::vec3(0.0f, 1.0f, 0.0f));
      rockModel = glm::rotate(rockModel, glm::radians(-5.0f + static_cast<float>(i % 4) * 3.0f), glm::vec3(1.0f, 0.0f, 0.0f));
      rockModel = glm::scale(rockModel, rockScales[i]);
      rockShader.setMat4("model", glm::value_ptr(rockModel));
      rockShader.setFloat("rockVariation", static_cast<float>(i) * 0.71f);
      rockModels[static_cast<size_t>(i) % rockModels.size()]->draw();
  }

  animalShader.use();
  animalShader.setMat4("view", glm::value_ptr(view));
  animalShader.setMat4("projection", glm::value_ptr(projection));
  setShaderVec3(animalShader, "cameraPos", camera.position());
  setShaderVec3(animalShader, "sunDirection", sunDirection);
  animalShader.setFloat("time", currentFrame);

  animalShader.setInt("animalType", 0);
  constexpr glm::vec3 clownfishCenters[] = {
      {-3.6f, 1.35f, -3.5f}, {-1.8f, 1.85f, -5.5f},
      {-4.5f, 2.10f, -7.5f}
  };
  for (int i = 0; i < 3; ++i) {
      const float speed = 0.22f + static_cast<float>(i) * 0.035f;
      const float phase = currentFrame * speed + static_cast<float>(i) * 1.63f;
      const float radiusX = 1.5f + static_cast<float>(i % 3) * 0.45f;
      const float radiusZ = 1.0f + static_cast<float>((i + 1) % 3) * 0.38f;
      const float x = clownfishCenters[i].x + std::cos(phase) * radiusX;
      const float z = clownfishCenters[i].z + std::sin(phase * (1.0f + i * 0.17f)) * radiusZ;
      const float y = clownfishCenters[i].y + std::sin(phase * 1.45f + i) * (0.20f + i * 0.035f);
      const float dx = -std::sin(phase) * radiusX * speed;
      const float dz = std::cos(phase * (1.0f + i * 0.17f))
          * radiusZ * speed * (1.0f + i * 0.17f);
      const float yaw = std::atan2(-dz, dx);

      glm::mat4 fishModel(1.0f);
      fishModel = glm::translate(fishModel, glm::vec3(x, y, z));
      fishModel = glm::rotate(fishModel, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
      fishModel = glm::rotate(fishModel, std::sin(phase * 1.7f) * 0.08f, glm::vec3(0.0f, 0.0f, 1.0f));
      fishModel = glm::scale(fishModel, glm::vec3(0.48f + i * 0.035f));
      animalShader.setMat4("model", glm::value_ptr(fishModel));
      animalShader.setFloat("animationPhase", static_cast<float>(i) * 1.63f);
      animalShader.setFloat("swimSpeed", 5.5f + static_cast<float>(i) * 0.25f);
      clownfish.draw();
  }

  constexpr glm::vec3 schoolCenters[] = {
      {-5.2f, 2.55f, -4.5f},
      { 5.4f, 3.05f, -9.0f},
      {-3.8f, 2.85f,-14.0f}
  };
  glBindVertexArray(reefFish.vao);
  for (int i = 0; i < 30; ++i) {
      const int school = i / 10;
      const int member = i % 10;
      const int species = i % 3;
      const float fi = static_cast<float>(i);
      const float speed = 0.19f + static_cast<float>(i % 5) * 0.018f;
      const float phase = currentFrame * speed + fi * 1.71f;
      const float orbitX = 1.6f + static_cast<float>(member % 4) * 0.32f;
      const float orbitZ = 0.9f + static_cast<float>((member + 2) % 4) * 0.23f;
      const float x = schoolCenters[school].x + std::cos(phase) * orbitX
          + std::sin(fi * 2.17f) * 0.45f;
      const float z = schoolCenters[school].z + std::sin(phase * 0.82f) * orbitZ
          + std::cos(fi * 1.39f) * 0.55f;
      const float y = schoolCenters[school].y
          + std::sin(phase * 1.25f + fi) * 0.40f
          + static_cast<float>((member % 3) - 1) * 0.18f;
      const float dx = -std::sin(phase) * orbitX * speed;
      const float dz = std::cos(phase * 0.82f) * orbitZ * speed * 0.82f;
      const float yaw = std::atan2(-dz, dx);
      const float baseScale = 0.24f + static_cast<float>(i % 6) * 0.025f;

      glm::vec3 speciesScale(baseScale);
      if (species == 0) {
          speciesScale *= glm::vec3(1.20f, 0.68f, 0.62f);
      } else if (species == 1) {
          speciesScale *= glm::vec3(0.90f, 1.12f, 0.72f);
      } else {
          speciesScale *= glm::vec3(1.02f, 0.88f, 0.72f);
      }

      glm::mat4 reefFishModel(1.0f);
      reefFishModel = glm::translate(reefFishModel, glm::vec3(x, y, z));
      reefFishModel = glm::rotate(reefFishModel, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
      reefFishModel = glm::rotate(
          reefFishModel,
          std::sin(phase * 1.6f + fi) * 0.07f,
          glm::vec3(0.0f, 0.0f, 1.0f)
      );
      reefFishModel = glm::scale(reefFishModel, speciesScale);
      animalShader.setInt("animalType", 2 + species);
      animalShader.setFloat("animationPhase", fi * 1.31f);
      animalShader.setFloat("swimSpeed", 4.2f + static_cast<float>(i % 5) * 0.42f);
      animalShader.setMat4("model", glm::value_ptr(reefFishModel));
      glDrawElements(GL_TRIANGLES, reefFish.indexCount, GL_UNSIGNED_INT, nullptr);
  }

  constexpr glm::vec3 foregroundFishCenters[] = {
      {-2.8f, 1.65f, 3.0f},
      { 2.6f, 2.05f, 2.1f},
      {-0.7f, 2.45f, 0.8f},
      { 3.8f, 1.45f,-0.3f},
      {-4.0f, 2.20f,-1.2f},
      { 1.2f, 1.75f,-2.4f}
  };
  for (int i = 0; i < 6; ++i) {
      const float fi = static_cast<float>(i);
      const float speed = 0.16f + static_cast<float>(i % 3) * 0.025f;
      const float phase = currentFrame * speed + fi * 1.43f;
      const float radiusX = 1.15f + static_cast<float>(i % 2) * 0.40f;
      const float radiusZ = 0.55f + static_cast<float>((i + 1) % 3) * 0.18f;
      const float x = foregroundFishCenters[i].x + std::cos(phase) * radiusX;
      const float z = foregroundFishCenters[i].z + std::sin(phase * 0.90f) * radiusZ;
      const float y = foregroundFishCenters[i].y + std::sin(phase * 1.4f + fi) * 0.22f;
      const float dx = -std::sin(phase) * radiusX * speed;
      const float dz = std::cos(phase * 0.90f) * radiusZ * speed * 0.90f;
      const float yaw = std::atan2(-dz, dx);
      const float scale = 0.34f + static_cast<float>(i % 4) * 0.035f;

      glm::mat4 foregroundFishModel(1.0f);
      foregroundFishModel = glm::translate(foregroundFishModel, glm::vec3(x, y, z));
      foregroundFishModel = glm::rotate(foregroundFishModel, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
      foregroundFishModel = glm::rotate(
          foregroundFishModel,
          std::sin(phase * 1.7f + fi) * 0.06f,
          glm::vec3(0.0f, 0.0f, 1.0f)
      );
      foregroundFishModel = glm::scale(foregroundFishModel, glm::vec3(scale, scale * 0.86f, scale * 0.72f));
      animalShader.setInt("animalType", 2 + i % 3);
      animalShader.setFloat("animationPhase", fi * 1.57f);
      animalShader.setFloat("swimSpeed", 4.4f + static_cast<float>(i % 4) * 0.38f);
      animalShader.setMat4("model", glm::value_ptr(foregroundFishModel));
      glDrawElements(GL_TRIANGLES, reefFish.indexCount, GL_UNSIGNED_INT, nullptr);
  }

  animalShader.setInt("animalType", 1);
  animalShader.setFloat("animationPhase", 0.0f);
  animalShader.setFloat("swimSpeed", 2.2f);
  const float dolphinPhase = currentFrame * 0.065f;
  const float dolphinX = 6.0f + std::sin(dolphinPhase) * 7.0f;
  const float dolphinZ = -17.0f + std::cos(dolphinPhase) * 4.5f;
  const float dolphinY = 3.5f + std::sin(dolphinPhase * 1.4f) * 0.40f;
  const float dolphinDx = std::cos(dolphinPhase) * 0.845f;
  const float dolphinDz = -std::sin(dolphinPhase) * 0.585f;
  glm::mat4 dolphinModel(1.0f);
  dolphinModel = glm::translate(dolphinModel, glm::vec3(dolphinX, dolphinY, dolphinZ));
  dolphinModel = glm::rotate(dolphinModel, std::atan2(-dolphinDz, dolphinDx), glm::vec3(0.0f, 1.0f, 0.0f));
  dolphinModel = glm::rotate(dolphinModel, std::sin(dolphinPhase * 1.7f) * 0.05f, glm::vec3(0.0f, 0.0f, 1.0f));
  dolphinModel = glm::scale(dolphinModel, glm::vec3(3.2f));
  animalShader.setMat4("model", glm::value_ptr(dolphinModel));
  dolphin.draw();

  schoolFishShader.use();
  schoolFishShader.setMat4("view", glm::value_ptr(view));
  schoolFishShader.setMat4("projection", glm::value_ptr(projection));
  setShaderVec3(schoolFishShader, "cameraPos", camera.position());
  setShaderVec3(schoolFishShader, "sunDirection", sunDirection);
  schoolFishShader.setFloat("time", currentFrame);
  glBindVertexArray(distantSchool.vao);
  glDrawElementsInstanced(
      GL_TRIANGLES,
      distantSchool.indexCount,
      GL_UNSIGNED_INT,
      nullptr,
      static_cast<GLsizei>(distantSchool.fish.size())
  );

  constexpr glm::vec3 crabBases[] = {
      {-2.6f, 0.025f, 1.15f},
      { 3.7f, 0.025f,-1.65f},
      {-5.1f, 0.025f,-5.75f}
  };
  constexpr float crabHeadings[] = {0.18f, 2.45f, -1.10f};
  glm::vec3 crabPositions[3];
  for (int i = 0; i < 3; ++i) {
      const float cycle = std::fmod(currentFrame + static_cast<float>(i) * 2.7f, 10.0f);
      const float walkPhase = std::min(cycle, 7.0f) / 7.0f * glm::two_pi<float>();
      const float sideways = std::sin(walkPhase) * (0.55f + static_cast<float>(i) * 0.12f);
      const glm::vec3 sideDirection(std::cos(crabHeadings[i]), 0.0f, -std::sin(crabHeadings[i]));
      crabPositions[i] = crabBases[i] + sideDirection * sideways;
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);
  contactShadowShader.use();
  contactShadowShader.setMat4("view", glm::value_ptr(view));
  contactShadowShader.setMat4("projection", glm::value_ptr(projection));
  setShaderVec3(contactShadowShader, "cameraPos", camera.position());
  glBindVertexArray(contactShadow.vao);
  for (int i = 0; i < 3; ++i) {
      glm::mat4 shadowModel(1.0f);
      shadowModel = glm::translate(shadowModel, glm::vec3(crabPositions[i].x, 0.008f, crabPositions[i].z));
      shadowModel = glm::rotate(shadowModel, crabHeadings[i], glm::vec3(0.0f, 1.0f, 0.0f));
      shadowModel = glm::scale(shadowModel, glm::vec3(0.85f, 1.0f, 0.55f));
      contactShadowShader.setMat4("model", glm::value_ptr(shadowModel));
      glDrawArrays(GL_TRIANGLES, 0, contactShadow.vertexCount);
  }
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);

  decorShader.use();
  decorShader.setMat4("view", glm::value_ptr(view));
  decorShader.setMat4("projection", glm::value_ptr(projection));
  setShaderVec3(decorShader, "cameraPos", camera.position());
  setShaderVec3(decorShader, "sunDirection", sunDirection);
  decorShader.setFloat("time", currentFrame);
  decorShader.setInt("shellDetail", 0);
  constexpr glm::vec3 coralPositions[] = {
      {-5.8f,-0.05f, 3.4f}, {-4.9f,-0.04f, 1.1f}, {-6.2f,-0.05f,-1.2f},
      { 5.6f,-0.05f, 1.8f}, { 6.7f,-0.04f,-0.8f}, { 4.9f,-0.04f,-2.5f},
      {-6.5f,-0.05f,-3.8f}, {-5.0f,-0.05f,-5.8f},
      { 6.2f,-0.05f,-6.8f}, { 4.5f,-0.05f,-8.5f},
      {-7.0f,-0.05f,-10.5f}, { 7.4f,-0.05f,-12.0f}
  };
  constexpr glm::vec3 coralColors[] = {
      {0.25f,0.32f,0.58f}, {0.38f,0.18f,0.52f}, {0.18f,0.42f,0.25f},
      {0.62f,0.31f,0.13f}, {0.30f,0.20f,0.55f}, {0.15f,0.38f,0.27f},
      {0.52f,0.25f,0.12f}, {0.20f,0.34f,0.56f}, {0.42f,0.20f,0.48f},
      {0.18f,0.40f,0.22f}, {0.58f,0.30f,0.14f}, {0.24f,0.30f,0.54f}
  };
  decorShader.setInt("shellDetail", 2);
  for (int i = 0; i < 12; ++i) {
      glm::mat4 coralModel(1.0f);
      coralModel = glm::translate(coralModel, coralPositions[i]);
      coralModel = glm::rotate(coralModel, glm::radians(static_cast<float>(i * 61)), glm::vec3(0.0f, 1.0f, 0.0f));
      const float coralScale = 0.72f + static_cast<float>(i % 4) * 0.13f;
      coralModel = glm::scale(coralModel, glm::vec3(coralScale, coralScale * (0.92f + (i % 3) * 0.08f), coralScale));
      decorShader.setMat4("model", glm::value_ptr(coralModel));
      setShaderVec3(decorShader, "baseColor", coralColors[i]);
      const ShellMesh* coralMesh = nullptr;
      if (i % 4 == 0) coralMesh = &branchingCoral;
      else if (i % 4 == 1) coralMesh = &fanCoral;
      else if (i % 4 == 2) coralMesh = &brainCoral;
      else coralMesh = &seaweedCoral;
      glBindVertexArray(coralMesh->vao);
      glDrawElements(GL_TRIANGLES, coralMesh->indexCount, GL_UNSIGNED_INT, nullptr);
  }
  decorShader.setInt("shellDetail", 0);

  constexpr glm::vec3 starfishPositions[] = {
      {-3.1f, 0.02f,5.05f}, { 3.3f, 0.02f,4.85f},
      {-4.7f, 0.02f,4.35f}, { 4.9f, 0.02f,4.05f},
      { 1.7f, 0.02f,3.55f}, {-4.2f, 0.02f,3.15f}
  };
  constexpr glm::vec3 starfishColors[] = {
      {0.72f,0.20f,0.10f},{0.55f,0.16f,0.34f},{0.82f,0.30f,0.12f},
      {0.42f,0.18f,0.58f},{0.75f,0.25f,0.12f},{0.58f,0.15f,0.20f}
  };
  for (int i = 0; i < 6; ++i) {
      glm::mat4 starfishModel(1.0f);
      starfishModel = glm::translate(starfishModel, starfishPositions[i]);
      starfishModel = glm::rotate(starfishModel, glm::radians(static_cast<float>(i * 53)), glm::vec3(0.0f, 1.0f, 0.0f));
      starfishModel = glm::scale(starfishModel, glm::vec3(0.34f + (i % 3) * 0.12f));
      decorShader.setMat4("model", glm::value_ptr(starfishModel));
      setShaderVec3(decorShader, "baseColor", starfishColors[i]);
      starfish.draw();
  }

  constexpr glm::vec3 shellPositions[] = {
      {-4.5f,-0.015f,5.45f}, {-3.8f,-0.020f,5.25f}, { 4.2f,-0.012f,5.38f},
      { 4.7f,-0.025f,5.12f}, {-2.7f,-0.020f,5.05f}, { 2.0f,-0.028f,4.92f},
      {-0.8f,-0.018f,4.78f}, {-0.1f,-0.030f,4.62f}, { 4.9f,-0.025f,4.55f},
      {-3.9f,-0.022f,4.48f}, {-3.4f,-0.030f,4.30f}, { 1.0f,-0.030f,4.25f},
      { 3.3f,-0.025f,4.10f}, {-1.8f,-0.035f,4.02f}, { 0.2f,-0.028f,3.88f},
      {-5.0f,-0.030f,3.72f}, { 3.7f,-0.022f,4.95f}, {-4.2f,-0.030f,4.88f},
      { 1.7f,-0.025f,4.42f}, {-2.2f,-0.032f,4.18f}
  };
  constexpr glm::vec3 shellColors[] = {
      {0.86f,0.72f,0.52f},{0.78f,0.56f,0.38f},{0.90f,0.78f,0.66f},
      {0.72f,0.48f,0.30f},{0.88f,0.76f,0.58f},{0.92f,0.70f,0.68f},
      {0.82f,0.64f,0.48f},{0.74f,0.50f,0.35f},{0.90f,0.80f,0.64f},
      {0.84f,0.68f,0.52f},{0.88f,0.66f,0.62f},{0.76f,0.52f,0.34f},
      {0.91f,0.79f,0.66f},{0.70f,0.46f,0.30f},{0.86f,0.70f,0.56f},
      {0.82f,0.62f,0.46f},{0.91f,0.75f,0.62f},{0.75f,0.52f,0.34f},
      {0.88f,0.72f,0.55f},{0.90f,0.68f,0.66f}
  };
  constexpr float shellScales[] = {
      0.56f,0.30f,0.52f,0.28f,0.36f,0.34f,0.30f,0.22f,
      0.32f,0.29f,0.20f,0.27f,0.25f,0.23f,0.21f,0.24f,
      0.26f,0.19f,0.22f,0.18f
  };
  decorShader.setInt("shellDetail", 1);
  for (int i = 0; i < 20; ++i) {
      glm::mat4 shellModel(1.0f);
      shellModel = glm::translate(shellModel, shellPositions[i]);
      shellModel = glm::rotate(shellModel, glm::radians(static_cast<float>(i * 47 % 360)), glm::vec3(0.0f, 1.0f, 0.0f));
      shellModel = glm::rotate(shellModel, glm::radians(-5.0f + (i % 4) * 3.0f), glm::vec3(1.0f, 0.0f, 0.0f));
      shellModel = glm::rotate(shellModel, glm::radians(-4.0f + (i % 3) * 4.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      const float stretch = 0.90f + static_cast<float>(i % 5) * 0.035f;
      shellModel = glm::scale(shellModel, glm::vec3(shellScales[i] * stretch, shellScales[i], shellScales[i] / stretch));
      decorShader.setMat4("model", glm::value_ptr(shellModel));
      setShaderVec3(decorShader, "baseColor", shellColors[i]);
      if (i % 4 == 0) {
          glBindVertexArray(spiralShell.vao);
          glDrawElements(GL_TRIANGLES, spiralShell.indexCount, GL_UNSIGNED_INT, nullptr);
      } else if (i % 4 == 1) {
          glBindVertexArray(clamShell.vao);
          glDrawElements(GL_TRIANGLES, clamShell.indexCount, GL_UNSIGNED_INT, nullptr);
      } else if (i % 4 == 2) {
          glBindVertexArray(scallopShell.vao);
          glDrawElements(GL_TRIANGLES, scallopShell.indexCount, GL_UNSIGNED_INT, nullptr);
      } else {
          glBindVertexArray(brokenShell.vao);
          glDrawElements(GL_TRIANGLES, brokenShell.indexCount, GL_UNSIGNED_INT, nullptr);
      }
  }
  decorShader.setInt("shellDetail", 0);

  constexpr glm::vec3 crabColors[] = {
      {0.58f,0.20f,0.10f},
      {0.68f,0.29f,0.12f},
      {0.50f,0.17f,0.09f}
  };
  glBindVertexArray(crabMesh.vao);
  for (int i = 0; i < 3; ++i) {
      const float cycle = std::fmod(currentFrame + static_cast<float>(i) * 2.7f, 10.0f);
      glm::mat4 crabModel(1.0f);
      crabModel = glm::translate(crabModel, crabPositions[i]);
      crabModel = glm::rotate(crabModel, crabHeadings[i], glm::vec3(0.0f, 1.0f, 0.0f));
      crabModel = glm::scale(crabModel, glm::vec3(0.34f + i * 0.035f));
      decorShader.setMat4("model", glm::value_ptr(crabModel));
      setShaderVec3(decorShader, "baseColor", crabColors[i]);
      glDrawElements(GL_TRIANGLES, crabMesh.indexCount, GL_UNSIGNED_INT, nullptr);
  }

  constexpr glm::vec3 seaweedPositions[] = {
      {-9.5f,0.0f,-3.0f},{-7.8f,0.0f,-5.5f},{-9.0f,0.0f,-9.0f},
      { 8.0f,0.0f,-8.0f},{ 9.5f,0.0f,-11.0f},{ 7.5f,0.0f,-15.0f}
  };
  for (int i = 0; i < 6; ++i) {
      glm::mat4 seaweedModel(1.0f);
      seaweedModel = glm::translate(seaweedModel, seaweedPositions[i]);
      seaweedModel = glm::rotate(seaweedModel, std::sin(currentFrame * 0.32f + i) * 0.10f, glm::vec3(0.0f, 0.0f, 1.0f));
      seaweedModel = glm::rotate(seaweedModel, glm::radians(static_cast<float>(i * 41)), glm::vec3(0.0f, 1.0f, 0.0f));
      seaweedModel = glm::scale(seaweedModel, glm::vec3(1.0f, 1.8f + (i % 3) * 0.45f, 1.0f));
      decorShader.setMat4("model", glm::value_ptr(seaweedModel));
      setShaderVec3(decorShader, "baseColor", glm::vec3(0.06f, 0.28f, 0.12f));
      seaweed.draw();
  }

  glEnable(GL_BLEND);
  glDepthMask(GL_FALSE);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  surfaceShader.use();
  surfaceShader.setMat4("view", glm::value_ptr(view));
  surfaceShader.setMat4("projection", glm::value_ptr(projection));
  setShaderVec3(surfaceShader, "cameraPos", camera.position());
  setShaderVec3(surfaceShader, "sunDirection", sunDirection);
  surfaceShader.setFloat("time", currentFrame);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, transparentMeshes.waterNormalTexture);
  glBindVertexArray(transparentMeshes.surfaceVao);
  glDrawElements(GL_TRIANGLES, transparentMeshes.surfaceIndexCount, GL_UNSIGNED_INT, nullptr);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  rayShader.use();
  rayShader.setMat4("view", glm::value_ptr(view));
  rayShader.setMat4("projection", glm::value_ptr(projection));
  setShaderVec3(rayShader, "cameraPos", camera.position());
  setShaderVec3(rayShader, "sunDirection", sunDirection);
  rayShader.setFloat("time", currentFrame);
  glBindVertexArray(transparentMeshes.quadVao);
  constexpr glm::vec3 rayPositions[] = {
      {-4.8f, 10.0f, -9.0f}, {-3.8f, 10.0f, -10.5f}, {-2.9f, 10.0f, -12.0f},
      {-2.0f, 10.0f, -9.5f}, {-1.0f, 10.0f, -13.0f}, { 0.0f, 10.0f, -10.5f},
      { 1.0f, 10.0f, -13.5f}, { 2.0f, 10.0f, -9.5f}, { 3.0f, 10.0f, -12.0f},
      { 4.2f, 10.0f, -10.5f}
  };
  for (int i = 0; i < 10; ++i) {
      glm::mat4 rayModel(1.0f);
      rayModel = glm::translate(rayModel, rayPositions[i]);
      rayModel = glm::rotate(rayModel, glm::radians(-15.0f + i * 1.2f), glm::vec3(0.0f, 0.0f, 1.0f));
      rayModel = glm::rotate(rayModel, glm::radians(-18.0f + i * 4.0f), glm::vec3(0.0f, 1.0f, 0.0f));
      rayModel = glm::scale(rayModel, glm::vec3(2.6f + (i % 4) * 0.75f, 12.0f + (i % 3) * 1.5f, 1.0f));
      rayShader.setMat4("model", glm::value_ptr(rayModel));
      rayShader.setFloat("rayOffset", static_cast<float>(i) * 1.7f);
      glDrawArrays(GL_TRIANGLES, 0, 6);
  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  particleShader.use();
  particleShader.setMat4("view", glm::value_ptr(view));
  particleShader.setMat4("projection", glm::value_ptr(projection));
  setShaderVec3(particleShader, "cameraPos", camera.position());
  particleShader.setFloat("time", currentFrame);
  glBindVertexArray(particles.vao);
  glDrawArrays(GL_POINTS, 0, particles.count);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  algaeShader.use();
  algaeShader.setMat4("view", glm::value_ptr(view));
  algaeShader.setMat4("projection", glm::value_ptr(projection));
  setShaderVec3(algaeShader, "cameraPos", camera.position());
  algaeShader.setFloat("time", currentFrame);
  constexpr glm::vec3 algaePositions[] = {
      {-10.5f, 10.0f, -7.0f}, {-8.2f, 10.0f, -10.0f},
      { 8.8f, 10.0f, -9.0f}, {10.8f, 10.0f, -12.0f}
  };
  for (int i = 0; i < 4; ++i) {
      glm::mat4 algaeModel(1.0f);
      algaeModel = glm::translate(algaeModel, algaePositions[i]);
      algaeModel = glm::rotate(algaeModel, glm::radians(i < 2 ? -18.0f : 18.0f), glm::vec3(0.0f, 1.0f, 0.0f));
      algaeModel = glm::scale(algaeModel, glm::vec3(1.0f, 3.4f + (i % 2) * 0.8f, 1.0f));
      algaeShader.setMat4("model", glm::value_ptr(algaeModel));
      algaeShader.setFloat("swayOffset", static_cast<float>(i) * 1.9f);
      glDrawArrays(GL_TRIANGLES, 0, 6);
  }

  constexpr glm::vec3 seabedPlantPositions[] = {
      {-10.5f, 0.0f, -4.0f}, {-8.8f, 0.0f, -7.5f}, {-7.3f, 0.0f,-10.0f},
      {  8.0f, 0.0f,-10.5f}, { 9.5f, 0.0f,-13.0f}, {10.8f, 0.0f,-16.0f}
  };
  for (int i = 0; i < 6; ++i) {
      glm::mat4 plantModel(1.0f);
      plantModel = glm::translate(plantModel, seabedPlantPositions[i]);
      plantModel = glm::rotate(plantModel, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      plantModel = glm::rotate(plantModel, glm::radians(i < 3 ? -15.0f : 15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
      plantModel = glm::scale(plantModel, glm::vec3(1.0f, 2.2f + (i % 3) * 0.55f, 1.0f));
      algaeShader.setMat4("model", glm::value_ptr(plantModel));
      algaeShader.setFloat("swayOffset", static_cast<float>(i) * 1.3f + 0.7f);
      glDrawArrays(GL_TRIANGLES, 0, 6);
  }

  glDepthMask(GL_TRUE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);


}

