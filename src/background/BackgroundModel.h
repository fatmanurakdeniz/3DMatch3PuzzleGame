#pragma once

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

namespace ocean_bg {

class BackgroundModel {
public:
  explicit BackgroundModel(const std::string &path, bool normalize = false);
  ~BackgroundModel();

  BackgroundModel(const BackgroundModel &) = delete;
  BackgroundModel &operator=(const BackgroundModel &) = delete;

  void draw() const;

private:
  struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
  };

  std::vector<Mesh> meshes_;
};

} // namespace ocean_bg
