#ifndef FRUSTRUM_H
#define FRUSTRUM_H

#include <glm/glm.hpp>

/**
 * @struct Plan
 * @brief Représente un plan dans l'espace 3D
 * 
 * Un plan est défini par son équation : dot(normal, point) + d = 0
 */
struct Plan
{
    glm::vec3 normal; /**< Vecteur normal unitaire du plan */
    float d;          /**< Distance du plan à l'origine */
};

/**
 * @struct Sphere
 * @brief Représente une sphère dans l'espace 3D
 * 
 * Utilisée pour les tests de collision avec le frustum
 */
struct Sphere
{
    glm::vec3 center; /**< Centre de la sphère */
    float radius;     /**< Rayon de la sphère */

    /**
     * @brief Constructeur de la sphère
     * @param c Centre de la sphère
     * @param r Rayon de la sphère
     */
    Sphere(glm::vec3 c, float r) : center(c), radius(r)
    {
    }
};

/**
 * @class Frustrum
 * @brief Gère le frustum de vision pour le culling
 * 
 * Le frustum est défini par 6 plans (gauche, droit, bas, haut, près, loin)
 * qui délimitent le volume visible par la caméra. Il permet de déterminer
 * quels objets sont potentiellement visibles.
 */
class Frustrum
{
private:
    Plan mPlans[6]; /**< Tableau des 6 plans du frustum */

public:
    /**
     * @brief Constructeur par défaut
     */
    Frustrum();

    /**
     * @brief Destructeur
     */
    ~Frustrum();

    /**
     * @brief Met à jour les plans du frustum à partir des matrices
     * @param projection Matrice de projection
     * @param view Matrice de vue
     * 
     * Calcule les 6 plans du frustum à partir de la combinaison
     * des matrices projection et vue.
     */
    void updateFrustum(glm::mat4 &projection, glm::mat4 &view);

    /**
     * @brief Teste si une sphère est dans le frustum
     * @param patchCentre Centre de la sphère (patch)
     * @param radius Rayon de la sphère
     * @return true si la sphère est (au moins partiellement) dans le frustum
     * 
     * Utilise l'algorithme de test sphère/frustum pour déterminer
     * si un patch est potentiellement visible.
     */
    bool isPatchInFrustum(const glm::vec3 &patchCentre, float radius);
};

#endif