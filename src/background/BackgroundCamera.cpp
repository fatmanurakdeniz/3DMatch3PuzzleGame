#include "background/BackgroundCamera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace ocean_bg {

BackgroundCamera::BackgroundCamera(const glm::vec3 &position)
    : position_(position) {
  updateVectors();
}

glm::mat4 BackgroundCamera::viewMatrix() const {
  return glm::lookAt(position_, position_ + front_, up_);
}

const glm::vec3 &BackgroundCamera::position() const { return position_; }

float BackgroundCamera::fov() const { return fov_; }

void BackgroundCamera::updateVectors() {
  const float yaw = glm::radians(yaw_);
  const float pitch = glm::radians(pitch_);
  front_ = glm::normalize(glm::vec3(std::cos(yaw) * std::cos(pitch),
                                    std::sin(pitch),
                                    std::sin(yaw) * std::cos(pitch)));
  right_ = glm::normalize(glm::cross(front_, worldUp_));
  up_ = glm::normalize(glm::cross(right_, front_));
}

} // namespace ocean_bg
