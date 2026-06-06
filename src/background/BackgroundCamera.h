#pragma once

#include <glm/glm.hpp>

namespace ocean_bg {

class BackgroundCamera {
public:
  explicit BackgroundCamera(
      const glm::vec3 &position = glm::vec3(0.0f, 1.2f, 6.0f));

  glm::mat4 viewMatrix() const;
  const glm::vec3 &position() const;
  float fov() const;

private:
  void updateVectors();

  glm::vec3 position_;
  glm::vec3 front_;
  glm::vec3 up_;
  glm::vec3 right_;
  const glm::vec3 worldUp_ = glm::vec3(0.0f, 1.0f, 0.0f);

  float yaw_ = -90.0f;
  float pitch_ = -8.53f;
  float fov_ = 75.0f;
};

} // namespace ocean_bg
