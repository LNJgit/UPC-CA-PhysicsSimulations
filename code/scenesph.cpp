#include "scenesph.h"
#include "glutils.h"
#include "model.h"
#include <QOpenGLFunctions_3_3_Core>
#include <iostream>

// Constructor and Destructor
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
    delete vaoCube4;
    delete vaoCube5;
    delete vaoCube6;
    delete vaoCube7;
    delete fGravity;
}

//Loading models and VAOS
void SceneSPH::loadModelAndVaos()
{
    // Load models and VAOs
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

    Model cube4 = Model::createCube();
    vaoCube4 = glutils::createVAO(shader, &cube4, buffers);
    glutils::checkGLError();

    Model cube5 = Model::createCube();
    vaoCube5 = glutils::createVAO(shader, &cube5, buffers);
    glutils::checkGLError();

    Model cube6 = Model::createCube();
    vaoCube6 = glutils::createVAO(shader, &cube6, buffers);
    glutils::checkGLError();

    Model cube7 = Model::createCube();
    vaoCube7 = glutils::createVAO(shader, &cube7, buffers);
    glutils::checkGLError();
}

void SceneSPH::initializeSPHParticles()
{
    sph.clearParticles();
    sph.Particles.clear();
    for (Particle* p : system.getParticles()) {
        sph.addParticle(p);
    }
    sph.setCellSize(3.0);
    sph.setSmoothingLength(3.0);
}

// Initialization
void SceneSPH::initialize() {
    shader = glutils::loadShaderProgram(":/shaders/phong.vert", ":/shaders/phong.frag");

    loadModelAndVaos();

    fGravity = new ForceConstAcceleration(Vec3(0.0, -9.81, 0.0));
    system.addForce(fGravity);

    colliderFloor.setPlane(Vec3(0, 1, 0), 0);
    updateSimParams();

}

// Cube Initialization
void SceneSPH::initializeCube(int numParticles, double x, double y, double z) {
    Vec3 cubeMin(-x, 10, -z);
    Vec3 cubeMax(x, y, z);

    for (int i = 0; i < numParticles; ++i) {
        Particle* particle = new Particle();
        particle->pos = Vec3(Random::get(cubeMin[0], cubeMax[0]), Random::get(cubeMin[1], cubeMax[1]), Random::get(cubeMin[2], cubeMax[2]));
        particle->vel = Vec3(0, 0, 0);
        particle->color = Vec3(0.6, 0.7, 0.9);
        particle->radius = 1.0;
        particle->life = 50.0;
        particle->mass = particleMass;
        particle->density = 0.0;
        particle->pressure = 0.0;
        particle->id = i;
        particle->ghost = false;

        system.addParticle(particle);
        fGravity->addInfluencedParticle(particle);
    }
}

// Reset Simulation
void SceneSPH::reset() {
    updateSimParams();
    initializeSPHParticles();
    Random::seed(1337);
    fGravity->clearInfluencedParticles();
    system.deleteParticles();
    initializeCube(numInitialParticles, (cubeWidth-10)/2, cubeHeight-10, (cubeWidth-10)/2);
    createSmallColliderContainer(cubeWidth, cubeHeight, cubeWidth);
    createBigColliderContainer(cubeWidth*2, cubeHeight/4, cubeWidth*2);
}

// Update Simulation Parameters
void SceneSPH::updateSimParams() {
    if (widget) {
        viscosity = widget->getViscosity();
        restDensity = widget->getDensity();
        leftWallActive = widget->getLeftWallValue();
        rightWallActive = widget->getRightWallValue();
        frontWallActive = widget->getFrontWallValue();
        backWallActive = widget->getBackWallValue();
        cubeWidth = widget->getWidthValue();
        cubeHeight = widget->getHeightValue();
        numInitialParticles = widget->getNumInitialParticles();
        particleMass = widget->getParticleMass();
        cs = widget->getCs();
    }
}

void SceneSPH::drawColliders()
{
    QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();
    if (backWallActive)
    {
        vaoCube1->bind();
        drawColliderCube(smallContainerBack, shader, glFuncs);
        vaoCube1->release();
    }
    if (leftWallActive) {
        vaoCube2->bind();
        drawColliderCube(smallContainerLeft, shader, glFuncs);
        vaoCube2->release();
    }

    if (rightWallActive) {
        vaoCube3->bind();
        drawColliderCube(smallContainerRight, shader, glFuncs);
        vaoCube3->release();
    }

    // Draw floor
    vaoFloor->bind();
    QMatrix4x4 modelMat;
    modelMat.scale(100, 1, 100);
    shader->setUniformValue("ModelMatrix", modelMat);
    shader->setUniformValue("matdiff", 0.8f, 0.8f, 0.8f);
    shader->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shader->setUniformValue("matshin", 0.0f);
    glFuncs->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    vaoFloor->release();

    vaoCube4->bind();
    drawColliderCube(bigContainerBack, shader, glFuncs);
    vaoCube4->release();

    vaoCube5->bind();
    drawColliderCube(bigContainerFront, shader, glFuncs);
    vaoCube5->release();

    vaoCube6->bind();
    drawColliderCube(bigContainerLeft, shader, glFuncs);
    vaoCube6->release();

    vaoCube7->bind();
    drawColliderCube(bigContainerRight, shader, glFuncs);
    vaoCube7->release();
}

void SceneSPH::drawParticles(bool paintDensity)
{
    QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();
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
        if (!particle->ghost) {
            Vec3 p = particle->pos;
            double r = particle->radius;
            double value = paintDensity ? particle->density : particle->pressure;
            Vec3 color = paintDensity
                             ? Vec3(value, 0.0f, 1.0f - value)  // Blue gradient for density
                             : Vec3(1.0f, 1.0f - value, 0.0f);

            float valueNorm = useLogScale
                                  ? float((std::log10(value + 1.0) - std::log10(clampMinValue + 1.0)) / (std::log10(clampMaxValue + 1.0) - std::log10(clampMinValue + 1.0)))
                                  : float((value - clampMinValue) / (clampMaxValue - clampMinValue));

            valueNorm = std::clamp(valueNorm, 0.0f, 1.0f);
            color = Vec3(1.0f * valueNorm, 0.0f, (1.0f - valueNorm));

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
    }
    vaoSphereL->release();
}

// Render Scene
void SceneSPH::paint(const Camera& camera) {
    QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();

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



    drawParticles(paintMode);
    drawColliders();

    shader->release();
}


// Simulation Update
void SceneSPH::update(double dt) {
    Vecd ppos = system.getPositions();
    integrator.step(system, dt);
    system.setPreviousPositions(ppos);
    paintMode = widget->getPaintMode();
    std::cout<<"PAINT MODE: "<<paintMode;


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
        minDensity = std::min(minDensity, particle.density);
        maxDensity = std::max(maxDensity, particle.density);
        totalDensity += particle.density;
    }

    avgDensity = totalDensity / sph.Particles.size();
    std::cout << "Density - Min: " << minDensity << ", Max: " << maxDensity << ", Avg: " << avgDensity << std::endl;

    double minPressure = std::numeric_limits<double>::max();
    double maxPressure = std::numeric_limits<double>::lowest();
    double totalPressure = 0.0;

    for (auto& particle : sph.Particles) {
        if (!particle.p->ghost) {
            sph.computePressure(particle, restDensity, cs);
            minPressure = std::min(minPressure, particle.pressure);
            maxPressure = std::max(maxPressure, particle.pressure);
            totalPressure += particle.pressure;
        }
    }
    avgPressure = totalPressure / sph.Particles.size();
    std::cout << "Pressure - Min: " << minPressure << ", Max: " << maxPressure << ", Avg: " << avgPressure << std::endl;

    for (auto& particle : sph.Particles) {
        if (particle.p->ghost) {
            particle.p->pressure = maxPressure;
        }
    }

    for (auto& particle : sph.Particles) {
        if (!particle.p->ghost) {
            sph.computePressureForces(particle, viscosity);
        }
    }

    colliderParticles.particleMap.clear();
    for (Particle* p : system.getParticles()) {
        colliderParticles.addParticle(p);
    }

    for (Particle* p : system.getParticles()) {
        if (!p->ghost) {
            std::vector<Collision> collisions;
            Collision colInfo;
            colInfo.collisionWall = false;

            if (colliderFloor.testCollision(p, colInfo)) {
                collisions.push_back(colInfo);
            }
            if(frontWallActive)
            {
                if (smallContainerFront.testCollision(p, colInfo)) {
                    collisions.push_back(colInfo);
                    colInfo.collisionWall = true;
                }
            }
            if(leftWallActive)
            {
                if (smallContainerLeft.testCollision(p, colInfo)) {
                    collisions.push_back(colInfo);
                    colInfo.collisionWall = true;
                }
            }
            if(rightWallActive)
            {
                if (smallContainerRight.testCollision(p, colInfo)) {
                    collisions.push_back(colInfo);
                    colInfo.collisionWall = true;
                }
            }
            if(backWallActive)
            {
                if (smallContainerBack.testCollision(p, colInfo)) {
                    collisions.push_back(colInfo);
                    colInfo.collisionWall = true;
                }
            }

            if (bigContainerBack.testCollision(p, colInfo)) {
                collisions.push_back(colInfo);
                colInfo.collisionWall = true;
            }

            if (bigContainerLeft.testCollision(p, colInfo)) {
                collisions.push_back(colInfo);
                colInfo.collisionWall = true;
            }

            if (bigContainerRight.testCollision(p, colInfo)) {
                collisions.push_back(colInfo);
                colInfo.collisionWall = true;
            }

            if (bigContainerFront.testCollision(p, colInfo)) {
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

// Mouse Events
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
            smallContainerFront.setFromCenterSize(smallContainerFront.getCenter() + disp, smallContainerFront.getSize());
        }
    }
}

// Draw Collider Cubes
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

// Collider Setup
void SceneSPH::createSmallColliderContainer(double width, double height, double depth) {
    double halfWidth = width / 2.0;
    double halfHeight = height;
    double halfDepth = depth / 2.0;

    colliderFloor.setPlane(Vec3(0, 1, 0), 0);
    smallContainerFront.setFromBounds(Vec3(-halfWidth, -10, halfDepth - 2), Vec3(halfWidth, halfHeight, halfDepth));
    smallContainerBack.setFromBounds(Vec3(-halfWidth, -10, -halfDepth), Vec3(halfWidth, halfHeight, -halfDepth + 2));
    smallContainerLeft.setFromBounds(Vec3(-halfWidth, -10, -halfDepth), Vec3(-halfWidth + 2, halfHeight, halfDepth));
    smallContainerRight.setFromBounds(Vec3(halfWidth - 2, -10, -halfDepth), Vec3(halfWidth, halfHeight, halfDepth));
    //initializeGhostParticles(width, height, depth);
}

void SceneSPH::createBigColliderContainer(double width, double height, double depth) {
    double halfWidth = width / 2.0;
    double halfHeight = height;
    double halfDepth = depth / 2.0;

    colliderFloor.setPlane(Vec3(0, 1, 0), 0);
    bigContainerFront.setFromBounds(Vec3(-halfWidth, -10, halfDepth - 2), Vec3(halfWidth, halfHeight, halfDepth));
    bigContainerLeft.setFromBounds(Vec3(-halfWidth, -10, -halfDepth), Vec3(halfWidth, halfHeight, -halfDepth + 2));
    bigContainerRight.setFromBounds(Vec3(-halfWidth, -10, -halfDepth), Vec3(-halfWidth + 2, halfHeight, halfDepth));
    bigContainerBack.setFromBounds(Vec3(halfWidth - 2, -10, -halfDepth), Vec3(halfWidth, halfHeight, halfDepth));
    //initializeGhostParticles(width, height, depth);
}

// Ghost Particles Initialization
void SceneSPH::initializeGhostParticles(double width, double height, double depth) {
    double halfWidth = width / 2.0;
    double halfHeight = height;
    double halfDepth = depth / 2.0;
    double spacing = sph.getSmoothingLength();

    auto addGhostParticle = [&](double x, double y, double z) {
        Particle* ghostParticle = new Particle();
        ghostParticle->pos = Vec3(x, y, z);
        ghostParticle->vel = Vec3(0, 0, 0);
        ghostParticle->color = Vec3(1.0, 1.0, 1.0);
        ghostParticle->radius = 1.0;
        ghostParticle->life = 50.0;
        ghostParticle->mass = particleMass;
        ghostParticle->density = restDensity;
        ghostParticle->pressure = 100000;
        ghostParticle->id = system.getParticles().size();
        ghostParticle->ghost = true;

        sph.addParticle(ghostParticle);
        system.addParticle(ghostParticle);
    };

    for (double x = -halfWidth; x <= halfWidth; x += spacing) {
        for (double y = 0; y <= halfHeight; y += spacing) {
            addGhostParticle(x, y, halfDepth);
            addGhostParticle(x, y, -halfDepth);
        }
    }

    for (double z = -halfDepth; z <= halfDepth; z += spacing) {
        for (double y = 0; y <= halfHeight; y += spacing) {
            addGhostParticle(-halfWidth, y, z);
            addGhostParticle(halfWidth, y, z);
        }
    }
}

void SceneSPH::setNumInitialParticles()
{
    numInitialParticles = widget->getNumInitialParticles();
}


