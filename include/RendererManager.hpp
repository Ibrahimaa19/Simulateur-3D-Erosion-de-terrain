#ifndef RENDERER_MANAGER_H
#define RENDERER_MANAGER_H

#include "Terrain.hpp"
#include <glm/glm.hpp>

/**
 * @class RendererManager
 * @brief Gère le rendu du terrain avec système de LOD (Level of Detail)
 *
 * Cette classe est responsable du rendu du terrain en utilisant un système
 * de niveau de détail (LOD) pour optimiser les performances. Elle utilise
 * un frustum culling et gère l'activation/désactivation du LOD.
 */
class RendererManager
{
private:
    Terrain *mTerrain;    /** Pointeur vers le terrain à afficher */
    Frustrum *mFrustrum;  /** Frustum pour le culling des patches */
    bool mLodIsOn = true; /** État du système LOD (activé/désactivé) */

    /**
     * @brief Corrige les différences de LOD entre patches voisins
     *
     * Parcourt tous les patches et ajuste leurs niveaux de LOD pour éviter
     * des différences trop importantes entre patches adjacents, ce qui
     * pourrait causer des artefacts visuels.
     */
    void correctLod();

public:
    /**
     * @brief Constructeur du RendererManager
     * @param terrain Pointeur vers le terrain associé
     *
     * Initialise le gestionnaire de rendu et crée le frustum associé.
     */
    RendererManager(Terrain *terrain);

    /**
     * @brief Destructeur du RendererManager
     *
     * Nettoie les ressources allouées (notamment le frustum).
     */
    ~RendererManager();

    /**
     * @brief Effectue le rendu du terrain avec gestion LOD
     * @param cameraPos Position actuelle de la caméra
     * @param projection Matrice de projection
     * @param view Matrice de vue
     *
     * Fonction principale de rendu qui :
     * 1. Met à jour le frustum avec les matrices projection et vue
     * 2. Pour chaque patch, choisit le niveau de LOD approprié
     * 3. Applique la correction des LOD si nécessaire
     * 4. Affiche uniquement les patches visibles
     */
    void renderLod(const glm::vec3 &cameraPos, glm::mat4 &projection, glm::mat4 &view);

    /**
     * @brief Active ou désactive le système de LOD
     *
     * Bascule l'état du LOD. Quand désactivé, tous les patches sont rendus
     * avec le niveau de détail maximal.
     */
    void activateLod();

    /**
     * @brief Change le terrain associé au renderer
     * @param terrain Nouveau pointeur vers le terrain
     *
     * Permet de changer dynamiquement le terrain rendu par ce gestionnaire.
     */
    void setTerrain(Terrain *terrain);
};

#endif