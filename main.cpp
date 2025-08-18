// Main Wind Tunnel Program
//   This main file was built upon ImGui's SDL3+OpenGL example to render the GUI.

#include <iostream>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <chrono>
#include <future>

#include "serial/serial.h" // serial library

#include "implot.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL3/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

// CONSTANT VARIABLES
// Angle Variables
const int16_t MIN_ANGLE = -15;
const int16_t MAX_ANGLE = 15;
const int16_t BASE_ANGLE = 90;

// Calibration Variables
const float BASE_CALIBRATION_V = 714.0f;
const float BASE_CALIBRATION_H = 26.5f;

// Serial Variables
const unsigned long BAUD = 115200;
static std::string PORT = "/dev/cu.usbmodem11401"; // port for arduino

// OTHER STATIC VARIABLES
static int16_t angle = BASE_ANGLE;
static int16_t angle_rel = 0;
static float calibration_v = BASE_CALIBRATION_V;
static float calibration_h = BASE_CALIBRATION_H;
static bool rt_angle = true;
static bool rt_graph = false;
static bool rt_calibration = true;
static bool serial_open = false;
static bool flag_clean_futures = true;
static std::string serial_buffer;
static std::string serial_result;
static std::string readings;
static float reading_v = 0.0f;
static float reading_h = 0.0f;
static serial::Serial serial_port(PORT, BAUD, serial::Timeout::simpleTimeout(1000));
static std::mutex serial_buffer_mutex;
static std::mutex serial_result_mutex;
static float readings_per_second = 5.0f; 
static double time_since_last_reading = 0.0f;
static std::vector<std::future<void>> futures;

// utility structure for realtime plot (taken from implot_demo.cpp)
struct ScrollingBuffer {
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;
    ScrollingBuffer(int max_size = 2000) {
        MaxSize = max_size;
        Offset  = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y) {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x,y));
        else {
            Data[Offset] = ImVec2(x,y);
            Offset =  (Offset + 1) % MaxSize;
        }
    }
    void Erase() {
        if (Data.size() > 0) {
            Data.shrink(0);
            Offset  = 0;
        }
    }
};

void update_angle() {
    angle = BASE_ANGLE + angle_rel;
}

std::vector<std::string> gather_ports () {
    std::vector<serial::PortInfo> ports = serial::list_ports();
    std::vector<std::string> result;
    for (const auto& port : ports) {
        result.push_back(port.port);
    }
    return result;
}

void enumerate_ports(std::vector<std::string> devices) {
    std::cout << "Listing all available devices: " << std::endl;
    for (const auto& device : devices) {
        std::cout << device << std::endl;
    }
}

void clean_futures (std::vector<std::future<void>> &futures) {
    while (!futures.empty()) {
        for (auto it = futures.begin(); it != futures.end(); /* no increment */) {
            auto status = it->wait_for(std::chrono::seconds(0));
            if (status == std::future_status::ready) {
                it = futures.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void read_from_serial () {
    {
        std::lock_guard<std::mutex> lock(serial_buffer_mutex);
        serial_buffer = "read:";
        serial_port.write(serial_buffer);
    }
   
    std::lock_guard<std::mutex> result_lock(serial_result_mutex);
    serial_result = serial_port.readline(100, "\n");
    std::cout << "Received: " << serial_result;

    reading_v = std::stof(serial_result.substr(0, serial_result.find(':')));
    reading_h = std::stof(serial_result.substr(serial_result.find(':') + 1, serial_result.find('\n')));
}

void update_to_serial () {
    update_angle();
    std::lock_guard<std::mutex> lock(serial_buffer_mutex);
    serial_buffer = "angle:" + std::to_string(angle) 
                  + "calibration_v:" + std::to_string(calibration_v)
                  + "calibration_h:" + std::to_string(calibration_h);
    serial_port.write(serial_buffer);
    std::cout << "Sent: " << serial_buffer << std::endl;
}

// Main code
int main(int, char**)
{
    std::vector<std::string> port_names = gather_ports();

    // Introduction from console
    std::cout << "Starting Wind Tunnel Program V2..." << std::endl;
    enumerate_ports(port_names);

    // Attempts to open the serial port 
    try {
        if (serial_port.isOpen()) {
            std::cout << "Serial port is open on port " + PORT + ", and listening on baud rate of " + std::to_string(BAUD) << std::endl;
            serial_open = true;
        } else {
            std::cout << "Serial port did not open. Here is a list of ports that you could choose from:" << std::endl;
        }
    } catch (const serial::IOException& e) {
        std::cerr << "Error: " << e.what()  << std::endl;
    }

    // Setup SDL
    // [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("Wind Tunnel Dashboard", (int)(1280 * main_scale), (int)(720 * main_scale), window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
    io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_debug_window = true;
    bool show_graph_window = true;
    bool show_main_window = true;

    if (!serial_open) {
        show_main_window = false;
    }
    
    // bool show_another_window = false;
    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        // [If using SDL_MAIN_USE_CALLBACKS: call ImGui_ImplSDL3_ProcessEvent() from your SDL_AppEvent() function]
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppIterate() function]
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // Debugging Window (in case something goes wrong during initialization)
        if (show_debug_window) {
            ImGui::Begin("Debug");
            ImGui::Text("Wind Tunnel Debugging Menu");

            // Device Port Popup
            static int selected_device = -1;
            if (ImGui::Button("List Available Ports"))
                ImGui::OpenPopup("start_devices_popup");
            ImGui::SameLine();
            ImGui::TextUnformatted(selected_device == -1 ? "<None>" : port_names[selected_device].c_str());
            if (ImGui::BeginPopup("start_devices_popup")) {
                ImGui::SeparatorText("Device Ports");
                for (size_t i = 0; i < port_names.size(); i++) {
                    if (ImGui::Selectable(port_names[i].c_str())) {
                        selected_device = i;
                        PORT = port_names[selected_device].c_str();
                        std::cout << selected_device << " " << PORT << std::endl;
                    }
                }
                ImGui::EndPopup();
            }

            if (!serial_open) {
                ImGui::TextWrapped("If you are seeing this message, the serial port did not open correctly. "
                                   "Please see if the desired port is available, or select the correct port using the dropdown above.");
            }

            if (ImGui::Button("Start")) {
                // TODO: sets the serial port and baud rate, then tries to reopen the port.
            }

            if (flag_clean_futures) {
                clean_futures(futures);
            }

            ImGui::BulletText("Remaining async operations: ");
            ImGui::SameLine();
            ImGui::Text("%d", static_cast<int>(futures.size()));  
            ImGui::Checkbox("Clean futures", &flag_clean_futures);

            ImGui::End();
        }

        // Wind Tunnel Window
        if (show_main_window) {
            ImGui::Begin ("Variables");
            
            // Displays the framerate
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

            // Variable Sliders
            if (ImGui::SliderScalar("Angle of Attack", ImGuiDataType_S16, &angle_rel, &MIN_ANGLE, &MAX_ANGLE) && rt_angle) {
                update_angle();
                std::lock_guard<std::mutex> lock(serial_buffer_mutex);
                serial_buffer = "angle:" + std::to_string(angle);
                serial_port.write(serial_buffer);
                std::cout << "Sent: " << serial_buffer << std::endl;
            }

            if (ImGui::InputFloat("Vertical Calibration", &calibration_v, 1.0f, 1.0f, "%.3f") && rt_calibration) {
                std::lock_guard<std::mutex> lock(serial_buffer_mutex);
                serial_buffer = "calibration_v:" + std::to_string(calibration_v);
                serial_port.write(serial_buffer);
                std::cout << "Sent: " << serial_buffer << std::endl;
            }

            if (ImGui::InputFloat("Horizontal Calibration", &calibration_h, 1.0f, 1.0f, "%.3f") && rt_calibration) {
                std::lock_guard<std::mutex> lock(serial_buffer_mutex);
                serial_buffer = "calibration_h:" + std::to_string(calibration_h);
                serial_port.write(serial_buffer);
                std::cout << "Sent: " << serial_buffer << std::endl;
            }

            // Sends the variables to be updated to the Arduino program via the Serial port.
            if (ImGui::Button("Update to Serial")) {
                update_to_serial();
            }

            ImGui::Checkbox("Update angle in real time", &rt_angle);
            ImGui::Checkbox("Update calibration in real time", &rt_calibration);

            // Saves all variables to a .txt config file
            if (ImGui::Button("Save")) {
                // TODO: save a config or something
            }

            ImGui::End();
        }

        // IMPLOT GRAPH
        static ScrollingBuffer angle_data, vertical_data, horizontal_data;
        static float t = 0;
        t += ImGui::GetIO().DeltaTime;

        // Limits readings depending on the rate
        if (rt_graph) {
            if (time_since_last_reading > (1.0f / readings_per_second)) {
                futures.push_back(std::async(std::launch::async, read_from_serial));
                time_since_last_reading = 0.0f;
            } else {
                time_since_last_reading += ImGui::GetIO().DeltaTime;
            }
        } 

        angle_data.AddPoint(t, angle_rel);
        vertical_data.AddPoint(t, reading_v);
        horizontal_data.AddPoint(t, reading_h);

        static float history = 30.0f;

        static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;

        if (show_graph_window) {
            // Reading from serial
            ImPlot::BeginPlot("Load Cell Forces", ImVec2(-1, 150));
            ImPlot::SetupAxes(nullptr, nullptr, flags, flags);
            ImPlot::SetupAxisLimits(ImAxis_X1, t - history, t, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -20, 20);
            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
            ImPlot::PlotLine("Angle of Attack", &angle_data.Data[0].x, &angle_data.Data[0].y, angle_data.Data.size(),  0, angle_data.Offset, 2*sizeof(float));
            ImPlot::PlotLine("Vertical Force", &vertical_data.Data[0].x, &vertical_data.Data[0].y, vertical_data.Data.size(),  0, vertical_data.Offset, 2*sizeof(float));
            ImPlot::PlotLine("Horizontal Force", &horizontal_data.Data[0].x, &horizontal_data.Data[0].y, horizontal_data.Data.size(), 0, horizontal_data.Offset, 2*sizeof(float));
            ImPlot::EndPlot();

            // RPS slider
            ImGui::SliderFloat("Readings per Second", &readings_per_second, 1.0f, 10.0f);

            // Lists readings in plaintext
            ImGui::Text("V: %f", reading_v);
            ImGui::SameLine();
            ImGui::Text("H: %f", reading_h);
        

            ImGui::Checkbox("Enable readings", &rt_graph);
            if (ImGui::Button("Tare Scale")) {
                std::lock_guard<std::mutex> lock(serial_buffer_mutex);
                serial_buffer = "tare:";
                serial_port.write(serial_buffer);
                std::cout << "Sent: " << serial_buffer << std::endl;
            }
            if (ImGui::Button("Tare Vertical Scale")) {
                std::lock_guard<std::mutex> lock(serial_buffer_mutex);
                serial_buffer = "tare_v:";
                serial_port.write(serial_buffer);
                std::cout << "Sent: " << serial_buffer << std::endl;
            }
            ImGui::SameLine();
            if (ImGui::Button("Tare Horizontal Scale")) {
                std::lock_guard<std::mutex> lock(serial_buffer_mutex);
                serial_buffer = "tare_h:";
                serial_port.write(serial_buffer);
                std::cout << "Sent: " << serial_buffer << std::endl;
            }
        }
        
        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }

        SDL_GL_SwapWindow(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup

    // cleans futures
    for (auto& future : futures) {
        future.get();
    }

    // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
