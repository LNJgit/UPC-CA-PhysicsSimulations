#include "scenesph.h"
#include "glutils.h"
#include "model.h"
#include <QOpenGLFunctions_3_3_Core>
#include <iostream>

SceneSPH::SceneSPH() {
    widget = new WidgetSPH();
    connect(widget, &WidgetSPH::updatedParameters, this, &SceneSPH::updateSimParams);
}

SceneSPH::~SceneSPH() {
    if (widget)     delete widget;
    if (shader)     delete shader;
    if (vaoFloor)   delete vaoFloor;
    if (vaoSphereH) delete vaoSphereH;
    if (vaoSphereL) delete vaoSphereL;
    if (vaoCube)    delete vaoCube;
    if (fGravity)   delete fGravity;
}

void SceneSPH::initialize() {
    // Load shader
    shader = glutils::loadShaderProgram(":/shaders/phong.vert", ":/shaders/phong.frag");

    // Initialize floor VAO
    Model quad = Model::createQuad();
    vaoFloor = glutils::createVAO(shader, &quad, buffers);
    glutils::checkGLError();

    // Initialize particle VAOs for low-res and high-res spheres
    Model sphereLowres = Model::createIcosphere(1);
    vaoSphereL = glutils::createVAO(shader, &sphereLowres, buffers);
    numFacesSphereL = sphereLowres.numFaces();
    glutils::checkGLError();

    Model sphere = Model::createIcosphere(3);
    vaoSphereH = glutils::createVAO(shader, &sphere, buffers);
    numFacesSphereH = sphere.numFaces();
    glutils::checkGLError();

    // Initialize cube VAO
    Model cube = Model::createCube();
    vaoCube = glutils::createVAO(shader, &cube, buffers);
    glutils::checkGLError();

    // Initialize SPH parameters and force
    fGravity = new ForceConstAcceleration(Vec3(0.0, -9.81, 0.0));
    system.addForce(fGravity);

    // Set up colliders
    colliderFloor.setPlane(Vec3(0, 1, 0), 0);
    colliderSphere.setCenter(Vec3(0, 0, 0));
    colliderSphere.setRadius(10);
    colliderBox.setFromBounds(Vec3(30, 0, 20), Vec3(50, 10, 60));
    colliderParticles.setCellSize(1);

    // Load initial parameters from UI
    updateSimParams();
}

void SceneSPH::reset() {
    updateSimParams();
}

void SceneSPH::updateSimParams() {
    if (widget) {
    }
}

void SceneSPH::paint(const Camera& camera) {
    QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();

    shader->bind();
    shader->setUniformValue("ProjMatrix", camera.getPerspectiveMatrix());
    shader->setUniformValue("ViewMatrix", camera.getViewMatrix());

    // Draw floor
    vaoFloor->bind();
    QMatrix4x4 modelMat;
    modelMat.scale(100, 1, 100);
    shader->setUniformValue("ModelMatrix", modelMat);
    shader->setUniformValue("matdiff", 0.8f, 0.8f, 0.8f);
    shader->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shader->setUniformValue("matshin", 0.0f);
    glFuncs->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Draw particles
    vaoSphereL->bind();
    for (const Particle* particle : system.getParticles()) {
        Vec3 p = particle->pos;
        Vec3 c = particle->color;
        double r = particle->radius;

        modelMat.setToIdentity();
        modelMat.translate(p[0], p[1], p[2]);
        modelMat.scale(r);
        shader->setUniformValue("ModelMatrix", modelMat);
        shader->setUniformValue("matdiff", GLfloat(c[0]), GLfloat(c[1]), GLfloat(c[2]));
        shader->setUniformValue("matspec", 1.0f, 1.0f, 1.0f);
        shader->setUniformValue("matshin", 100.f);

        glFuncs->glDrawElements(GL_TRIANGLES, 3 * numFacesSphereL, GL_UNSIGNED_INT, 0);
    }

    // Draw collider sphere
    vaoSphereH->bind();
    Vec3 cc = colliderSphere.getCenter();
    modelMat.setToIdentity();
    modelMat.translate(cc[0], cc[1], cc[2]);
    modelMat.scale(colliderSphere.getRadius());
    shader->setUniformValue("ModelMatrix", modelMat);
    shader->setUniformValue("matdiff", 0.8f, 0.4f, 0.4f);
    shader->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shader->setUniformValue("matshin", 0.0f);
    glFuncs->glDrawElements(GL_TRIANGLES, 3 * numFacesSphereH, GL_UNSIGNED_INT, 0);

    shader->release();
}

void SceneSPH::update(double dt) {
    //system.update(dt);

    colliderParticles.particleMap.clear();
    for (Particle* p : system.getParticles()) {
        colliderParticles.addParticle(p);
    }

    // Handle collisions
    Collision colInfo;
    for (Particle* p : system.getParticles()) {
        if (colliderFloor.testCollision(p, colInfo)) {
            colliderFloor.resolveCollision(p, colInfo, kBounce, kFriction);
        }
        if (colliderSphere.testCollision(p, colInfo)) {
            colliderSphere.resolveCollision(p, colInfo, kBounce, kFriction);
        }
        if (colliderBox.testCollision(p, colInfo)) {
            colliderBox.resolveCollision(p, colInfo, kBounce, kFriction);
        }
        if (colliderParticles.testCollision(p, colInfo)) {
            colliderParticles.resolveCollisionParticles(colInfo, kBounce, kFriction);
        }
    }
}

void SceneSPH::mousePressed(const QMouseEvent* e, const Camera&) {
    mouseX = e->pos().x();
    mouseY = e->pos().y();
}

void SceneSPH::mouseMoved(const QMouseEvent* e, const Camera& cam) {
    int dx = e->pos().x() - mouseX;
    int dy = e->pos().y() - mouseY;
    mouseX = e->pos().x();
    mouseY = e->pos().y();

    Vec3 disp = cam.worldSpaceDisplacement(dx, -dy, cam.getEyeDistance());

    if (e->buttons() & Qt::RightButton) {
        if (!e->modifiers()) {
            colliderSphere.setCenter(colliderSphere.getCenter() + disp);
        }
        else if (e->modifiers() & Qt::ShiftModifier) {
            colliderBox.setFromCenterSize(colliderBox.getCenter() + disp, colliderBox.getSize());
        }
    }
}
