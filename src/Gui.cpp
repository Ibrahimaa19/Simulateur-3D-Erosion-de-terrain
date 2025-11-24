#include "Gui.hpp"
#include "terrain.hpp" 

#include <GL/glew.h> 

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cstdio> 

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

// Constructeur
Gui::Gui() {
}

// Destructeur
Gui::~Gui() {
    Shutdown();
}

void Gui::Init(GLFWwindow* window) {
    // 1. Initialisation du contexte ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // 2. Configuration du style 
    ImGui::StyleColorsDark();
    
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    
    // 3. Initialisation des Backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130"); 
}

void Gui::Render(Terrain* terrain) {
    // --- Début de la frame ImGui ---
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // --- Configuration de la fenêtre principale ---
    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver); 
    
    ImGui::Begin("Controle Simulation", nullptr);

    // Affichage des FPS en haut
    ImGui::Text("FPS moyen : %.1f", ImGui::GetIO().Framerate);
    ImGui::Separator();

    // --- DÉBUT DES ONGLETS ---
    if (ImGui::BeginTabBar("MyTabBar"))
    {
        // ============================================================
        // ONGLET 1 : SIMULATION
        // ============================================================
        if (ImGui::BeginTabItem("Simulation"))
        {
            ImGui::Spacing();
            
            // --- Contrôle du Temps ---
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "--- Controle Temporel ---");
            
            // Bouton Pause / Reprendre
            if (ImGui::Button(isPaused ? "Reprendre" : "Pause", ImVec2(100, 30))) {
                isPaused = !isPaused;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset", ImVec2(100, 30))) {

                printf("Reset du terrain demande !\n");
            }

            ImGui::DragFloat("Vitesse", &timeSpeed, 0.1f, 0.0f, 10.0f);
            HelpMarker("Multiplicateur de temps pour la simulation.");

            ImGui::Separator();

            // --- Paramètres d'Érosion ---
            if (ImGui::CollapsingHeader("Erosion Thermique", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SliderInt("Iterations", &thermalIterations, 0, 50000);
                ImGui::SliderFloat("Angle Talus", &talusAngle, 0.0f, 90.0f, "%.1f deg");
                ImGui::SliderFloat("Constante K", &thermalK, 0.0f, 1.0f);
            }

            if (ImGui::CollapsingHeader("Erosion Hydraulique"))
            {
                ImGui::SliderInt("Iterations", &hydroIterations, 0, 100000);
                ImGui::SliderFloat("Pluie", &rainAmount, 0.0f, 5.0f);
                ImGui::SliderFloat("Evaporation", &evaporationRate, 0.0f, 1.0f);
            }

            ImGui::EndTabItem();
        }

        // ============================================================
        // ONGLET 2 : VISUEL
        // ============================================================
        if (ImGui::BeginTabItem("Visuel"))
        {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "--- Parametres Rendu ---");

            // --- Gestion du Wireframe ---
            if (ImGui::Checkbox("Mode Fil de fer (Wireframe)", &showWireframe)) {
                if(showWireframe) 
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                else 
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }

            ImGui::Separator();

            ImGui::DragFloat("Echelle Verticale", &verticalScale, 0.1f, 0.1f, 10.0f);
            HelpMarker("Permet d'exagerer le relief pour mieux voir les details.");

            ImGui::ColorEdit3("Couleur Sol", terrainColor);
            
            ImGui::Spacing();
            ImGui::Text("Position Soleil :");
            ImGui::DragFloat3("##SunPos", sunPosition, 1.0f, -100.0f, 100.0f);

            ImGui::EndTabItem();
        }

        // ============================================================
        // ONGLET 3 : INFOS & DEBUG
        // ============================================================
        if (ImGui::BeginTabItem("Infos"))
        {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "--- Stats Terrain ---");
            
            if (terrain != nullptr) {
            
                ImGui::Text("Nombre de Triangles : %zu", terrain->indices.size() / 3);
                ImGui::Text("Nombre de Sommets   : %zu", terrain->vertices.size());
            } else {
                ImGui::TextColored(ImVec4(1,0,0,1), "Terrain non connecte !");
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "--- Camera ---");
            
            // Affiche la position de la caméra 
            ImGui::Text("X : %.2f", cameraPos.x);
            ImGui::Text("Y : %.2f", cameraPos.y);
            ImGui::Text("Z : %.2f", cameraPos.z);
            
            ImGui::Separator();
            ImGui::TextDisabled("Simulateur-3D-Erosion-de-terrain - M1 CHPS");

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Gui::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}