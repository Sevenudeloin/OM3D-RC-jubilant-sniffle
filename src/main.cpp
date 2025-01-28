
#include "ImageFormat.h"
#include "SceneObject.h"
#include "defines.h"
#include <cmath>
#include <glad/gl.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "shader_structs.h"
#include "graphics.h"
#include "Scene.h"
#include "Texture.h"
#include "Framebuffer.h"
#include "TimestampQuery.h"
#include "ImGuiRenderer.h"

#include <imgui/imgui.h>

#include <iostream>
#include <vector>
#include <filesystem>

#include <glm/gtx/string_cast.hpp>

using namespace OM3D;


static float delta_time = 0.0f;
static std::unique_ptr<Scene> scene;
// static float exposure = 1.0;
static std::vector<std::string> scene_files;
// static u32 debug_mode = 0;

static glm::dvec2 prev_mouse_pos;
static bool flatland_clear_screen = false;
static float flatland_drawing_color[4] = { 1.0, 1.0, 1.0, 1.0};
static int flatland_line_width = 10; // in pixels

static int rc_base = 4;
static int rc_cascade_count = 10;
static int rc_cascade_index = 0; // last cascade to be drawn
static int rc_debug_display = 0; // 0 final rc, 1 SDF, 2 JFA, 3 draw

namespace OM3D {
extern bool audit_bindings_before_draw;
}

void parse_args(int argc, char** argv) {
    for(int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];

        if(arg == "--validate") {
            OM3D::audit_bindings_before_draw = true;
        } else {
            std::cerr << "Unknown argument \"" << arg << "\"" << std::endl;
        }
    }
}

void glfw_check(bool cond) {
    if(!cond) {
        const char* err = nullptr;
        glfwGetError(&err);
        std::cerr << "GLFW error: " << err << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void update_delta_time() {
    static double time = 0.0;
    const double new_time = program_time();
    delta_time = float(new_time - time);
    time = new_time;
}

void process_inputs(GLFWwindow* window, Camera& camera) {
    static glm::dvec2 mouse_pos;

    glm::dvec2 new_mouse_pos;
    glfwGetCursorPos(window, &new_mouse_pos.x, &new_mouse_pos.y);

    {
        glm::vec3 movement = {};
        if(glfwGetKey(window, 'W') == GLFW_PRESS) {
            movement += camera.forward();
        }
        if(glfwGetKey(window, 'S') == GLFW_PRESS) {
            movement -= camera.forward();
        }
        if(glfwGetKey(window, 'D') == GLFW_PRESS) {
            movement += camera.right();
        }
        if(glfwGetKey(window, 'A') == GLFW_PRESS) {
            movement -= camera.right();
        }

        float speed = 10.0f;
        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            speed *= 10.0f;
        }

        if(movement.length() > 0.0f) {
            const glm::vec3 new_pos = camera.position() + movement * delta_time * speed;
            camera.set_view(glm::lookAt(new_pos, new_pos + camera.forward(), camera.up()));
        }
    }

    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        const glm::vec2 delta = glm::vec2(mouse_pos - new_mouse_pos) * 0.01f;
        if(delta.length() > 0.0f) {
            glm::mat4 rot = glm::rotate(glm::mat4(1.0f), delta.x, glm::vec3(0.0f, 1.0f, 0.0f));
            rot = glm::rotate(rot, delta.y, camera.right());
            camera.set_view(glm::lookAt(camera.position(), camera.position() + (glm::mat3(rot) * camera.forward()), (glm::mat3(rot) * camera.up())));
        }
    }

    {
        int width = 0;
        int height = 0;
        glfwGetWindowSize(window, &width, &height);
        camera.set_ratio(float(width) / float(height));
    }

    mouse_pos = new_mouse_pos;
}

void process_inputs_flatland(GLFWwindow* window, glm::dvec2& mouse_pos, bool& is_drawing) {
    prev_mouse_pos = mouse_pos;

    glfwGetCursorPos(window, &mouse_pos.x, &mouse_pos.y);
    is_drawing = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
}

void gui(ImGuiRenderer& imgui) {
    const ImVec4 error_text_color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
    const ImVec4 warning_text_color = ImVec4(1.0f, 0.8f, 0.4f, 1.0f);

    static bool open_gpu_profiler = false;
    // static bool display_camera_pos = false;
    static bool show_color_picker = false;
    static bool show_width_slider = false;

    PROFILE_GPU("GUI");

    imgui.start();
    DEFER(imgui.finish());

    // ImGui::ShowDemoWindow();

    bool open_scene_popup = false;

    flatland_clear_screen = false;

    if(ImGui::BeginMainMenuBar()) {
        if(ImGui::BeginMenu("File")) {
            if(ImGui::MenuItem("Open Scene")) {
                open_scene_popup = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Clear screen")) {
            flatland_clear_screen = true;
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Drawing options")) {
            if (ImGui::MenuItem("Line color")) {
                show_color_picker = !show_color_picker;
            }
            if (ImGui::MenuItem("Line width")) {
                show_width_slider = !show_width_slider;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("RC Base")) {
            if (ImGui::RadioButton("4", rc_base == 4)) {
                rc_base = 4;
            }
            if (ImGui::RadioButton("16", rc_base == 16)) {
                rc_base = 16;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("RC Cascade Index")) { // so ugly
            if (rc_cascade_count >= 0) {
                if (ImGui::RadioButton("0", rc_cascade_index == 0)) {
                    rc_cascade_index = 0;
                }
            }
            if (rc_cascade_count >= 1) {
                if (ImGui::RadioButton("1", rc_cascade_index == 1)) {
                    rc_cascade_index = 1;
                }
            }
            if (rc_cascade_count >= 2) {
                if (ImGui::RadioButton("2", rc_cascade_index == 2)) {
                    rc_cascade_index = 2;
                }
            }
            if (rc_cascade_count >= 3) {
                if (ImGui::RadioButton("3", rc_cascade_index == 3)) {
                    rc_cascade_index = 3;
                }
            }
            if (rc_cascade_count >= 4) {
                if (ImGui::RadioButton("4", rc_cascade_index == 4)) {
                    rc_cascade_index = 4;
                }
            }
            if (rc_cascade_count >= 5) {
                if (ImGui::RadioButton("5", rc_cascade_index == 5)) {
                    rc_cascade_index = 5;
                }
            }
            if (rc_cascade_count >= 6) {
                if (ImGui::RadioButton("6", rc_cascade_index == 6)) {
                    rc_cascade_index = 1;
                }
            }
            if (rc_cascade_count >= 7) {
                if (ImGui::RadioButton("7", rc_cascade_index == 7)) {
                    rc_cascade_index = 7;
                }
            }
            if (rc_cascade_count >= 8) {
                if (ImGui::RadioButton("8", rc_cascade_index == 8)) {
                    rc_cascade_index = 8;
                }
            }
            if (rc_cascade_count >= 9) {
                if (ImGui::RadioButton("9", rc_cascade_index == 9)) {
                    rc_cascade_index = 9;
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("RC Debug")) {
            if (ImGui::MenuItem("RC Final")) {
                rc_debug_display = 0;
            } else if (ImGui::MenuItem("SDF")) {
                rc_debug_display = 1;
            } else if (ImGui::MenuItem("JFA")) {
                rc_debug_display = 2;
            } else if (ImGui::MenuItem("Draw")) {
                rc_debug_display = 3;
            }
            ImGui::EndMenu();
        }

        // if(ImGui::BeginMenu("Exposure")) {
        //     ImGui::DragFloat("Exposure", &exposure, 0.25f, 0.01f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        //     if(exposure != 1.0f && ImGui::Button("Reset")) {
        //         exposure = 1.0f;
        //     }
        //     ImGui::EndMenu();
        // }

        // if (ImGui::BeginMenu("Debug")) {
        //     if (ImGui::MenuItem("Albedo")) {
        //         debug_mode = 0;
        //     } else if (ImGui::MenuItem("Normals")) {
        //         debug_mode = 1;
        //     } else if (ImGui::MenuItem("Depth")) {
        //         debug_mode = 2;
        //     }
        //     ImGui::EndMenu();
        // }

        if(scene && ImGui::BeginMenu("Scene Info")) {
            ImGui::Text("%u objects", u32(scene->objects().size()));
            ImGui::Text("%u point lights", u32(scene->point_lights().size()));
            ImGui::EndMenu();
        }

        if(ImGui::MenuItem("GPU Profiler")) {
            open_gpu_profiler = true;
        }

        // if(ImGui::MenuItem("Camera pos")) {
        //     display_camera_pos = !display_camera_pos;
        // }

        // ImGui::Separator();
        // ImGui::TextUnformatted(reinterpret_cast<const char*>(glGetString(GL_RENDERER))); // nom de la carte

        ImGui::Separator();
        ImGui::Text("%.2f ms", delta_time * 1000.0f);

#ifdef OM3D_DEBUG
        ImGui::Separator();
        ImGui::TextColored(warning_text_color, ICON_FA_BUG " (DEBUG)");
#endif

        if(!bindless_enabled()) {
            ImGui::Separator();
            ImGui::TextColored(error_text_color, ICON_FA_EXCLAMATION_TRIANGLE " Bindless textures not supported");
        }
        ImGui::EndMainMenuBar();
    }

    if(open_scene_popup) {
        ImGui::OpenPopup("###openscenepopup");

        scene_files.clear();
        for(auto&& entry : std::filesystem::directory_iterator(data_path)) {
            if(entry.status().type() == std::filesystem::file_type::regular) {
                const auto ext = entry.path().extension();
                if(ext == ".gltf" || ext == ".glb") {
                    scene_files.emplace_back(entry.path().string());
                }
            }
        }
    }

    if(ImGui::BeginPopup("###openscenepopup", ImGuiWindowFlags_AlwaysAutoResize)) {
        auto load_scene = [](const std::string path) {
            auto result = Scene::from_gltf(path);
            if(!result.is_ok) {
                std::cerr << "Unable to load scene (" << path << ")" << std::endl;
            } else {
                scene = std::move(result.value);
            }
            ImGui::CloseCurrentPopup();
        };

        char buffer[1024] = {};
        if(ImGui::InputText("Load scene", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            load_scene(buffer);
        }

        if(!scene_files.empty()) {
            for(const std::string& p : scene_files) {
                const auto abs = std::filesystem::absolute(p).string();
                if(ImGui::MenuItem(abs.c_str())) {
                    load_scene(p);
                    break;
                }
            }
        }

        ImGui::EndPopup();
    }

    if(open_gpu_profiler) {
        if(ImGui::Begin(ICON_FA_CLOCK " GPU Profiler")) {
            const ImGuiTableFlags table_flags =
                ImGuiTableFlags_SortTristate |
                ImGuiTableFlags_NoSavedSettings |
                ImGuiTableFlags_SizingFixedFit |
                ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_RowBg;

            ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(1, 1, 1, 0.01f));
            DEFER(ImGui::PopStyleColor());

            if(ImGui::BeginTable("##timetable", 3, table_flags)) {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("CPU (ms)", ImGuiTableColumnFlags_NoResize, 70.0f);
                ImGui::TableSetupColumn("GPU (ms)", ImGuiTableColumnFlags_NoResize, 70.0f);
                ImGui::TableHeadersRow();

                std::vector<u32> indents;
                for(const auto& zone : retrieve_profile()) {
                    auto color_from_time = [](float time) {
                        const float t = std::min(time / 0.008f, 1.0f); // 8ms = red
                        return ImVec4(t, 1.0f - t, 0.0f, 1.0f);
                    };

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(zone.name.data());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushStyleColor(ImGuiCol_Text, color_from_time(zone.cpu_time));
                    ImGui::Text("%.2f", zone.cpu_time * 1000.0f);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::PushStyleColor(ImGuiCol_Text, color_from_time(zone.gpu_time));
                    ImGui::Text("%.2f", zone.gpu_time * 1000.0f);

                    ImGui::PopStyleColor(2);

                    if(!indents.empty() && --indents.back() == 0) {
                        indents.pop_back();
                        ImGui::Unindent();
                    }

                    if(zone.contained_zones) {
                        indents.push_back(zone.contained_zones);
                        ImGui::Indent();
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    if (show_color_picker) {
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Line color");
        ImGui::ColorPicker4("Color", flatland_drawing_color);
        ImGui::End();
    }

    if (show_width_slider) {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        ImGui::Begin("Line width");
        ImGui::DragInt("Width", &flatland_line_width, 0.3f, 1, 50, "%d");
        if(flatland_line_width != 10 && ImGui::Button("Reset")) {
            flatland_line_width = 10;
        }
        ImGui::End();
    }

    // if (display_camera_pos) {
    //     auto cam_view_mat = scene->camera().view_matrix();

    //     glm::mat3 rotation = glm::mat3(cam_view_mat);
    //     glm::vec3 translation = glm::vec3(cam_view_mat[3]);
    //     glm::vec3 cam_pos = -glm::transpose(rotation) * translation;

    //     ImGui::Begin("Camera Position", &display_camera_pos); // Create a window with a close button
    //     ImGui::Text("X: %.3f", cam_pos.x);
    //     ImGui::Text("Y: %.3f", cam_pos.y);
    //     ImGui::Text("Z: %.3f", cam_pos.z);
    //     ImGui::End();
    // }
}




std::unique_ptr<Scene> create_default_scene() {
    auto scene = std::make_unique<Scene>();

    // Load default cube model
    auto result = Scene::from_gltf(std::string(data_path) + "cube.glb");
    ALWAYS_ASSERT(result.is_ok, "Unable to load default scene");
    scene = std::move(result.value);

    scene->set_sun(glm::vec3(0.2f, 1.0f, 0.1f), glm::vec3(1.0f));

    // Add lights
    {
        PointLight light;
        light.set_position(glm::vec3(1.0f, 2.0f, 4.0f));
        light.set_color(glm::vec3(0.0f, 50.0f, 0.0f));
        light.set_radius(100.0f);
        scene->add_light(std::move(light));
    }
    {
        PointLight light;
        light.set_position(glm::vec3(1.0f, 2.0f, -4.0f));
        light.set_color(glm::vec3(50.0f, 0.0f, 0.0f));
        light.set_radius(50.0f);
        scene->add_light(std::move(light));
    }

    return scene;
}

struct RendererState {
    static RendererState create(glm::uvec2 size) {
        RendererState state;

        state.size = size;

        if(state.size.x > 0 && state.size.y > 0) {
            state.depth_texture = Texture(size, ImageFormat::Depth32_FLOAT);
            state.lit_hdr_texture = Texture(size, ImageFormat::RGBA16_FLOAT);
            state.tone_mapped_texture = Texture(size, ImageFormat::RGBA8_UNORM);
            // G-buffer
            state.albedo_texture = Texture(size, ImageFormat::RGB8_sRGB);
            state.normal_texture = Texture(size, ImageFormat::RGB8_UNORM);
            // Flatland
            state.flatland_draw_texture = Texture(size, ImageFormat::RGBA8_UNORM); // For drawn pixels 
            state.flatland_jfa_A_texture = Texture(size, ImageFormat::RG16_FLOAT); // For JFA pipeline
            state.flatland_jfa_B_texture = Texture(size, ImageFormat::RG16_FLOAT); // For JFA pipeline
            state.flatland_jfa_dist_texture = Texture(size, ImageFormat::R16_FLOAT); // Stores scene lights SDF
            state.flatland_scene_A_texture = Texture(size, ImageFormat::RGBA8_UNORM); // For RC pipeline
            state.flatland_scene_B_texture = Texture(size, ImageFormat::RGBA8_UNORM); // For RC pipeline
            state.flatland_final_texture = Texture(size, ImageFormat::RGBA8_UNORM); // Final output

            state.main_framebuffer = Framebuffer(&state.depth_texture, std::array{&state.lit_hdr_texture});
            state.tone_map_framebuffer = Framebuffer(nullptr, std::array{&state.tone_mapped_texture});
            state.g_buffer_framebuffer = Framebuffer(&state.depth_texture, std::array{&state.albedo_texture, &state.normal_texture});
            // Flatland
            state.flatland_framebuffer = Framebuffer(nullptr, std::array{&state.flatland_final_texture});
        }

        return state;
    }

    glm::uvec2 size = {};

    Texture depth_texture;
    Texture lit_hdr_texture;
    Texture tone_mapped_texture;
    // G-buffer
    Texture albedo_texture;
    Texture normal_texture;
    // Flatland
    Texture flatland_draw_texture;
    Texture flatland_jfa_A_texture;
    Texture flatland_jfa_B_texture;
    Texture flatland_jfa_dist_texture;
    Texture flatland_scene_A_texture;
    Texture flatland_scene_B_texture;
    Texture flatland_final_texture;

    Framebuffer z_prepass_framebuffer;
    Framebuffer main_framebuffer;
    Framebuffer tone_map_framebuffer;
    Framebuffer g_buffer_framebuffer;
    Framebuffer flatland_framebuffer;
};

int main(int argc, char** argv) {
    DEBUG_ASSERT([] { std::cout << "Debug asserts enabled" << std::endl; return true; }());

    parse_args(argc, argv);

    glfw_check(glfwInit());
    DEFER(glfwTerminate());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    static const int WINDOW_WIDTH = 800; // 1600
    static const int WINDOW_HEIGHT = 800; // 900
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OM3D", nullptr, nullptr);
    glfw_check(window);
    DEFER(glfwDestroyWindow(window));

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    init_graphics();

    ImGuiRenderer imgui(window);

    // scene = create_default_scene();

    auto clear_draw_tex_program = Program::from_file("clear_draw_tex.comp");

    auto flatland_draw_program = Program::from_file("flatland_draw.comp");
    auto flatland_jfa_seed_program = Program::from_file("flatland_jfa_seed.comp");
    auto flatland_jfa_program = Program::from_file("flatland_jfa.comp");
    auto flatland_jfa_dist_program = Program::from_file("flatland_jfa_dist.comp");
    auto flatland_raymarch_program = Program::from_file("flatland_raymarch.comp");
    auto flatland_render_program = Program::from_files("flatland_render.frag", "screen.vert");
    RendererState renderer;

    glm::dvec2 mouse_pos;
    bool is_drawing;
    
    for(;;) {
        glfwPollEvents();
        if(glfwWindowShouldClose(window) || glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            break;
        }

        process_profile_markers();

        {
            int width = 0;
            int height = 0;
            glfwGetWindowSize(window, &width, &height);

            if(renderer.size != glm::uvec2(width, height)) {
                renderer = RendererState::create(glm::uvec2(width, height));
            }
        }

        update_delta_time();

        // if(const auto& io = ImGui::GetIO(); !io.WantCaptureMouse && !io.WantCaptureKeyboard) {
        //     process_inputs(window, scene->camera());
        // }

        if(const auto& io = ImGui::GetIO(); !io.WantCaptureMouse && !io.WantCaptureKeyboard) { // TODO remove keyboard ?
            process_inputs_flatland(window, mouse_pos, is_drawing);
        }

        // Draw everything
        {
            PROFILE_GPU("Frame");

            // Clear screen (only draw texture)
            if (flatland_clear_screen) {
                // renderer.flatland_framebuffer.bind(false, true); // trick to clear screen, doesnt work anymore

                clear_draw_tex_program->bind();

                renderer.flatland_draw_texture.bind_as_image(0, OM3D::AccessType::WriteOnly);

                int nb_groups_x = (WINDOW_WIDTH + 16 - 1) / 16;
                int nb_groups_y = (WINDOW_HEIGHT + 16 - 1) / 16;
                glDispatchCompute(nb_groups_x, nb_groups_y, 1);TEST_OPENGL_ERROR();

                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);TEST_OPENGL_ERROR();
            }

            // Flatland drawing
            {
                PROFILE_GPU("Flatland drawing");

                flatland_draw_program->bind();

                flatland_draw_program->set_uniform<u32>("is_drawing", is_drawing); // set bool as u32
                flatland_draw_program->set_uniform<glm::vec2>("prev_mouse_pos", glm::vec2(prev_mouse_pos.x, WINDOW_HEIGHT - prev_mouse_pos.y));
                flatland_draw_program->set_uniform<glm::vec2>("mouse_pos", glm::vec2(mouse_pos.x, WINDOW_HEIGHT - mouse_pos.y));
                flatland_draw_program->set_uniform<glm::vec3>("line_color", glm::vec3(
                    flatland_drawing_color[0], flatland_drawing_color[1], flatland_drawing_color[2]
                ));
                flatland_draw_program->set_uniform<float>("line_width", static_cast<float>(flatland_line_width));

                renderer.flatland_draw_texture.bind_as_image(0, OM3D::AccessType::ReadWrite);

                int nb_groups_x = (WINDOW_WIDTH + 16 - 1) / 16;
                int nb_groups_y = (WINDOW_HEIGHT + 16 - 1) / 16;
                glDispatchCompute(nb_groups_x, nb_groups_y, 1);TEST_OPENGL_ERROR();

                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);TEST_OPENGL_ERROR();
            }

            // Flatland JFA seed
            {
                PROFILE_GPU("Flatland JFA seed");

                flatland_jfa_seed_program->bind();

                renderer.flatland_draw_texture.bind_as_image(0, OM3D::AccessType::ReadOnly);
                renderer.flatland_jfa_B_texture.bind_as_image(1, OM3D::AccessType::WriteOnly);

                int nb_groups_x = (WINDOW_WIDTH + 16 - 1) / 16;
                int nb_groups_y = (WINDOW_HEIGHT + 16 - 1) / 16;
                glDispatchCompute(nb_groups_x, nb_groups_y, 1);TEST_OPENGL_ERROR();

                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);TEST_OPENGL_ERROR();
            }

            // Flatland JFA
            {
                PROFILE_GPU("Flatland JFA");

                flatland_jfa_program->bind();

                int jfa_passes = std::ceil(std::log2(std::max(WINDOW_WIDTH, WINDOW_HEIGHT)));
                // only do odd number (>=3) of ping pongs so that jfa B is the output
                jfa_passes = (jfa_passes % 2 == 0) ? jfa_passes + 1 : jfa_passes;
                jfa_passes += 2; // tmp fix for non square screen ?

                for (int i = 1; i < jfa_passes; i++) {
                    if (i % 2 == 0) {
                        renderer.flatland_jfa_A_texture.bind_as_image(0, OM3D::AccessType::ReadOnly);
                        renderer.flatland_jfa_B_texture.bind_as_image(1, OM3D::AccessType::WriteOnly);
                    } else {
                        renderer.flatland_jfa_B_texture.bind_as_image(0, OM3D::AccessType::ReadOnly);
                        renderer.flatland_jfa_A_texture.bind_as_image(1, OM3D::AccessType::WriteOnly);
                    }

                    flatland_jfa_program->set_uniform<u32>("offset", static_cast<u32>(std::pow(2, jfa_passes - i - 1)));

                    int nb_groups_x = (WINDOW_WIDTH + 16 - 1) / 16;
                    int nb_groups_y = (WINDOW_HEIGHT + 16 - 1) / 16;
                    glDispatchCompute(nb_groups_x, nb_groups_y, 1);TEST_OPENGL_ERROR();

                    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);TEST_OPENGL_ERROR();
                }
            }

            // Flatland JFA to dist
            {
                PROFILE_GPU("Flatland JFA to dist");

                flatland_jfa_dist_program->bind();

                renderer.flatland_jfa_B_texture.bind_as_image(0, OM3D::AccessType::ReadOnly);
                renderer.flatland_jfa_dist_texture.bind_as_image(1, OM3D::AccessType::WriteOnly);

                int nb_groups_x = (WINDOW_WIDTH + 16 - 1) / 16;
                int nb_groups_y = (WINDOW_HEIGHT + 16 - 1) / 16;
                glDispatchCompute(nb_groups_x, nb_groups_y, 1);TEST_OPENGL_ERROR();

                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);TEST_OPENGL_ERROR();
            }

            // Flatland RC
            int rc_iter = 0;
            {
                PROFILE_GPU("Flatland RC");

                glDisable(GL_CULL_FACE); // Dont apply backface culling to fullscreen triangle

                flatland_raymarch_program->bind();
                
                flatland_raymarch_program->set_uniform<glm::vec2>("resolution", glm::vec2(WINDOW_WIDTH, WINDOW_HEIGHT));
                flatland_raymarch_program->set_uniform<float>("base", static_cast<float>(rc_base));

                rc_cascade_count = static_cast<int>(
                    std::ceil(std::log(std::min(WINDOW_WIDTH, WINDOW_HEIGHT)) / std::log(rc_base))
                ) + 1; // + 1 ugly fix
                flatland_raymarch_program->set_uniform<float>("cascade_count", static_cast<float>(rc_cascade_count));

                for (int i = rc_cascade_count; i >= rc_cascade_index; i--) {
                    flatland_raymarch_program->set_uniform<float>("cascade_index", static_cast<float>(i));
                    flatland_raymarch_program->set_uniform<u32>("last_index", static_cast<u32>(i == rc_cascade_index)); // bool

                    renderer.flatland_jfa_dist_texture.bind_as_image(0, OM3D::AccessType::ReadOnly);
                    renderer.flatland_draw_texture.bind_as_image(1, OM3D::AccessType::ReadOnly);
                    if (rc_iter % 2 == 0) {
                        renderer.flatland_scene_A_texture.bind_as_image(2, OM3D::AccessType::ReadOnly);
                        // renderer.flatland_scene_A_texture.bind(2);
                        renderer.flatland_scene_B_texture.bind_as_image(3, OM3D::AccessType::WriteOnly);
                    } else {
                        renderer.flatland_scene_B_texture.bind_as_image(2, OM3D::AccessType::ReadOnly);
                        // renderer.flatland_scene_B_texture.bind(2);
                        renderer.flatland_scene_A_texture.bind_as_image(3, OM3D::AccessType::WriteOnly);
                    }

                    int nb_groups_x = (WINDOW_WIDTH + 16 - 1) / 16;
                    int nb_groups_y = (WINDOW_HEIGHT + 16 - 1) / 16;
                    glDispatchCompute(nb_groups_x, nb_groups_y, 1);TEST_OPENGL_ERROR();

                    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);TEST_OPENGL_ERROR();

                    rc_iter++;
                }
            }

            // Render light image to fullscreen triangle
            {
                PROFILE_GPU("Flatland render");

                glDisable(GL_CULL_FACE); // Dont apply backface culling to fullscreen triangle

                renderer.flatland_framebuffer.bind(false, false);

                flatland_render_program->bind();

                if (rc_debug_display == 0) {
                    if (rc_iter % 2 == 0) { // if need this level of performance, "rc_iter & 1" same as "rc_iter % 2 == 1" 
                        renderer.flatland_scene_A_texture.bind(0);
                    } else {
                        renderer.flatland_scene_B_texture.bind(0);
                    }
                } else if (rc_debug_display == 1) {
                    renderer.flatland_jfa_dist_texture.bind(0);
                } else if (rc_debug_display == 2) {
                    renderer.flatland_jfa_B_texture.bind(0);
                } else if (rc_debug_display == 3) {
                    renderer.flatland_draw_texture.bind(0);
                }

                glDrawArrays(GL_TRIANGLES, 0, 3);
            }

            // Blit result to screen
            {
                PROFILE_GPU("Blit");

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                renderer.flatland_framebuffer.blit();
            }

            // Draw GUI on top
            gui(imgui);
        }

        glfwSwapBuffers(window);
    }

    // scene = nullptr; // destroy scene and child OpenGL objects
}
