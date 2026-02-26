#include "Gui.hpp"
#include "Terrain.hpp"

#include <GL/glew.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cstdio>

namespace {

void ApplyModernStyle()
{
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4* colors = s.Colors;

    // Palette : thème sombre avec accents cyan/orange
    const ImVec4 bgDark      = ImVec4(0.08f, 0.09f, 0.12f, 0.94f);
    const ImVec4 bgMedium    = ImVec4(0.12f, 0.14f, 0.18f, 1.00f);
    const ImVec4 bgLight     = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
    const ImVec4 accent      = ImVec4(0.00f, 0.75f, 0.85f, 1.00f);
    const ImVec4 accentHover = ImVec4(0.20f, 0.85f, 0.92f, 1.00f);
    const ImVec4 success     = ImVec4(0.20f, 0.72f, 0.45f, 1.00f);
    const ImVec4 successHover= ImVec4(0.30f, 0.82f, 0.55f, 1.00f);
    const ImVec4 warning     = ImVec4(1.00f, 0.55f, 0.20f, 1.00f);
    const ImVec4 text        = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    const ImVec4 textDim     = ImVec4(0.60f, 0.62f, 0.68f, 1.00f);

    colors[ImGuiCol_WindowBg]         = bgDark;
    colors[ImGuiCol_ChildBg]          = ImVec4(0.10f, 0.11f, 0.14f, 0.50f);
    colors[ImGuiCol_PopupBg]          = bgDark;
    colors[ImGuiCol_Border]           = ImVec4(0.25f, 0.28f, 0.35f, 0.50f);
    colors[ImGuiCol_FrameBg]          = bgMedium;
    colors[ImGuiCol_FrameBgHovered]   = bgLight;
    colors[ImGuiCol_FrameBgActive]    = ImVec4(0.20f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBg]          = bgMedium;
    colors[ImGuiCol_TitleBgActive]    = ImVec4(0.15f, 0.17f, 0.22f, 1.00f);
    colors[ImGuiCol_MenuBarBg]        = bgMedium;
    colors[ImGuiCol_Header]           = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    colors[ImGuiCol_HeaderHovered]    = ImVec4(accent.x, accent.y, accent.z, 0.50f);
    colors[ImGuiCol_HeaderActive]     = ImVec4(accent.x, accent.y, accent.z, 0.70f);
    colors[ImGuiCol_Button]           = accent;
    colors[ImGuiCol_ButtonHovered]    = accentHover;
    colors[ImGuiCol_ButtonActive]     = ImVec4(accent.x * 0.8f, accent.y * 0.8f, accent.z * 0.8f, 1.00f);
    colors[ImGuiCol_Text]             = text;
    colors[ImGuiCol_TextDisabled]     = textDim;
    colors[ImGuiCol_CheckMark]        = accent;
    colors[ImGuiCol_SliderGrab]       = accent;
    colors[ImGuiCol_SliderGrabActive] = accentHover;
    colors[ImGuiCol_Tab]              = ImVec4(accent.x, accent.y, accent.z, 0.50f);
    colors[ImGuiCol_TabHovered]       = accent;
    colors[ImGuiCol_TabActive]        = accent;
    colors[ImGuiCol_Separator]        = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = accent;
    colors[ImGuiCol_SeparatorActive]  = accent;

    s.WindowRounding    = 8.0f;
    s.ChildRounding     = 6.0f;
    s.FrameRounding     = 5.0f;
    s.PopupRounding     = 6.0f;
    s.ScrollbarRounding = 4.0f;
    s.GrabRounding      = 4.0f;
    s.TabRounding       = 5.0f;

    s.WindowPadding     = ImVec2(16.0f, 16.0f);
    s.FramePadding      = ImVec2(10.0f, 6.0f);
    s.ItemSpacing       = ImVec2(10.0f, 8.0f);
    s.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
    s.IndentSpacing     = 22.0f;
}

void HelpMarker(const char* desc)
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

} // namespace

Gui::Gui() {}

Gui::~Gui() {
    Shutdown();
}

void Gui::Init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ApplyModernStyle();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

void Gui::Render(Terrain* terrain) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();

    // ============================================================
    // ÉCRAN D'ACCUEIL
    // ============================================================
    if (showWelcomeScreen) {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;

        ImGui::Begin("Welcome", nullptr, flags);

        float w = ImGui::GetWindowSize().x;
        float h = ImGui::GetWindowSize().y;

        ImGui::SetCursorPosY(h * 0.25f);
        ImGui::SetWindowFontScale(2.8f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.82f, 0.92f, 1.0f));
        const char* title = "SIMULATEUR D'ÉROSION 3D";
        ImGui::SetCursorPosX((w - ImGui::CalcTextSize(title).x) * 0.5f);
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();

        ImGui::SetWindowFontScale(1.2f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.68f, 0.72f, 1.0f));
        const char* sub = "Projet M1 CHPS — UVSQ Paris-Saclay";
        ImGui::SetCursorPosY(h * 0.38f);
        ImGui::SetCursorPosX((w - ImGui::CalcTextSize(sub).x) * 0.5f);
        ImGui::Text("%s", sub);
        ImGui::PopStyleColor();

        ImGui::SetCursorPosY(h * 0.58f);
        float btnW = 280.0f;
        float btnH = 56.0f;
        ImGui::SetCursorPosX((w - btnW) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.75f, 0.85f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.88f, 0.95f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.65f, 0.75f, 1.0f));
        if (ImGui::Button("COMMENCER", ImVec2(btnW, btnH))) {
            showWelcomeScreen = false;
            showConfigScreen = true;
        }
        ImGui::PopStyleColor(3);
        ImGui::SetWindowFontScale(1.0f);
        ImGui::End();
    }
    // ============================================================
    // CONFIGURATION DU TERRAIN
    // ============================================================
    else if (showConfigScreen) {
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(520, 600));

        ImGui::Begin("Configuration du Terrain", nullptr, ImGuiWindowFlags_NoCollapse);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.82f, 0.92f, 1.0f));
        ImGui::Text("MÉTHODE DE GÉNÉRATION");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        const char* methods[] = { "Image (Heightmap)", "Faille (Fault Formation)", "Déplacement (Midpoint)", "Perlin Noise" };
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##Method", &selectedMethod, methods, IM_ARRAYSIZE(methods));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.5f, 1.0f));
        ImGui::Text("PARAMÈTRES");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        if (selectedMethod == GEN_HEIGHTMAP) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Chargement depuis une image PNG.");
            const char* imgs[] = { "iceland_heightmap.png", "heightmap.png" };
            ImGui::Combo("Fichier source", &selectedImage, imgs, 2);
        }
        else if (selectedMethod == GEN_FAULT_FORMATION) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Génération procédurale par failles.");
            ImGui::InputInt("Largeur", &faultWidth);
            ImGui::InputInt("Hauteur", &faultHeight);
            ImGui::InputInt("Itérations", &faultIterations, 10, 100);
            ImGui::DragFloatRange2("Hauteur min / max", &faultMinHeight, &faultMaxHeight, 1.0f, 0.0f, 500.0f);
            ImGui::Checkbox("Filtre de lissage", &faultUseFilter);
            if (faultUseFilter) {
                ImGui::SliderFloat("Intensité du filtre", &faultFilter, 0.0f, 1.0f);
            }
        }
        else if (selectedMethod == GEN_MIDPOINT_DISPLACEMENT) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Génération fractale diamond-square.");
            ImGui::InputInt("Taille (2^n + 1)", &midpointSize);
            HelpMarker("Ex: 129, 257, 513, 1025");
            ImGui::SliderFloat("Rugosité", &midpointRoughness, 0.0f, 2.0f);
            ImGui::DragFloatRange2("Hauteur min / max", &midpointMinHeight, &midpointMaxHeight, 1.0f, 0.0f, 500.0f);
        }
        else if (selectedMethod == GEN_PERLIN_NOISE) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Bruit de Perlin procédural.");
            ImGui::InputInt("Largeur", &perlinWidth);
            ImGui::InputInt("Hauteur", &perlinHeight);
            ImGui::DragFloatRange2("Hauteur min / max", &perlinMinHeight, &perlinMaxHeight, 1.0f, 0.0f, 500.0f);
            ImGui::Separator();
            ImGui::Text("Réglages du bruit");
            ImGui::SliderFloat("Fréquence", &perlinFrequency, 0.001f, 0.1f, "%.4f");
            ImGui::SliderInt("Octaves", &perlinOctaves, 1, 10);
            ImGui::SliderFloat("Persistance", &perlinPersistence, 0.1f, 1.5f);
            ImGui::SliderFloat("Lacunarité", &perlinLacunarity, 1.0f, 5.0f);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float winW = ImGui::GetWindowSize().x;
        ImGui::SetCursorPosX((winW - 220) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.75f, 0.85f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.88f, 0.95f, 1.0f));
        if (ImGui::Button("GÉNÉRER LE TERRAIN", ImVec2(220, 48))) {
            startGeneration = true;
            showConfigScreen = false;
        }
        ImGui::PopStyleColor(2);
        ImGui::End();
    }
    // ============================================================
    // INTERFACE DE SIMULATION
    // ============================================================
    else {
        ImGui::SetNextWindowSize(ImVec2(400, 540), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 420, 20), ImGuiCond_FirstUseEver);

        ImGui::Begin("Simulation", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.25f, 0.25f, 1.0f));
        if (ImGui::Button("← Retour au menu", ImVec2(-1, 36))) {
            resetSimulation = true;
            showConfigScreen = true;
        }
        ImGui::PopStyleColor(2);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("SimTabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Érosion"))
            {
                ImGui::Spacing();

                if (ImGui::CollapsingHeader("Érosion thermique", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Glissement gravitaire (angle de talus)");
                    ImGui::Spacing();

                    ImGui::SliderFloat("Angle talus (deg)", &talusAngle, 0.0f, 90.0f, "%.1f");
                    ImGui::SliderFloat("Taux de transfert", &thermalK, 0.0f, 1.0f, "%.2f");

                    ImGui::Spacing();
                    if (thermalRunning) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.35f, 0.35f, 1.0f));
                        if (ImGui::Button("PAUSE", ImVec2(-1, 38))) { thermalRunning = false; }
                        ImGui::PopStyleColor();
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.75f, 0.45f, 0.95f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.85f, 0.55f, 1.0f));
                        if (ImGui::Button("LANCER##Thermal", ImVec2(-1, 38))) { thermalRunning = true; }
                        ImGui::PopStyleColor(2);
                    }

                    ImGui::Spacing();
                    ImGui::Text("Étape : %d", thermalCurrentStep);
                    ImGui::Text("Cellules modifiées : %d", thermalCellsModified);
                }

                ImGui::Spacing();
                if (ImGui::CollapsingHeader("Érosion hydraulique", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Modèle gouttes d'eau (batching)");
                    ImGui::Spacing();

                    ImGui::InputInt("Total gouttes", &hydroIterations, 1000, 10000);
                    ImGui::InputInt("Gouttes par batch", &hydroBatchSize, 100, 1000);
                    HelpMarker("Chaque batch : les gouttes lisent le même terrain, accumulent érosion/dépôt, puis application. Adapté à la parallélisation future.");
                    ImGui::SliderFloat("Pluie", &hydroRain, 0.1f, 5.0f, "%.2f");
                    ImGui::SliderFloat("Taux érosion", &hydroErosionRate, 0.01f, 0.15f, "%.3f");
                    ImGui::SliderFloat("Taux dépôt (à l'arrêt)", &hydroDepositRate, 0.5f, 1.0f, "%.2f");
                    ImGui::SliderFloat("Évaporation", &hydroEvaporation, 0.01f, 0.5f, "%.2f");
                    ImGui::InputInt("Graine aléatoire", &hydroSeed);

                    ImGui::Spacing();
                    if (hydroRunning) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.35f, 0.35f, 1.0f));
                        if (ImGui::Button("ARRÊTER##Hydro", ImVec2(-1, 38))) { hydroRunning = false; }
                        ImGui::PopStyleColor();
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.65f, 0.75f, 0.95f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.8f, 0.88f, 1.0f));
                        if (ImGui::Button("LANCER (érosion hydraulique)##Hydro", ImVec2(-1, 38))) { hydroRunning = true; }
                        ImGui::PopStyleColor(2);
                    }

                    ImGui::Spacing();
                    ImGui::Text("Gouttes traitées : %d", hydroDropletsProcessed);
                    HelpMarker("Lancer pour appliquer en continu. L'effet est visible au fur et à mesure.");
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Infos"))
            {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.85f, 0.5f, 1.0f));
                ImGui::Text("CONTRÔLES CAMÉRA");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                ImGui::BulletText("W A S D : Déplacement");
                ImGui::BulletText("Q / E : Monter / Descendre");
                ImGui::BulletText("Souris : Orienter");
                ImGui::BulletText("Molette : Zoom");
                ImGui::BulletText("M : Afficher / Masquer ce panneau");
                ImGui::BulletText("P : Mode fil de fer");
                ImGui::BulletText("F : Démarrer / Pause érosion thermique");
                ImGui::BulletText("H : Démarrer / Arrêter érosion hydraulique");
                ImGui::BulletText("Échap : Quitter");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                if (terrain != nullptr) {
                    ImGui::Text("Terrain : %d × %d", terrain->get_terrain_width(), terrain->get_terrain_height());
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
