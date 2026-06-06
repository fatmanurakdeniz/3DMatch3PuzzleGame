#pragma once

#include <memory>

class OceanBackgroundRenderer {
public:
  OceanBackgroundRenderer();
  ~OceanBackgroundRenderer();

  void init();
  void render(float time, int framebufferWidth, int framebufferHeight);

private:
  struct State;
  std::unique_ptr<State> state;
};
