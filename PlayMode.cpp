#include "PlayMode.hpp"
#include <string>
#include <unordered_map>
#include <vector>

#include "LitColorTextureProgram.hpp"

#include "Load.hpp"
#include "Mesh.hpp"
#include "WinMode.hpp"
#include "data_path.hpp"
#include "gl_errors.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

// send message we picked up object
static void send_pickup(Connection &conn, uint8_t obj_index) {
  conn.send(Message::C2S_Pickup);
  uint32_t size = 1;
  conn.send(uint8_t(size));
  conn.send(uint8_t(size >> 8));
  conn.send(uint8_t(size >> 16));
  conn.send(obj_index);
}

GLuint basket_vao_for_lit = 0;
Load<MeshBuffer> basket_meshes(LoadTagDefault, []() -> MeshBuffer const * {
  MeshBuffer const *ret = new MeshBuffer(data_path("basket.pnct"));
  basket_vao_for_lit =
      ret->make_vao_for_program(lit_color_texture_program->program);
  return ret;
});

GLuint gift_vao_for_lit = 0;
Load<MeshBuffer> gift_meshes(LoadTagDefault, []() -> MeshBuffer const * {
  MeshBuffer const *ret = new MeshBuffer(data_path("gift.pnct"));
  gift_vao_for_lit =
      ret->make_vao_for_program(lit_color_texture_program->program);
  return ret;
});

GLuint carrot_vao_for_lit = 0;
Load<MeshBuffer> carrot_meshes(LoadTagDefault, []() -> MeshBuffer const * {
  MeshBuffer const *ret = new MeshBuffer(data_path("carrot.pnct"));
  carrot_vao_for_lit =
      ret->make_vao_for_program(lit_color_texture_program->program);
  return ret;
});

GLuint tomato_vao_for_lit = 0;
Load<MeshBuffer> tomato_meshes(LoadTagDefault, []() -> MeshBuffer const * {
  MeshBuffer const *ret = new MeshBuffer(data_path("tomato.pnct"));
  tomato_vao_for_lit =
      ret->make_vao_for_program(lit_color_texture_program->program);
  return ret;
});

GLuint beet_vao_for_lit = 0;
Load<MeshBuffer> beet_meshes(LoadTagDefault, []() -> MeshBuffer const * {
  MeshBuffer const *ret = new MeshBuffer(data_path("beet.pnct"));
  beet_vao_for_lit =
      ret->make_vao_for_program(lit_color_texture_program->program);
  return ret;
});

GLuint beet_seeds_vao_for_lit = 0;
Load<MeshBuffer> beet_seeds_meshes(LoadTagDefault, []() -> MeshBuffer const * {
  MeshBuffer const *ret = new MeshBuffer(data_path("beetseeds.pnct"));
  beet_seeds_vao_for_lit =
      ret->make_vao_for_program(lit_color_texture_program->program);
  return ret;
});

GLuint tomato_seeds_vao_for_lit = 0;
Load<MeshBuffer>
tomato_seeds_meshes(LoadTagDefault, []() -> MeshBuffer const * {
  MeshBuffer const *ret = new MeshBuffer(data_path("tomatoseeds.pnct"));
  tomato_seeds_vao_for_lit =
      ret->make_vao_for_program(lit_color_texture_program->program);
  return ret;
});

GLuint carrot_seeds_vao_for_lit = 0;
Load<MeshBuffer>
carrot_seeds_meshes(LoadTagDefault, []() -> MeshBuffer const * {
  MeshBuffer const *ret = new MeshBuffer(data_path("carrotseeds.pnct"));
  carrot_seeds_vao_for_lit =
      ret->make_vao_for_program(lit_color_texture_program->program);
  return ret;
});

GLuint ground_vao_for_lit = 0;
Load<MeshBuffer> ground_meshes(LoadTagDefault, []() -> MeshBuffer const * {
  MeshBuffer const *ret = new MeshBuffer(data_path("ground.pnct"));
  ground_vao_for_lit =
      ret->make_vao_for_program(lit_color_texture_program->program);
  return ret;
});

Load<Scene> soup_scene(LoadTagDefault, []() -> Scene const * {
  Scene *game_scene = new Scene();

  auto load_scene = [&](const char *scene_path, MeshBuffer const *buf,
                        GLuint vao) {
    game_scene->load(data_path(scene_path),
                     [&](Scene &scene, Scene::Transform *transform,
                         std::string const &mesh_name) {
                       Mesh const &mesh = buf->lookup(mesh_name);

                       scene.drawables.emplace_back(transform);
                       Scene::Drawable &drawable = scene.drawables.back();

                       drawable.pipeline = lit_color_texture_program_pipeline;

                       drawable.pipeline.vao = vao;
                       drawable.pipeline.type = mesh.type;
                       drawable.pipeline.start = mesh.start;
                       drawable.pipeline.count = mesh.count;
                     });
  };

  load_scene("basket.scene", &*basket_meshes, basket_vao_for_lit);
  load_scene("carrot.scene", &*carrot_meshes, carrot_vao_for_lit);
  load_scene("tomato.scene", &*tomato_meshes, tomato_vao_for_lit);
  load_scene("beet.scene", &*beet_meshes, beet_vao_for_lit);
  load_scene("beetseeds.scene", &*beet_seeds_meshes, beet_seeds_vao_for_lit);
  load_scene("ground.scene", &*ground_meshes, ground_vao_for_lit);
  load_scene("carrotseeds.scene", &*carrot_seeds_meshes,
             carrot_seeds_vao_for_lit);
  load_scene("tomatoseeds.scene", &*tomato_seeds_meshes,
             tomato_seeds_vao_for_lit);
  load_scene("gift.scene", &*gift_meshes, gift_vao_for_lit);

  return game_scene;
});

// get meshes at root
static std::vector<Scene::Transform *> get_meshes(Scene &scene,
                                                  Scene::Transform *root) {
  std::vector<Scene::Transform *> transforms;
  for (auto &t : scene.transforms) {
    Scene::Transform *tp = &t;
    for (Scene::Transform *p = tp; p != nullptr; p = p->parent) {
      if (p == root) {
        transforms.push_back(tp);
        break;
      }
    }
  }
  return transforms;
}

// Duplicate meshes at root
static Scene::Transform *duplicate_meshes(Scene &scene, Scene::Transform *root,
                                          std::string const &suffix) {
  std::vector<Scene::Transform *> nodes = get_meshes(scene, root);
  std::unordered_map<Scene::Transform *, Scene::Transform *> xform_map;
  for (auto *oldt : nodes) {
    scene.transforms.emplace_back();
    Scene::Transform *nt = &scene.transforms.back();
    nt->name = oldt->name + suffix;
    nt->position = oldt->position;
    nt->rotation = oldt->rotation;
    nt->scale = oldt->scale;
    nt->parent = nullptr;
    xform_map[oldt] = nt;
  }
  for (auto *oldt : nodes) {
    Scene::Transform *nt = xform_map[oldt];
    if (oldt->parent && xform_map.count(oldt->parent))
      nt->parent = xform_map[oldt->parent];
    else
      nt->parent = nullptr;
  }
  // duplicate drawables referencing this subtree to reference new nodes
  for (auto &d : scene.drawables) {
    if (xform_map.count(d.transform)) {
      scene.drawables.emplace_back(xform_map[d.transform]);
      scene.drawables.back().pipeline = d.pipeline;
    }
  }
  return xform_map.count(root) ? xform_map[root] : nullptr;
}

PlayMode::PlayMode(Client &client_) : client(client_), scene(*soup_scene) {
  for (auto &transform : scene.transforms) {
    if (transform.name == "basket_root")
      basket_root = &transform;
    else if (transform.name == "carrot_root")
      carrot_root = &transform;
    else if (transform.name == "ground_root")
      ground_root = &transform;
    else if (transform.name == "carrot_seeds_root")
      carrot_seeds_root = &transform;
    else if (transform.name == "gift_root")
      gift_root = &transform;
    else if (transform.name == "tomato_root")
      tomato_root = &transform;
    else if (transform.name == "beet_root")
      beet_root = &transform;
    else if (transform.name == "tomato_seeds_root")
      tomato_seeds_root = &transform;
    else if (transform.name == "beet_seeds_root")
      beet_seeds_root = &transform;
  }

  if (basket_root == nullptr)
    throw std::runtime_error("skewer_root not found.");
  if (carrot_root == nullptr)
    throw std::runtime_error("carrot_root not found.");
  if (carrot_seeds_root == nullptr)
    throw std::runtime_error("carrot_seeds_root not found.");
  if (ground_root == nullptr)
    throw std::runtime_error("ground_root not found.");

  basket_root->rotation = glm::quat_cast(glm::mat4(1.0f));

  // set garden positions
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> garden_dist(-20.0f, 20.0f);

  garden_objects = {carrot_root,       carrot_seeds_root, tomato_root,
                    tomato_seeds_root, beet_root,         beet_seeds_root};
  // type for each garden object
  garden_types = {uint8_t(0), uint8_t(1), uint8_t(2),
                  uint8_t(3), uint8_t(4), uint8_t(5)};
  for (size_t gi = 0; gi < garden_objects.size(); ++gi) {
    Scene::Transform *obj = garden_objects[gi];
    obj->position.x = garden_dist(gen);
    obj->position.y = garden_dist(gen);
  }

  pickup_sent.clear();
  pickup_sent.resize(garden_objects.size(), false);

  // make initial clones
  for (int i = 0; i < 5; ++i) {
    // carrot clones
    if (carrot_root) {
      Scene::Transform *new_root =
          duplicate_meshes(scene, carrot_root, std::string("_init"));
      if (new_root)
        new_root->position =
            glm::vec3(garden_dist(gen), garden_dist(gen), 0.0f);
      garden_objects.push_back(new_root);
      garden_types.push_back(uint8_t(0));
      pickup_sent.push_back(false);
    }

    // tomato clones
    if (tomato_root) {
      Scene::Transform *new_root =
          duplicate_meshes(scene, tomato_root, std::string("_init"));
      if (new_root)
        new_root->position =
            glm::vec3(garden_dist(gen), garden_dist(gen), 0.0f);
      garden_objects.push_back(new_root);
      garden_types.push_back(uint8_t(2));
      pickup_sent.push_back(false);
    }

    // beet clones
    if (beet_root) {
      Scene::Transform *new_root =
          duplicate_meshes(scene, beet_root, std::string("_init"));
      if (new_root)
        new_root->position =
            glm::vec3(garden_dist(gen), garden_dist(gen), 0.0f);
      garden_objects.push_back(new_root);
      garden_types.push_back(uint8_t(4));
      pickup_sent.push_back(false);
    }
  }

  // get pointer to camera for convenience:
  if (scene.cameras.size() != 1)
    throw std::runtime_error(
        "Expecting scene to have exactly one camera, but it has " +
        std::to_string(scene.cameras.size()));
  camera = &scene.cameras.front();

  gift_root->parent = camera->transform;
  gift_root->position = glm::vec3(-3.0f, 3.0f, -20.0f);
  {
    glm::quat qx =
        glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat qz =
        glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    gift_root->rotation = qz * qx;
  }

  // initially hide gift far away
  this->gift_visible_pos = gift_root->position;
  gift_root->position = glm::vec3(0.0f, 0.0f, -1000.0f);
}

PlayMode::~PlayMode() {}

bool PlayMode::handle_event(SDL_Event const &evt,
                            glm::uvec2 const &window_size) {

  if (evt.type == SDL_EVENT_KEY_DOWN) {
    if (evt.key.repeat) {
      // ignore repeats
    } else if (evt.key.key == SDLK_A) {
      controls.left.downs += 1;
      controls.left.pressed = true;
      return true;
    } else if (evt.key.key == SDLK_D) {
      controls.right.downs += 1;
      controls.right.pressed = true;
      return true;
    } else if (evt.key.key == SDLK_W) {
      controls.up.downs += 1;
      controls.up.pressed = true;
      return true;
    } else if (evt.key.key == SDLK_S) {
      controls.down.downs += 1;
      controls.down.pressed = true;
      return true;
    } else if (evt.key.key == SDLK_SPACE) {
      controls.jump.downs += 1;
      controls.jump.pressed = true;
      return true;
    }
  } else if (evt.type == SDL_EVENT_KEY_UP) {
    if (evt.key.key == SDLK_A) {
      controls.left.pressed = false;
      return true;
    } else if (evt.key.key == SDLK_D) {
      controls.right.pressed = false;
      return true;
    } else if (evt.key.key == SDLK_W) {
      controls.up.pressed = false;
      return true;
    } else if (evt.key.key == SDLK_S) {
      controls.down.pressed = false;
      return true;
    } else if (evt.key.key == SDLK_SPACE) {
      controls.jump.pressed = false;
      return true;
    }
  }

  return false;
}

void PlayMode::update(float elapsed) {
  controls.send_controls_message(&client.connection);

  // reset button press counters:
  controls.left.downs = 0;
  controls.right.downs = 0;
  controls.up.downs = 0;
  controls.down.downs = 0;
  controls.jump.downs = 0;

  // send/receive data:
  client.poll(
      [this](Connection *c, Connection::Event event) {
        if (event == Connection::OnOpen) {
          std::cout << "[" << c->socket << "] opened" << std::endl;
        } else if (event == Connection::OnClose) {
          std::cout << "[" << c->socket << "] closed (!)" << std::endl;
          throw std::runtime_error("Lost connection to server!");
        } else {
          assert(event == Connection::OnRecv);
          // std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n"
          // << hex_dump(c->recv_buffer); std::cout.flush(); //DEBUG
          bool handled_message;
          try {
            do {
              handled_message = false;
              if (game.recv_state_message(c))
                handled_message = true;
              if (game.recv_gift_message(c))
                handled_message = true;
              if (game.recv_win_message(c))
                handled_message = true;
            } while (handled_message);
          } catch (std::exception const &e) {
            std::cerr << "[" << c->socket
                      << "] malformed message from server: " << e.what()
                      << std::endl;
            // quit the game:
            throw e;
          }
        }
      },
      0.0);

  if (game.win) {
    Mode::set_current(std::make_shared<WinMode>());
    return;
  }

  static float rotation = 0.0f;
  float amt = 0.0f;
  if (controls.left.pressed && !controls.right.pressed)
    amt += 1.0f;
  if (!controls.left.pressed && controls.right.pressed)
    amt -= 1.0f;
  rotation += amt * 1.8f * elapsed;

  // Actual Z rotation
  glm::mat4 zrot_mat =
      glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0, 0, 1));

  basket_root->rotation = glm::quat_cast(zrot_mat);

  // Camera does z rotation only
  glm::mat4 cam_rot_mat = zrot_mat;
  glm::quat cam_rot = glm::quat_cast(cam_rot_mat);

  amt = 0.0f;
  if (controls.up.pressed && !controls.down.pressed)
    amt = +1.0f;
  else if (!controls.up.pressed && controls.down.pressed)
    amt = -1.0f;
  float yaw = rotation; // rotation variable is the accumulated Z rotation
  glm::vec2 dir2 = glm::vec2(std::sin(yaw), -std::cos(yaw));
  glm::vec3 move_vec = glm::vec3(dir2.x, dir2.y, 0.0f);
  basket_root->position += move_vec * (amt * 20.0f * elapsed);

  // Set camera to follow basket
  {
    const glm::vec3 camera_offset(0.0f, 23.0f, 8.0f);
    camera->transform->position =
        basket_root->position + cam_rot * camera_offset;
    camera->transform->rotation = glm::quat_cast(
        glm::inverse(glm::lookAt(camera->transform->position,
                                 basket_root->position, glm::vec3(0, 0, 1))));
  }

  // check for picking up garden objects
  for (size_t gi = 0; gi < garden_objects.size(); ++gi) {
    Scene::Transform *g = garden_objects[gi];
    if (!g)
      continue;

    glm::vec2 dist = glm::vec2(g->position.x - basket_root->position.x,
                               g->position.y - basket_root->position.y);
    if (glm::length(dist) < 2.0f) {
      if (gi >= pickup_sent.size() || pickup_sent[gi])
        continue;

      uint8_t send_type = 0xFF;
      if (gi < garden_types.size())
        send_type = garden_types[gi];
      if (send_type != 0xFF) {
        send_pickup(client.connection, send_type);
      }
      if (gi < pickup_sent.size())
        pickup_sent[gi] = true;

      // if seed, spawn it in a diff location
      if (send_type == 1 || send_type == 3 || send_type == 5) {
        if (gi < garden_objects.size() && garden_objects[gi]) {
          Scene::Transform *t = garden_objects[gi];
          std::random_device rd_r2;
          std::mt19937 gen_r2(rd_r2());
          std::uniform_real_distribution<float> garden_dist(-20.0f, 20.0f);
          t->position = glm::vec3(garden_dist(gen_r2), garden_dist(gen_r2), 0.0f);

          if (gi < pickup_sent.size())
            pickup_sent[gi] = false;
        }
        continue;
      }

      client.poll();

      std::vector<Scene::Transform *> to_remove;
      for (auto &t : scene.transforms) {
        Scene::Transform *tp = &t;
        for (Scene::Transform *p = tp; p != nullptr; p = p->parent) {
          if (p == g) {
            to_remove.push_back(tp);
            break;
          }
        }
      }

      for (auto it = scene.drawables.begin(); it != scene.drawables.end();) {
        if (std::find(to_remove.begin(), to_remove.end(), it->transform) !=
            to_remove.end()) {
          it = scene.drawables.erase(it);
        } else {
          ++it;
        }
      }

      for (auto it = scene.transforms.begin(); it != scene.transforms.end();) {
        Scene::Transform *tp = &*it;
        if (std::find(to_remove.begin(), to_remove.end(), tp) !=
            to_remove.end()) {
          it = scene.transforms.erase(it);
        } else {
          ++it;
        }
      }

      for (size_t k = 0; k < garden_objects.size(); ++k) {
        if (garden_objects[k] == nullptr)
          continue;
        if (std::find(to_remove.begin(), to_remove.end(), garden_objects[k]) !=
            to_remove.end()) {
          garden_objects[k] = nullptr;
          if (k < garden_types.size())
            garden_types[k] = 0xFF;
        }
      }

      std::cout << "picked garden object " << gi << std::endl;
    }
  }

  // process gifts from server
  if (!game.my_gifts.empty()) {
    std::random_device rd_g;
    std::mt19937 gen_g(rd_g());
    std::uniform_real_distribution<float> gift_dist(-20.0f, 20.0f);
    while (!game.my_gifts.empty()) {
      uint8_t gift_type = game.my_gifts.front();
      game.my_gifts.pop_front();

      // choose prototype root based on gift type
      Scene::Transform *root = nullptr;
      if (gift_type == 2) {
        root = tomato_root;
      } else if (gift_type == 0) {
        root = carrot_root;
      } else if (gift_type == 4) {
        root = beet_root;
      }

      Scene::Transform *new_root =
          duplicate_meshes(scene, root, std::string("_gift"));
      if (new_root)
        new_root->position =
            glm::vec3(gift_dist(gen_g), gift_dist(gen_g), 0.0f);
      garden_objects.push_back(new_root);
      pickup_sent.push_back(false);
    }

    // restore gift by moving it back to its visible position
    if (!gifts_shown) {
      gift_root->position = gift_visible_pos;
      gifts_shown = true;
      gifts_shown_time = 0.0f;
    }
  }

  if (gifts_shown) {
    gifts_shown_time += elapsed;
    if (gifts_shown_time >= gifts_hide_after) {
      // hide gift by moving it offscreen
      gift_root->position = glm::vec3(0.0f, 0.0f, -1000.0f);
      gifts_shown = false;
    }
  }
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

  // update camera aspect ratio for drawable:
  camera->aspect = float(drawable_size.x) / float(drawable_size.y);

  glUseProgram(lit_color_texture_program->program);
  glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
  glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1,
               glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
  glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1,
               glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
  glUseProgram(0);

  glClearColor(0.02f, 0.02f, 0.08f, 1.0f);
  glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth
                      // buffer to, but FYI you can change it.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS); // this is the default depth comparison function, but
                        // FYI you can change it.

  scene.draw(*camera);

  GL_ERRORS();
}
