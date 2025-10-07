#include "Mode.hpp"

#include "Connection.hpp"
#include "Game.hpp"

#include "Scene.hpp"
#include <glm/glm.hpp>

#include <string>

#include <deque>
#include <vector>

struct PlayMode : Mode {
  PlayMode(Client &client);
  virtual ~PlayMode();

  // functions called by main loop:
  virtual bool handle_event(SDL_Event const &,
                            glm::uvec2 const &window_size) override;
  virtual void update(float elapsed) override;
  virtual void draw(glm::uvec2 const &drawable_size) override;

  //----- game state -----

  // input tracking for local player:
  Player::Controls controls;

  // latest game state (from server):
  Game game;

  // last message from server:
  std::string server_message;

  // connection to server:
  Client &client;

  Scene scene;

  Scene::Transform *ground_root = nullptr;
  Scene::Transform *basket_root = nullptr;
  Scene::Transform *carrot_root = nullptr;
  Scene::Transform *tomato_root = nullptr;
  Scene::Transform *beet_root = nullptr;
  Scene::Transform *carrot_seeds_root = nullptr;
  Scene::Transform *tomato_seeds_root = nullptr;
  Scene::Transform *beet_seeds_root = nullptr;
  Scene::Transform *pot_root = nullptr;
  Scene::Transform *gift_root = nullptr;
  Scene::Camera *camera = nullptr;

  // references to garden objects
  std::vector<Scene::Transform *> garden_objects;
  // 0=carrot, 1=carrot_seed, 2=tomato, 3=tomato_seed, 4=beet, 5=beet_seed
  std::vector<uint8_t> garden_types;

  std::vector<bool> pickup_sent;

  bool gifts_shown = false;
  float gifts_shown_time = 0.0f;
  const float gifts_hide_after = 3.0f;
  glm::vec3 gift_visible_pos = glm::vec3(0.0f);
};
