#ifndef ENTITYSHADER_H
#define ENTITYSHADER_H

#define _USE_MATH_DEFINES

#include "../entities/Entity.h"
#include "../entities/Light.h"
#include "../entities/Camera.h"
#include "../Model.h"
#include "ShaderProgram.h"

#include <cstdio>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <GL/glew.h>

const std::string ENTITY_VERTEX_SHADER = "shaders/entity.vert";
const std::string ENTITY_FRAGMENT_SHADER = "shaders/entity.frag";


class EntityShader : public ShaderProgram {
private:
    GLuint location_texMap;
    GLuint location_cubeMap;
    GLuint location_shadowMap;
    GLuint location_clip_plane;

    GLuint location_projection;
    GLuint location_model;
    GLuint location_view;
    GLuint location_inv_view;

    GLuint location_num_lights;
    GLuint location_shininess;
    GLuint location_emission;

    GLuint location_mtl_ambient;
    GLuint location_mtl_diffuse;
    GLuint location_mtl_specular;

    GLuint location_depth_pv;
    GLuint location_render_shadows;
public:
    EntityShader();

    virtual void bindUniformLocations();

    void loadLights(std::vector<Light*> lights);
    void loadLight(Light* light, int i);
    void loadView(glm::mat4 view);
    void loadEntity(Entity* entity);
    void loadModelComponent(const ModelComponent& component);
    void loadProjection(glm::mat4 proj);
    void loadDepth(glm::mat4 pv);
    void loadClipPlane(glm::vec4 clip);
};

#endif //ENTITYSHADER_H

