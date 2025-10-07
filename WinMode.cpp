#include "WinMode.hpp"
#include "DrawLines.hpp"
#include "GL.hpp"
#include "LitColorTextureProgram.hpp"
#include "Load.hpp"
#include "Mesh.hpp"
#include "PathFont.hpp"
#include "data_path.hpp"
#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

// Load bowl mesh
GLuint bowl_vao_for_lit = 0;
Load<MeshBuffer> bowl_meshes(LoadTagDefault, []() -> MeshBuffer const * {
  MeshBuffer const *ret = new MeshBuffer(data_path("bowl.pnct"));
  bowl_vao_for_lit =
      ret->make_vao_for_program(lit_color_texture_program->program);
  return ret;
});

// Load a small scene containing the bowl (bowl.scene expected in data)
Load<Scene> win_scene(LoadTagDefault, []() -> Scene const * {
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

  load_scene("bowl.scene", &*bowl_meshes, bowl_vao_for_lit);
  return game_scene;
});

WinMode::WinMode() : scene(*win_scene) {
  SDL_SetWindowTitle(Mode::window, "Fall harvest 🍂 - nice harvest, enjoy the soup!");

  // find a camera in the loaded scene (there should be one)
  if (scene.cameras.size() > 0)
    camera = &scene.cameras.front();

  // find bowl_root transform if present
  for (auto &transform : scene.transforms) {
    if (transform.name == "bowl_root")
      bowl_root = &transform;
  }

  // Actual Z rotation
  glm::mat4 zrot_mat = glm::rotate(glm::mat4(1.0f), 0.f, glm::vec3(0, 0, 1));

  bowl_root->rotation = glm::quat_cast(zrot_mat);

  // Camera does z rotation only
  glm::mat4 cam_rot_mat = zrot_mat;
  glm::quat cam_rot = glm::quat_cast(cam_rot_mat);

  {
    const glm::vec3 camera_offset(0.0f, 5.0f, 8.0f);
    camera->transform->position = bowl_root->position + cam_rot * camera_offset;
    camera->transform->rotation = glm::quat_cast(glm::inverse(glm::lookAt(
        camera->transform->position, bowl_root->position, glm::vec3(0, 0, 1))));
  }
}

WinMode::~WinMode() {}

bool WinMode::handle_event(SDL_Event const &evt,
                           glm::uvec2 const &window_size) {
  if (evt.type == SDL_EVENT_QUIT) {
    Mode::set_current(nullptr);
    return true;
  }
  // ignore key presses in win mode (do not exit on key down)
  if (evt.type == SDL_EVENT_KEY_DOWN) {
    return true;
  }
  return false;
}

void WinMode::draw(glm::uvec2 const &drawable_size) {
  // update camera aspect ratio for drawable:
  camera->aspect = float(drawable_size.x) / float(drawable_size.y);

  // set up light type and position for lit_color_texture_program:
  //  TODO: consider using the Light(s) in the scene to do this
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
}
