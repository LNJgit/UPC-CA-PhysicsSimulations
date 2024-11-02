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
    delete widget;
    delete shader;
    delete vaoFloor;
    delete vaoSphereH;
    delete vaoSphereL;
    delete vaoCube1;
    delete vaoCube2;
    delete vaoCube3;
    delete fGravity;
}

void SceneSPH::initialize() {
    shader = glutils::loadShaderProgram(":/shaders/phong.vert", ":/shaders/phong.frag");

    sph.setCellSize(3.0);
    sph.setSmoothingLength(3.0);

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

    Model cube1 = Model::createCube();
    vaoCube1 = glutils::createVAO(shader, &cube1, buffers);
    glutils::checkGLError();

    Model cube2 = Model::createCube();
    vaoCube2 = glutils::createVAO(shader, &cube2, buffers);
    glutils::checkGLError();

    Model cube3 = Model::createCube();
    vaoCube3 = glutils::createVAO(shader, &cube3, buffers);
    glutils::checkGLError();

    fGravity = new ForceConstAcceleration(Vec3(0.0, -9.81, 0.0));
    system.addForce(fGravity);

    colliderFloor.setPlane(Vec3(0, 1, 0), 0);


    createColliderContainer(75,30,75);
    sph.clearParticles();
    sph.Particles.clear();

    for (Particle* p : system.getParticles()) {
        sph.addParticle(p);
    }
    sph.initializeReferenceDensity();


    updateSimParams();
}

void SceneSPH::initializeCube(int numParticles,double x, double y, double z) {
    Vec3 cubeMin(-x, 10, -z);
    Vec3 cubeMax(x, y, z);

    for (int i = 0; i < numParticles; ++i) {
        Particle* particle = new Particle();
        particle->pos = Vec3(Random::get(cubeMin[0], cubeMax[0]), Random::get(cubeMin[1], cubeMax[1]), Random::get(cubeMin[2], cubeMax[2]));
        particle->vel = Vec3(0, 0, 0);
        particle->color = Vec3(0.6, 0.7, 0.9);
        particle->radius = 1.0;
        particle->life = 50.0;
        particle->mass = 0.01;
        particle->density = 0.0;
        particle->id = i;
        particle->ghost = false;

        system.addParticle(particle);
        fGravity->addInfluencedParticle(particle);
    }

}

void SceneSPH::reset() {
    updateSimParams();
    Random::seed(1337);
    fGravity->clearInfluencedParticles();
    system.deleteParticles();
    initializeCube(numInitialParticles,30,40,30);
    initializeGhostParticles(71,2,71);
}

void SceneSPH::updateSimParams() {
    if (widget) {
        // Update simulation parameters from the UI widget
    }
}

void SceneSPH::paint(const Camera& camera) {
    QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();
    bool paintDensity = false;

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

    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::lowest();
    for (const Particle* particle : system.getParticles()) {
        double value = paintDensity ? particle->density : particle->pressure;
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }

    double clampMinValue = minValue;
    double clampMaxValue = maxValue * 1.1;
    bool useLogScale = !paintDensity && (clampMaxValue > 1000);

    vaoSphereL->bind();
    for (const Particle* particle : system.getParticles()) {
        Vec3 p = particle->pos;
        double r = particle->radius;
        double value = paintDensity ? particle->density : particle->pressure;

        float valueNorm;
        if (useLogScale) {
            valueNorm = float((std::log10(value + 1.0) - std::log10(clampMinValue + 1.0)) / (std::log10(clampMaxValue + 1.0) - std::log10(clampMinValue + 1.0)));
        } else {
            valueNorm = float((value - clampMinValue) / (clampMaxValue - clampMinValue));
        }
        valueNorm = std::clamp(valueNorm, 0.0f, 1.0f);

        Vec3 color = Vec3(1.0f * valueNorm, 0.0f, (1.0f - valueNorm));

        QMatrix4x4 modelMat;
        modelMat.setToIdentity();
        modelMat.translate(p[0], p[1], p[2]);
        modelMat.scale(r);
        shader->setUniformValue("ModelMatrix", modelMat);
        shader->setUniformValue("matdiff", GLfloat(color[0]), GLfloat(color[1]), GLfloat(color[2]));
        shader->setUniformValue("matspec", 1.0f, 1.0f, 1.0f);
        shader->setUniformValue("matshin", 100.f);

        glFuncs->glDrawElements(GL_TRIANGLES, 3 * numFacesSphereL, GL_UNSIGNED_INT, 0);
    }
    vaoSphereL->release();

    vaoCube1->bind();
    drawColliderCube(colliderBox2, shader, glFuncs);
    vaoCube1->release();

    vaoCube2->bind();
    drawColliderCube(colliderBox3, shader, glFuncs);
    vaoCube2->release();

    vaoCube3->bind();
    drawColliderCube(colliderBox4, shader, glFuncs);
    vaoCube3->release();

    shader->release();
}

void SceneSPH::update(double dt) {
    Vecd ppos = system.getPositions();
    integrator.step(system, dt);
    system.setPreviousPositions(ppos);

    sph.clearParticles();
    sph.Particles.clear();

    for (Particle* p : system.getParticles()) {
        sph.addParticle(p);
    }

    double minDensity = std::numeric_limits<double>::max();
    double maxDensity = std::numeric_limits<double>::lowest();
    double totalDensity = 0.0;

    for (auto& particle : sph.Particles) {
            sph.getNeighbors(particle);
            sph.computeDensity(particle);
            //std::cout << "Particle ID: " << particle.p->id << " Density: " << particle.density << "\n";
            minDensity = std::min(minDensity, particle.density);
            maxDensity = std::max(maxDensity, particle.density);
            totalDensity += particle.density;
    }

    double avgDensity = totalDensity / sph.Particles.size();
    std::cout << "Density - Min: " << minDensity << ", Max: " << maxDensity << ", Avg: " << avgDensity << std::endl;

    double minPressure = std::numeric_limits<double>::max();
    double maxPressure = std::numeric_limits<double>::lowest();
    double totalPressure = 0.0;

    for (auto& particle : sph.Particles) {
        if (!particle.p->ghost)
        {
            sph.computePressure(particle);
            minPressure = std::min(minPressure, particle.pressure);
            maxPressure = std::max(maxPressure, particle.pressure);
            totalPressure += particle.pressure;
        }
    }
    double avgPressure = totalPressure / sph.Particles.size();
    std::cout << "Pressure - Min: " << minPressure << ", Max: " << maxPressure << ", Avg: " << avgPressure << std::endl;
    for (auto& particle : sph.Particles) {
        if (particle.p->ghost)
        {
            particle.p->pressure=maxPressure;
        }
    }

    for (auto& particle : sph.Particles) {
        if (!particle.p->ghost)
        {
            sph.computePressureForces(particle);
        }
    }

    colliderParticles.particleMap.clear();
    for (Particle* p : system.getParticles()) {
        colliderParticles.addParticle(p);
    }

    for (Particle* p : system.getParticles()) {
        if(!p->ghost)
        {
            std::vector<Collision> collisions;
            Collision colInfo;
            colInfo.collisionWall = false;

            if (colliderFloor.testCollision(p, colInfo)) {
                collisions.push_back(colInfo);
            }
            if (colliderBox1.testCollision(p, colInfo)) {
                collisions.push_back(colInfo);
                colInfo.collisionWall = true;
            }
            if (colliderBox2.testCollision(p, colInfo)) {
                collisions.push_back(colInfo);
                colInfo.collisionWall = true;
            }
            if (colliderBox3.testCollision(p, colInfo)) {
                collisions.push_back(colInfo);
                colInfo.collisionWall = true;
            }
            if (colliderBox4.testCollision(p, colInfo)) {
                collisions.push_back(colInfo);
                colInfo.collisionWall = true;
            }

            if (!collisions.empty()) {
                colliderFloor.resolveCollisions(p, collisions, kBounce, kFriction);
            }
        }
    }
    system.clearForces();
    system.addForce(fGravity);
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
        } else if (e->modifiers() & Qt::ShiftModifier) {
            colliderBox1.setFromCenterSize(colliderBox1.getCenter() + disp, colliderBox1.getSize());
        }
    }
}

void SceneSPH::drawColliderCube(const ColliderAABB& collider, QOpenGLShaderProgram* shader, QOpenGLFunctions* glFuncs) {
    Vec3 cc = collider.getCenter();
    Vec3 hs = 0.5 * collider.getSize();
    QMatrix4x4 modelMat;
    modelMat.setToIdentity();
    modelMat.translate(cc[0], cc[1], cc[2]);
    modelMat.scale(hs[0], hs[1], hs[2]);
    shader->setUniformValue("ModelMatrix", modelMat);
    shader->setUniformValue("matdiff", 0.4f, 0.8f, 0.4f);
    shader->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shader->setUniformValue("matshin", 0.0f);
    glFuncs->glDrawElements(GL_TRIANGLES, 3 * 2 * 6, GL_UNSIGNED_INT, 0);
}

void SceneSPH::createColliderContainer(double width, double height, double depth) {
    // Calculate half-dimensions for convenience
    double halfWidth = width / 2.0;
    double halfHeight = height;
    double halfDepth = depth / 2.0;

    // Set up colliders
    colliderFloor.setPlane(Vec3(0, 1, 0), 0);
    colliderBox1.setFromBounds(Vec3(-halfWidth, -10, halfDepth - 2), Vec3(halfWidth, halfHeight, halfDepth)); // Back wall
    colliderBox2.setFromBounds(Vec3(-halfWidth, -10, -halfDepth), Vec3(halfWidth, halfHeight, -halfDepth + 2)); // Front wall
    colliderBox3.setFromBounds(Vec3(-halfWidth, -10, -halfDepth), Vec3(-halfWidth + 2, halfHeight, halfDepth)); // Left wall
    colliderBox4.setFromBounds(Vec3(halfWidth - 2, -10, -halfDepth), Vec3(halfWidth, halfHeight, halfDepth)); // Right wall
    initializeGhostParticles(width,height,depth);
}

void SceneSPH::initializeGhostParticles(double width, double height, double depth) {
    double halfWidth = width / 2.0;
    double halfHeight = height;
    double halfDepth = depth / 2.0;
    double spacing = sph.getSmoothingLength(); // Use the smoothing length as spacing for consistency

    auto addGhostParticle = [&](double x, double y, double z) {
        Particle* ghostParticle = new Particle();
        ghostParticle->pos = Vec3(x, y, z);
        ghostParticle->vel = Vec3(0, 0, 0);              // Static particle
        ghostParticle->color = Vec3(1.0, 1.0, 1.0);      // Distinct color for visualization
        ghostParticle->radius = 1.0;
        ghostParticle->life = 50.0;
        ghostParticle->mass = 0.01;
        ghostParticle->density = sph.getRestingDensity(); // Set to reference density
        ghostParticle->pressure = 1000000.0;                 // High pressure to repel real particles
        ghostParticle->id = system.getParticles().size(); // Unique ID
        ghostParticle->ghost = true;

        // Add the ghost particle to the SPH system and the main system
        sph.addParticle(ghostParticle);
        system.addParticle(ghostParticle);
    };



    // Back wall (z = halfDepth)
    for (double x = -halfWidth; x <= halfWidth; x += spacing) {
        for (double y = 0; y <= halfHeight; y += spacing) {
            addGhostParticle(x, y, halfDepth);
        }
    }

    // Front wall (z = -halfDepth)
    for (double x = -halfWidth; x <= halfWidth; x += spacing) {
        for (double y = 0; y <= halfHeight; y += spacing) {
            addGhostParticle(x, y, -halfDepth);
        }
    }

    // Left wall (x = -halfWidth)
    for (double z = -halfDepth; z <= halfDepth; z += spacing) {
        for (double y = 0; y <= halfHeight; y += spacing) {
            addGhostParticle(-halfWidth, y, z);
        }
    }

    // Right wall (x = halfWidth)
    for (double z = -halfDepth; z <= halfDepth; z += spacing) {
        for (double y = 0; y <= halfHeight; y += spacing) {
            addGhostParticle(halfWidth, y, z);
        }
    }
}


