#include "scenefountain.h"
#include "glutils.h"
#include "model.h"
#include <QOpenGLFunctions_3_3_Core>
#include <iostream>

SceneFountain::SceneFountain() {
    widget = new WidgetFountain();
    connect(widget, SIGNAL(updatedParameters()), this, SLOT(updateSimParams()));
}

SceneFountain::~SceneFountain() {
    if (widget)     delete widget;
    if (shader)     delete shader;
    if (vaoFloor)   delete vaoFloor;
    if (vaoSphereH) delete vaoSphereH;
    if (vaoSphereL) delete vaoSphereL;
    if (vaoCube)    delete vaoCube;
    if (fGravity)   delete fGravity;
    if (fAttractor) delete fAttractor;
}

void SceneFountain::createBlackHole(const Vec3& position, double radius, double mass) {
    double gravitationalConstant = 1.0;
    fAttractor = new ForceGravitationalAttractor(position, mass, gravitationalConstant);
    system.addForce(fAttractor);

    blackHole.setCenter(position);
    blackHole.setRadius(radius);

    Model sphere = Model::createIcosphere(3);
    vaoBlackHole = glutils::createVAO(shader, &sphere, buffers);
    numFacesSphereH = sphere.numFaces();
    glutils::checkGLError();
}

void SceneFountain::initializeShaderAndVAOS() {
    shader = glutils::loadShaderProgram(":/shaders/phong.vert", ":/shaders/phong.frag");

    Model quad = Model::createQuad();
    vaoFloor = glutils::createVAO(shader, &quad, buffers);
    glutils::checkGLError();

    Model sphereLowres = Model::createIcosphere(1);
    vaoSphereL = glutils::createVAO(shader, &sphereLowres, buffers);
    numFacesSphereL = sphereLowres.numFaces();
    glutils::checkGLError();

    Model sphere = Model::createIcosphere(3);
    vaoSphereH = glutils::createVAO(shader, &sphere, buffers);
    numFacesSphereH = sphere.numFaces();
    glutils::checkGLError();

    Model sphere2 = Model::createIcosphere(3);
    vaoBlackHole = glutils::createVAO(shader, &sphere2, buffers);
    numFacesSphereH = sphere2.numFaces();
    glutils::checkGLError();

    Model cube = Model::createCube();
    vaoCube = glutils::createVAO(shader, &cube, buffers);
    glutils::checkGLError();
}

void SceneFountain::setCollidersAndFountain() {
    fountainPos = Vec3(0, 80, 0);
    colliderFloor.setPlane(Vec3(0, 1, 0), 0);
    colliderSphere.setCenter(Vec3(0, 0, 0));
    colliderSphere.setRadius(10);
    colliderBox.setFromBounds(Vec3(30, 0, 20), Vec3(50, 10, 60));

    ColliderAABB colliderBox1, colliderBox2, colliderBox3, colliderBox4;
    colliderBox1.setFromBounds(Vec3(-10, -5, 10), Vec3(10, 5, 11));
    colliderBox2.setFromBounds(Vec3(-10, -5, -11), Vec3(10, 5, -10));
    colliderBox3.setFromBounds(Vec3(-11, -5, -10), Vec3(-10, 5, 10));
    colliderBox4.setFromBounds(Vec3(10, -5, -10), Vec3(11, 5, 10));
    colliderParticles.setCellSize(1);
}

void SceneFountain::initialize() {
    updateSimParams();
    initializeShaderAndVAOS();

    fGravity = new ForceConstAcceleration();
    system.addForce(fGravity);

    setCollidersAndFountain();
}

void SceneFountain::reset() {
    updateSimParams();
    Random::seed(1337);

    fGravity->clearInfluencedParticles();
    fAttractor->clearInfluencedParticles();
    system.deleteParticles();
    deadParticles.clear();
    if (blackHoleEnabled) {
        createBlackHole(Vec3(0, 100, 0), blackHoleRadius, blackHoleMass);
    }
}

void SceneFountain::updateSimParams() {
    double g = widget->getGravity();
    fGravity->setAcceleration(Vec3(0, -g, 0));
    emitRate = widget->getEmitRate();
    kFriction = widget->getFriction();
    kBounce = widget->getElasticity();
    maxParticleLife = widget->getParticleLife();

    double newBlackHoleRadius = widget->getBlackHoleRadius();
    double newBlackHoleMass = widget->getBlackHoleMass();
    bool newBlackHoleEnabled = widget->blackHoleEnabled();

    if (newBlackHoleEnabled != blackHoleEnabled ||
        (newBlackHoleEnabled && (newBlackHoleRadius != blackHoleRadius || newBlackHoleMass != blackHoleMass))) {

        blackHoleRadius = newBlackHoleRadius;
        blackHoleMass = newBlackHoleMass;
        blackHoleEnabled = newBlackHoleEnabled;

        if (blackHoleEnabled) {
            fAttractor->setAttractorMass(blackHoleMass);
            fAttractor->clearInfluencedParticles();
            blackHole.setRadius(blackHoleRadius);

            for (Particle* p : system.getParticles()) {
                fAttractor->addInfluencedParticle(p);
            }
        } else if (fAttractor) {
            fAttractor->clearInfluencedParticles();
        }
    }
}

void SceneFountain::paint(const Camera& camera) {
    QOpenGLFunctions* glFuncs = nullptr;
    glFuncs = QOpenGLContext::currentContext()->functions();

    shader->bind();

    QMatrix4x4 camProj = camera.getPerspectiveMatrix();
    QMatrix4x4 camView = camera.getViewMatrix();
    shader->setUniformValue("ProjMatrix", camProj);
    shader->setUniformValue("ViewMatrix", camView);

    const int numLights = 1;
    const QVector3D lightPosWorld[numLights] = {QVector3D(100, 500, 100)};
    const QVector3D lightColor[numLights] = {QVector3D(1, 1, 1)};
    QVector3D lightPosCam[numLights];
    for (int i = 0; i < numLights; i++) {
        lightPosCam[i] = camView.map(lightPosWorld[i]);
    }
    shader->setUniformValue("numLights", numLights);
    shader->setUniformValueArray("lightPos", lightPosCam, numLights);
    shader->setUniformValueArray("lightColor", lightColor, numLights);

    vaoFloor->bind();
    QMatrix4x4 modelMat;
    modelMat.scale(100, 1, 100);
    shader->setUniformValue("ModelMatrix", modelMat);
    shader->setUniformValue("matdiff", 0.8f, 0.8f, 0.8f);
    shader->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shader->setUniformValue("matshin", 0.0f);
    glFuncs->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    vaoSphereL->bind();
    for (const Particle* particle : system.getParticles()) {
        Vec3 p = particle->pos;
        Vec3 c = particle->color;
        double r = particle->radius;

        modelMat = QMatrix4x4();
        modelMat.translate(p[0], p[1], p[2]);
        modelMat.scale(r);
        shader->setUniformValue("ModelMatrix", modelMat);

        shader->setUniformValue("matdiff", GLfloat(c[0]), GLfloat(c[1]), GLfloat(c[2]));
        shader->setUniformValue("matspec", 1.0f, 1.0f, 1.0f);
        shader->setUniformValue("matshin", 100.f);

        glFuncs->glDrawElements(GL_TRIANGLES, 3 * numFacesSphereL, GL_UNSIGNED_INT, 0);
    }

    vaoSphereH->bind();
    Vec3 cc = colliderSphere.getCenter();
    modelMat = QMatrix4x4();
    modelMat.translate(cc[0], cc[1], cc[2]);
    modelMat.scale(colliderSphere.getRadius());
    shader->setUniformValue("ModelMatrix", modelMat);
    shader->setUniformValue("matdiff", 0.8f, 0.4f, 0.4f);
    shader->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shader->setUniformValue("matshin", 0.0f);
    glFuncs->glDrawElements(GL_TRIANGLES, 3 * numFacesSphereH, GL_UNSIGNED_INT, 0);

    if (blackHoleEnabled) {
        vaoBlackHole->bind();
        cc = blackHole.getCenter();
        modelMat = QMatrix4x4();
        modelMat.translate(cc[0], cc[1], cc[2]);
        modelMat.scale(blackHole.getRadius());
        shader->setUniformValue("ModelMatrix", modelMat);
        shader->setUniformValue("matdiff", 0.05f, 0.05f, 0.05f);
        shader->setUniformValue("matspec", 0.1f, 0.1f, 0.1f);
        shader->setUniformValue("matshin", 0.0f);
        glFuncs->glDrawElements(GL_TRIANGLES, 3 * numFacesSphereH, GL_UNSIGNED_INT, 0);
    }

    vaoCube->bind();
    cc = colliderBox.getCenter();
    Vec3 hs = 0.5 * colliderBox.getSize();
    modelMat = QMatrix4x4();
    modelMat.translate(cc[0], cc[1], cc[2]);
    modelMat.scale(hs[0], hs[1], hs[2]);
    shader->setUniformValue("ModelMatrix", modelMat);
    shader->setUniformValue("matdiff", 0.4f, 0.8f, 0.4f);
    shader->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shader->setUniformValue("matshin", 0.0f);
    glFuncs->glDrawElements(GL_TRIANGLES, 3 * 2 * 6, GL_UNSIGNED_INT, 0);
    vaoCube->release();
    shader->release();
}

void SceneFountain::update(double dt) {
    int emitParticles = std::max(1, int(std::round(emitRate * dt)));
    for (int i = 0; i < emitParticles; i++) {
        Particle* p;
        if (!deadParticles.empty()) {
            p = deadParticles.front();
            deadParticles.pop_front();
        } else {
            p = new Particle();
            system.addParticle(p);
            fGravity->addInfluencedParticle(p);
            if (blackHoleEnabled) {
                fAttractor->addInfluencedParticle(p);
            }
        }

        p->color = Vec3(153 / 255.0, 217 / 255.0, 234 / 255.0);
        p->radius = 1.0;
        p->life = maxParticleLife;

        double x = Random::get(-10.0, 10.0);
        double y = 0;
        double z = Random::get(-10.0, 10.0);
        p->pos = Vec3(x, y, z) + fountainPos;
        p->vel = Vec3(0, 0, 0);
    }

    Vecd ppos = system.getPositions();
    integrator.step(system, dt);
    system.setPreviousPositions(ppos);

    colliderParticles.particleMap.clear();
    for (Particle* p : system.getParticles()) {
        colliderParticles.addParticle(p);
    }

    Collision colInfo;
    for (Particle* p : system.getParticles()) {
        if (colliderFloor.testCollision(p, colInfo)) {
            colliderFloor.resolveCollision(p, colInfo, kBounce, kFriction);
        }
        if (colliderSphere.testCollision(p, colInfo)) {
            colliderSphere.resolveCollision(p, colInfo, kBounce, kFriction);
        }
        if (blackHoleEnabled && blackHole.testCollision(p, colInfo)) {
            blackHole.resolveCollision(p, colInfo, kBounce, kFriction);
        }
        if (colliderBox.testCollision(p, colInfo)) {
            colliderBox.resolveCollision(p, colInfo, kBounce, kFriction);
        }
        if (colliderParticles.testCollision(p, colInfo)) {
            colliderParticles.resolveCollisionParticles(colInfo, kBounce, kFriction);
        }
    }

    for (Particle* p : system.getParticles()) {
        if (p->life > 0) {
            p->life -= dt;
            if (p->life < 0) {
                deadParticles.push_back(p);
            }
        }
    }
}

void SceneFountain::mousePressed(const QMouseEvent* e, const Camera&) {
    mouseX = e->pos().x();
    mouseY = e->pos().y();
}

void SceneFountain::mouseMoved(const QMouseEvent* e, const Camera& cam) {
    int dx = e->pos().x() - mouseX;
    int dy = e->pos().y() - mouseY;
    mouseX = e->pos().x();
    mouseY = e->pos().y();

    Vec3 disp = cam.worldSpaceDisplacement(dx, -dy, cam.getEyeDistance());

    if (e->buttons() & Qt::RightButton) {
        if (!e->modifiers()) {
            fountainPos += disp;
        } else if (e->modifiers() & Qt::ShiftModifier) {
            colliderBox.setFromCenterSize(
                colliderBox.getCenter() + disp,
                colliderBox.getSize());
        }
    }
}
