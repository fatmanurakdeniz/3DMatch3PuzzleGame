#include "background/BackgroundModel.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

namespace ocean_bg {
namespace {
struct Vertex {
  float position[3];
  float normal[3];
};
} // namespace

BackgroundModel::BackgroundModel(const std::string &path, bool normalize) {
  Assimp::Importer importer;
  const aiScene *scene =
      importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                  aiProcess_JoinIdenticalVertices |
                                  aiProcess_ImproveCacheLocality |
                                  aiProcess_PreTransformVertices);
  if (!scene || !scene->mRootNode) {
    throw std::runtime_error("Assimp failed to load model: " + path);
  }

  aiVector3D minimum(std::numeric_limits<float>::max());
  aiVector3D maximum(-std::numeric_limits<float>::max());
  if (normalize) {
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes;
         ++meshIndex) {
      const aiMesh *source = scene->mMeshes[meshIndex];
      for (unsigned int i = 0; i < source->mNumVertices; ++i) {
        const aiVector3D &p = source->mVertices[i];
        minimum.x = std::min(minimum.x, p.x);
        minimum.y = std::min(minimum.y, p.y);
        minimum.z = std::min(minimum.z, p.z);
        maximum.x = std::max(maximum.x, p.x);
        maximum.y = std::max(maximum.y, p.y);
        maximum.z = std::max(maximum.z, p.z);
      }
    }
  }
  const aiVector3D center = (minimum + maximum) * 0.5f;
  const aiVector3D extent = maximum - minimum;
  const float normalizationScale =
      normalize ? 2.0f / std::max(extent.x, std::max(extent.y, extent.z))
                : 1.0f;

  for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes;
       ++meshIndex) {
    const aiMesh *source = scene->mMeshes[meshIndex];
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(source->mNumVertices);

    for (unsigned int i = 0; i < source->mNumVertices; ++i) {
      aiVector3D position = source->mVertices[i];
      if (normalize) {
        position = (position - center) * normalizationScale;
      }
      const aiVector3D normal =
          source->HasNormals() ? source->mNormals[i] : aiVector3D(0.0f, 1.0f, 0.0f);
      vertices.push_back({{position.x, position.y, position.z},
                          {normal.x, normal.y, normal.z}});
    }

    for (unsigned int i = 0; i < source->mNumFaces; ++i) {
      const aiFace &face = source->mFaces[i];
      for (unsigned int j = 0; j < face.mNumIndices; ++j) {
        indices.push_back(face.mIndices[j]);
      }
    }

    Mesh mesh;
    mesh.indexCount = static_cast<GLsizei>(indices.size());
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    meshes_.push_back(mesh);
  }
}

BackgroundModel::~BackgroundModel() {
  for (const Mesh &mesh : meshes_) {
    glDeleteBuffers(1, &mesh.ebo);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteVertexArrays(1, &mesh.vao);
  }
}

void BackgroundModel::draw() const {
  for (const Mesh &mesh : meshes_) {
    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
  }
}

} // namespace ocean_bg
