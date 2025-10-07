#pragma once

#include "Mode.hpp"
#include "Scene.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

struct WinMode : Mode {
  WinMode();
  virtual ~WinMode();

  virtual bool handle_event(SDL_Event const &evt,
                            glm::uvec2 const &window_size) override;
  virtual void draw(glm::uvec2 const &drawable_size) override;

  Scene scene;
  Scene::Camera *camera = nullptr;
  Scene::Transform *bowl_root = nullptr;
};
