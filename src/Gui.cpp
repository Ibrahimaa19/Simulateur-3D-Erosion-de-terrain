#include "Gui.hpp"
#include "Terrain.hpp" 

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

    ImGuiIO& io = ImGui::GetIO();

    // ============================================================
    // ÉTAPE 1 : ECRAN D'ACCUEIL 
    // ============================================================
    if (showWelcomeScreen) {
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
        const char* subtitle = "Projet M1 CHPS";
        textWidth = ImGui::CalcTextSize(subtitle).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::Text("%s", subtitle);

        ImGui::SetCursorPosY(windowHeight * 0.6f); 
        const char* btnText = "COMMENCER";
        float btnWidth = 300.0f;
        float btnHeight = 60.0f;
        ImGui::SetCursorPosX((windowWidth - btnWidth) * 0.5f);
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
        
        if (ImGui::Button(btnText, ImVec2(btnWidth, btnHeight))) {
            showWelcomeScreen = false; 
            showConfigScreen = true;
        }
        
        ImGui::PopStyleColor(2); 
        ImGui::SetWindowFontScale(1.0f); 
        ImGui::End();
    }
    // ============================================================
    // ÉTAPE 2 : MENU DE CONFIGURATION
    // ============================================================
    else if (showConfigScreen) {
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(500, 550)); 
        
        ImGui::Begin("Configuration du Terrain", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "1. Choisissez une methode de generation :");
        
        const char* items[] = { "Image (Heightmap)", "Faille (Fault Formation)", "Deplacement (Midpoint)", "Perlin Noise" };
        ImGui::Combo("##Method", &selectedMethod, items, IM_ARRAYSIZE(items));
        
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "2. Parametres :");
        ImGui::Spacing();

        if (selectedMethod == GEN_HEIGHTMAP) {
            ImGui::Text("Chargement depuis un fichier image PNG.");
            const char* imgItems[] = { "iceland_heightmap.png", "heightmap.png" };
            ImGui::Combo("Fichier Source", &selectedImage, imgItems, 2);
        }
        else if (selectedMethod == GEN_FAULT_FORMATION) {
            ImGui::Text("Generation procedurale par failles.");
            ImGui::InputInt("Largeur (X)", &faultWidth);
            ImGui::InputInt("Hauteur (Z)", &faultHeight);
            ImGui::InputInt("Iterations", &faultIterations, 10, 100);
            ImGui::DragFloatRange2("Hauteur Min/Max", &faultMinHeight, &faultMaxHeight, 1.0f, 0.0f, 500.0f);
            
            ImGui::Checkbox("Appliquer Filtre Lissage", &faultUseFilter);
            if(faultUseFilter) {
                ImGui::SliderFloat("Intensite Filtre", &faultFilter, 0.0f, 1.0f);
            }
        }
        else if (selectedMethod == GEN_MIDPOINT_DISPLACEMENT) {
            ImGui::Text("Generation fractale (Diamant-Carre).");
            ImGui::InputInt("Taille (2^n + 1)", &midpointSize);
            HelpMarker("La taille doit etre une puissance de 2 plus 1 (ex: 129, 257, 513, 1025).");
            
            ImGui::SliderFloat("Rugosite (Roughness)", &midpointRoughness, 0.0f, 2.0f);
            ImGui::DragFloatRange2("Hauteur Min/Max", &midpointMinHeight, &midpointMaxHeight, 1.0f, 0.0f, 500.0f);
        }
        else if (selectedMethod == GEN_PERLIN_NOISE) {
            ImGui::Text("Generation procedurale par Bruit de Perlin.");
            ImGui::InputInt("Largeur (X)", &perlinWidth);
            ImGui::InputInt("Hauteur (Z)", &perlinHeight);
            ImGui::DragFloatRange2("Hauteur Min/Max", &perlinMinHeight, &perlinMaxHeight, 1.0f, 0.0f, 500.0f);
            ImGui::Separator();
            ImGui::Text("Reglages du Bruit :");
            ImGui::SliderFloat("Frequence", &perlinFrequency, 0.001f, 0.1f, "%.4f");
            ImGui::SliderInt("Octaves", &perlinOctaves, 1, 10);
            ImGui::SliderFloat("Persistance", &perlinPersistence, 0.1f, 1.5f);
            ImGui::SliderFloat("Lacunarite", &perlinLacunarity, 1.0f, 5.0f);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float windowWidth = ImGui::GetWindowSize().x;
        ImGui::SetCursorPosX((windowWidth - 200) * 0.5f);
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.6f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.7f, 0.9f, 1.0f));

        if (ImGui::Button("GENERER LE TERRAIN", ImVec2(200, 50))) {
            startGeneration = true; 
            showConfigScreen = false;
        }
        ImGui::PopStyleColor(2);

        ImGui::End();
    }
    // ============================================================
    // ÉTAPE 3 : INTERFACE DE SIMULATION 
    // ============================================================
    else {
        ImGui::SetNextWindowSize(ImVec2(380, 500), ImGuiCond_FirstUseEver); 
        ImGui::Begin("Controle Simulation", nullptr);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("<- Retour au Menu (Reset)", ImVec2(-1, 30))) {
             resetSimulation = true; 
             showConfigScreen = true; 
        }
        ImGui::PopStyleColor();
        ImGui::Separator();

        if (ImGui::BeginTabBar("MyTabBar"))
        {
            // ONGLET SIMULATION
            if (ImGui::BeginTabItem("Simulation"))
            {
                ImGui::Spacing();
                
                if (ImGui::CollapsingHeader("Erosion Thermique", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::SliderFloat("Angle Talus", &talusAngle, 0.0f, 90.0f, "%.1f deg");
                    ImGui::SliderFloat("Constante K", &thermalK, 0.0f, 1.0f);

                    ImGui::Spacing();
                    
                    if (thermalRunning) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.4f, 1.0f));
                        if (ImGui::Button("PAUSE ##Thermal", ImVec2(100, 30))) {
                            thermalRunning = false;
                        }
                        ImGui::PopStyleColor();
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
                        if (ImGui::Button("LANCER ##Thermal", ImVec2(100, 30))) {
                            thermalRunning = true;
                        }
                        ImGui::PopStyleColor();
                    }

                    ImGui::SameLine();
                    ImGui::Text("Step: %d", thermalCurrentStep);
                }

                if (ImGui::CollapsingHeader("Erosion Hydraulique"))
                {
                    ImGui::InputInt("Iterations##Hydro", &hydroIterations, 1000, 5000);
                    ImGui::SliderFloat("Pluie", &rainAmount, 0.0f, 5.0f);
                    ImGui::SliderFloat("Evaporation", &evaporationRate, 0.0f, 1.0f);
                }

                ImGui::EndTabItem();
            }


            // ONGLET INFOS 
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

                ImGui::Spacing();
                ImGui::Text("Mouse :");
                ImGui::BulletText("Move mouse : Rotate camera");
                ImGui::BulletText("Scroll wheel : Zoom in/out");
                
                ImGui::Separator();
                if (terrain != nullptr) {
                     ImGui::Text("Taille Terrain: %d x %d", terrain->get_terrain_width(), terrain->get_terrain_height());
                }
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