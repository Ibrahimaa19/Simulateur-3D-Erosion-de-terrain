#include "Gui.hpp"
#include "Terrain.hpp" 

#include <GL/glew.h> 
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cstdio> 

Gui::Gui() {}

Gui::~Gui() {
    Shutdown();
}

void Gui::Init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130"); 
}

void Gui::Render(Terrain* terrain) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ============================================================
    // CAS 1 : ECRAN D'ACCUEIL
    // ============================================================
    if (showWelcomeScreen) {
        ImGuiIO& io = ImGui::GetIO();
        
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

        ImGui::Begin("Welcome", nullptr, window_flags);

        float windowWidth = ImGui::GetWindowSize().x;
        float windowHeight = ImGui::GetWindowSize().y;
        
        ImGui::SetCursorPosY(windowHeight * 0.3f); 
        ImGui::SetWindowFontScale(3.0f); 
        const char* title = "SIMULATEUR D'EROSION 3D";
        float textWidth = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "%s", title);
        
        ImGui::SetWindowFontScale(1.5f); 
        const char* subtitle = "Projet M1 CHPC - Universite de Versailles";
        textWidth = ImGui::CalcTextSize(subtitle).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::Text("%s", subtitle);

        ImGui::SetCursorPosY(windowHeight * 0.6f); 
        const char* btnText = "LANCER LA SIMULATION";
        float btnWidth = 300.0f;
        float btnHeight = 60.0f;
        ImGui::SetCursorPosX((windowWidth - btnWidth) * 0.5f);
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
        
        if (ImGui::Button(btnText, ImVec2(btnWidth, btnHeight))) {
            showWelcomeScreen = false; 
        }
        
        ImGui::PopStyleColor(2); 
        ImGui::SetWindowFontScale(1.0f); 

        ImGui::End();
    }
    // ============================================================
    // CAS 2 : INTERFACE DE SIMULATION
    // ============================================================
    else {
        ImGui::SetNextWindowSize(ImVec2(380, 400), ImGuiCond_FirstUseEver); 
        ImGui::Begin("Controle Simulation", nullptr);

        if (ImGui::BeginTabBar("MyTabBar"))
        {
            if (ImGui::BeginTabItem("Simulation"))
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "--- Controle Temporel ---");
                
                if (ImGui::Button(isPaused ? "Reprendre" : "Pause", ImVec2(100, 30))) {
                    isPaused = !isPaused;
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset", ImVec2(100, 30))) {
                    printf("Reset du terrain demande !\n");
                }

                ImGui::DragFloat("Vitesse", &timeSpeed, 0.1f, 0.0f, 10.0f);

                ImGui::Separator();

                if (ImGui::CollapsingHeader("Erosion Thermique", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::InputInt("Iterations##Therm", &thermalIterations, 1000, 5000); 
                    ImGui::SliderFloat("Angle Talus", &talusAngle, 0.0f, 90.0f, "%.1f deg");
                    ImGui::SliderFloat("Constante K", &thermalK, 0.0f, 1.0f);
                }

                if (ImGui::CollapsingHeader("Erosion Hydraulique"))
                {
                    ImGui::InputInt("Iterations##Hydro", &hydroIterations, 1000, 5000);
                    ImGui::SliderFloat("Pluie", &rainAmount, 0.0f, 5.0f);
                    ImGui::SliderFloat("Evaporation", &evaporationRate, 0.0f, 1.0f);
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Visuel"))
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "--- Parametres Rendu ---");
                
                ImGui::DragFloat("Echelle Verticale", &verticalScale, 0.1f, 0.1f, 10.0f);
                ImGui::ColorEdit3("Couleur Sol", terrainColor);
                
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Infos"))
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "====== CAMERA CONTROL MENU ======");
                
                ImGui::Text("Keyboard :");
                ImGui::BulletText("W : Move forward");
                ImGui::BulletText("S : Move backward");
                ImGui::BulletText("A : Move left");
                ImGui::BulletText("D : Move right");
                ImGui::BulletText("Q : Move up");
                ImGui::BulletText("E : Move down");
                ImGui::BulletText("M : Toggle Mouse/Menu");
                ImGui::BulletText("ESC : Quit");
                ImGui::BulletText("H : Show this menu"); 

                ImGui::Spacing();
                ImGui::Text("Mouse :");
                ImGui::BulletText("Move mouse : Rotate camera");
                ImGui::BulletText("Scroll wheel : Zoom in/out");
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextDisabled("=================================");

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Gui::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}