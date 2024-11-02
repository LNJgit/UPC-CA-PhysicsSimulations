#include "scenecloth.h"
#include "glutils.h"
#include "model.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLBuffer>
#include <chrono>
#include <iomanip>
#include <ctime>


SceneCloth::SceneCloth() {
    widget = new WidgetCloth();
    connect(widget, SIGNAL(updatedParameters()), this, SLOT(updateSimParams()));
    connect(widget, SIGNAL(freeAnchors()), this, SLOT(freeAnchors()));
}

SceneCloth::~SceneCloth() {
    if (widget)      delete widget;
    if (shaderPhong) delete shaderPhong;
    if (vaoSphereS)  delete vaoSphereS;
    if (vaoSphereL)  delete vaoSphereL;
    if (vaoCube)     delete vaoCube;
    if (vaoMesh)     delete vaoMesh;
    if (vboMesh)     delete vboMesh;
    if (iboMesh)     delete iboMesh;

    system.deleteParticles();
    if (fGravity)  delete fGravity;
    for (ForceSpring* f : springsStretch) delete f;
    for (ForceSpring* f : springsShear) delete f;
    for (ForceSpring* f : springsBend) delete f;
}

void SceneCloth::initialize() {

    // load shaders
    shaderPhong = glutils::loadShaderProgram(":/shaders/phong.vert", ":/shaders/phong.frag");
    shaderCloth = glutils::loadShaderProgram(":/shaders/cloth.vert", ":/shaders/cloth.geom", ":/shaders/cloth.frag");

    // create sphere VAOs
    Model sphere = Model::createIcosphere(3);
    vaoSphereL = glutils::createVAO(shaderPhong, &sphere, buffers);
    numFacesSphereL = sphere.numFaces();
    glutils::checkGLError();

    sphere = Model::createIcosphere(1);
    vaoSphereS = glutils::createVAO(shaderPhong, &sphere, buffers);
    numFacesSphereS = sphere.numFaces();
    glutils::checkGLError();

    // create cube VAO
    Model cube = Model::createCube();
    vaoCube = glutils::createVAO(shaderPhong, &cube, buffers);
    glutils::checkGLError();


    // create cloth mesh VAO
    vaoMesh = new QOpenGLVertexArrayObject();
    vaoMesh->create();
    vaoMesh->bind();
    vboMesh = new QOpenGLBuffer(QOpenGLBuffer::Type::VertexBuffer);
    vboMesh->create();
    vboMesh->bind();
    vboMesh->setUsagePattern(QOpenGLBuffer::UsagePattern::DynamicDraw);
    vboMesh->allocate(1000*1000*3*3*sizeof(float)); // sync with widget max particles
    shaderCloth->setAttributeBuffer("vertex", GL_FLOAT, 0, 3, 0);
    shaderCloth->enableAttributeArray("vertex");
    iboMesh = new QOpenGLBuffer(QOpenGLBuffer::Type::IndexBuffer);
    iboMesh->create();
    iboMesh->bind();
    iboMesh->setUsagePattern(QOpenGLBuffer::UsagePattern::StaticDraw);
    iboMesh->allocate(1000*1000*2*3*sizeof(unsigned int));
    vaoMesh->release();

    // create gravity force
    fGravity = new ForceConstAcceleration();
    system.addForce(fGravity);
    colliderParticles.setCellSize(2.0);

    // TODO: in my solution setup, these were the colliders
    colliderBall.setCenter(Vec3(15,-20,0.5));
    colliderBall.setRadius(30);
    colliderCube.setFromCenterSize(Vec3(-60,0,0), Vec3(60, 10, 60));

    kS = widget->getStiffness();
    kD = widget->getDamping();
}

void SceneCloth::reset()
{
    // we only update numParticles on resets
    updateSimParams();

    // reset particles
    system.deleteParticles();

    // reset forces
    system.clearForces();
    fGravity->clearInfluencedParticles();
    for (ForceSpring* f : springsStretch) delete f;
    springsStretch.clear();
    for (ForceSpring* f : springsShear) delete f;
    springsShear.clear();
    for (ForceSpring* f : springsBend) delete f;
    springsBend.clear();

    // cloth props
    Vec2 dims = widget->getDimensions();
    Vec2i dimParticles = widget->getNumParticles();
    numParticlesX = dimParticles.x();
    numParticlesY = dimParticles.y();
    clothWidth  = dims[0];
    clothHeight = dims[1];
    double edgeX = dims[0]/numParticlesX;
    double edgeY = dims[1]/numParticlesY;
    particleRadius = widget->getParticleRadius();

    // create particles
    numParticles = numParticlesX * numParticlesY;
    fixedParticle = std::vector<bool>(numParticles, false);

    for (int i = 0; i < numParticlesX; i++) {
        for (int j = 0; j < numParticlesY; j++) {
            int idx = i * numParticlesY + j;

            // Determine if the particle is fixed
            if (idx < numParticlesY ) {
                fixedParticle[idx] = true;
            } else {
                fixedParticle[idx] = false;
            }

            // Calculate positions
            double tx = i * edgeX - 0.5 * clothWidth;
            double ty = j * edgeY - 0.5 * clothHeight;
            Vec3 pos = Vec3(ty + edgeY, 70 - tx - edgeX, 0);

            // Assuming Particle is instantiated as shown
            Particle* p = new Particle();
            p->id = idx;
            p->pos = pos;
            p->prevPos = pos;
            p->vel = Vec3(0, 0, 0);
            p->mass = 1;
            p->radius = particleRadius;

            p->color = Vec3(230 / 255.0, 51 / 255.0, 36 / 255.0);

            system.addParticle(p);
            fGravity->addInfluencedParticle(p);
        }
    }

    restLengthStretch = edgeX;
    restLengthShear = restLengthStretch * sqrt(2);
    restLengthBend  = restLengthStretch * 2;


    // forces: gravity
    system.addForce(fGravity);

    // Connect particles with springs (both vertical and horizontal)
    for (int i = 0; i < numParticlesX; i++) {
        for (int j = 0; j < numParticlesY; j++) {
            int idx = i * numParticlesY + j;
            Particle* p = system.getParticle(idx);

            // Vertical spring
            if (i < numParticlesX - 1) {
                Particle* p_down = system.getParticle(idx+numParticlesY);
                springsStretch.push_back(new ForceSpring(p,p_down,restLengthStretch,kS,kD));
                system.addForce(springsStretch.back());
            }

            // Horizontal spring
            if (j < numParticlesY - 1) {
                Particle* p_right = system.getParticle(idx + 1);
                springsStretch.push_back(new ForceSpring(p, p_right,restLengthStretch,kS,kD));
                system.addForce(springsStretch.back());
            }

            if (i < numParticlesX - 1) {
                if (j < numParticlesY - 1) {
                    Particle* p_bottom_right = system.getParticle(idx + numParticlesY + 1);
                    springsShear.push_back(new ForceSpring(p, p_bottom_right,restLengthShear,kS,kD));
                    system.addForce(springsShear.back());
                }
                if (j > 0) {
                    Particle* p_bottom_left = system.getParticle(idx + numParticlesY - 1);
                    springsShear.push_back(new ForceSpring(p, p_bottom_left, restLengthShear, kS, kD));
                    system.addForce(springsShear.back());
                }
            }

            // Vertical spring
            if (i < numParticlesX - 2) {
                Particle* p_down_bend = system.getParticle(idx+numParticlesY*2);
                springsBend.push_back(new ForceSpring(p,p_down_bend,restLengthBend,kS,kD)); //FIX PLACEHOLDERS
                system.addForce(springsBend.back());
            }

            // Horizontal spring
            if (j < numParticlesY - 2) {
                Particle* p_right_bend = system.getParticle(idx+2);
                springsBend.push_back(new ForceSpring(p,p_right_bend,restLengthBend,kS,kD));
                system.addForce(springsBend.back());
            }
        }
    }

    // Code for PROVOT layout
    updateSprings();

    // update index buffer
    iboMesh->bind();
    numMeshIndices = (numParticlesX - 1)*(numParticlesY - 1)*2*3;
    int* indices = new int[numMeshIndices];
    int idx = 0;
    for (int i = 0; i < numParticlesX-1; i++) {
        for (int j = 0; j < numParticlesY-1; j++) {
            indices[idx  ] = i*numParticlesY + j;
            indices[idx+1] = (i+1)*numParticlesY + j;
            indices[idx+2] = i*numParticlesY + j + 1;
            indices[idx+3] = i*numParticlesY + j + 1;
            indices[idx+4] = (i+1)*numParticlesY + j;
            indices[idx+5] = (i+1)*numParticlesY + j + 1;
            idx += 6;
        }
    }
    void* bufptr = iboMesh->mapRange(0, numMeshIndices*sizeof(int),
                                     QOpenGLBuffer::RangeInvalidateBuffer | QOpenGLBuffer::RangeWrite);
    memcpy(bufptr, (void*)(indices), numMeshIndices*sizeof(int));
    iboMesh->unmap();
    iboMesh->release();
    delete[] indices;
    glutils::checkGLError();
}


void SceneCloth::updateSprings()
{
    kS = widget->getStiffness();
    kD = widget->getDamping();

    for (ForceSpring* f : springsStretch) {
        f->setSpringConstant(kS);
        f->setDampingCoeff(kD);
    }
    for (ForceSpring* f : springsShear) {
        f->setDampingCoeff(kD);
        f->setSpringConstant(kS);
    }
    for (ForceSpring* f : springsBend) {
        f->setDampingCoeff(kD);
        f->setSpringConstant(kS);
    }
}

void SceneCloth::updateSimParams()
{
    double g = widget->getGravity();
    fGravity->setAcceleration(Vec3(0, -g, 0));

    updateSprings();

    for (Particle* p : system.getParticles()) {
        p->radius = widget->getParticleRadius();
    }

    showParticles = widget->showParticles();
}

void SceneCloth::freeAnchors()
{
    fixedParticle = std::vector<bool>(numParticles, false);
}

void SceneCloth::paint(const Camera& camera)
{
    QOpenGLFunctions* glFuncs = nullptr;
    glFuncs = QOpenGLContext::currentContext()->functions();

    shaderPhong->bind();
    shaderPhong->setUniformValue("normalSign", 1.0f);

    // camera matrices
    QMatrix4x4 camProj = camera.getPerspectiveMatrix();
    QMatrix4x4 camView = camera.getViewMatrix();
    shaderPhong->setUniformValue("ProjMatrix", camProj);
    shaderPhong->setUniformValue("ViewMatrix", camView);

    // lighting
    const int numLights = 1;
    const QVector3D lightPosWorld[numLights] = {QVector3D(80,80,80)};
    const QVector3D lightColor[numLights] = {QVector3D(1,1,1)};
    QVector3D lightPosCam[numLights];
    for (int i = 0; i < numLights; i++) {
        lightPosCam[i] = camView.map(lightPosWorld[i]);
    }
    shaderPhong->setUniformValue("numLights", numLights);
    shaderPhong->setUniformValueArray("lightPos", lightPosCam, numLights);
    shaderPhong->setUniformValueArray("lightColor", lightColor, numLights);

    // draw the particle spheres
    QMatrix4x4 modelMat;
    if (showParticles) {
        vaoSphereS->bind();
        shaderPhong->setUniformValue("matspec", 1.0f, 1.0f, 1.0f);
        shaderPhong->setUniformValue("matshin", 100.f);
        for (int i = 0; i < numParticles; i++) {
            const Particle* particle = system.getParticle(i);
            Vec3   p = particle->pos;
            Vec3   c = particle->color;
            if (fixedParticle[i])      c = Vec3(63/255.0, 72/255.0, 204/255.0);
            if (i == selectedParticle) c = Vec3(1.0,0.9,0);

            modelMat = QMatrix4x4();
            modelMat.translate(p[0], p[1], p[2]);
            modelMat.scale(particle->radius);
            shaderPhong->setUniformValue("ModelMatrix", modelMat);
            shaderPhong->setUniformValue("matdiff", GLfloat(c[0]), GLfloat(c[1]), GLfloat(c[2]));
            glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphereS, GL_UNSIGNED_INT, 0);
        }
    }

    // TODO: draw colliders and walls

    // draw sphere
    vaoSphereL->bind();
    Vec3 cc = colliderBall.getCenter();
    modelMat = QMatrix4x4();
    modelMat.translate(cc[0], cc[1], cc[2]);
    modelMat.scale(colliderBall.getRadius());
    shaderPhong->setUniformValue("ModelMatrix", modelMat);
    shaderPhong->setUniformValue("matdiff", 0.8f, 0.4f, 0.4f);
    shaderPhong->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shaderPhong->setUniformValue("matshin", 0.0f);
    glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphereL, GL_UNSIGNED_INT, 0);

    // draw box
    vaoCube->bind();
    cc = colliderCube.getCenter();
    Vec3 hs = 0.5*colliderCube.getSize();
    modelMat = QMatrix4x4();
    modelMat.translate(cc[0], cc[1], cc[2]);
    modelMat.scale(hs[0], hs[1], hs[2]);
    shaderPhong->setUniformValue("ModelMatrix", modelMat);
    shaderPhong->setUniformValue("matdiff", 0.4f, 0.8f, 0.4f);
    shaderPhong->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shaderPhong->setUniformValue("matshin", 0.0f);
    glFuncs->glDrawElements(GL_TRIANGLES, 3*2*6, GL_UNSIGNED_INT, 0);
    vaoCube->release();

    shaderPhong->release();


    // update cloth mesh VBO coords
    vboMesh->bind();
    float* pos = new float[3*numParticles];
    for (int i = 0; i < numParticles; i++) {
        pos[3*i  ] = system.getParticle(i)->pos.x();
        pos[3*i+1] = system.getParticle(i)->pos.y();
        pos[3*i+2] = system.getParticle(i)->pos.z();
    }
    void* bufptr = vboMesh->mapRange(0, 3*numParticles*sizeof(float),
                       QOpenGLBuffer::RangeInvalidateBuffer | QOpenGLBuffer::RangeWrite);
    memcpy(bufptr, (void*)(pos), 3*numParticles*sizeof(float));
    vboMesh->unmap();
    vboMesh->release();
    delete[] pos;

    // draw mesh
    shaderCloth->bind();
    shaderCloth->setUniformValue("ProjMatrix", camProj);
    shaderCloth->setUniformValue("ViewMatrix", camView);
    shaderCloth->setUniformValue("NormalMatrix", camView.normalMatrix());
    shaderCloth->setUniformValue("matdiffFront", 0.7f, 0.0f, 0.0f);
    shaderCloth->setUniformValue("matspecFront", 1.0f, 1.0f, 1.0f);
    shaderCloth->setUniformValue("matshinFront", 100.0f);
    shaderCloth->setUniformValue("matdiffBack", 0.7f, 0.3f, 0.0f);
    shaderCloth->setUniformValue("matspecBack", 0.0f, 0.0f, 0.0f);
    shaderCloth->setUniformValue("matshinBack", 0.0f);
    shaderCloth->setUniformValue("numLights", numLights);
    shaderCloth->setUniformValueArray("lightPos", lightPosCam, numLights);
    shaderCloth->setUniformValueArray("lightColor", lightColor, numLights);
    vaoMesh->bind();
    glFuncs->glDrawElements(GL_TRIANGLES, numMeshIndices, GL_UNSIGNED_INT, 0);
    vaoMesh->release();
    shaderCloth->release();



    glutils::checkGLError();
}

void SceneCloth::update(double dt)
{
    // fixed particles: no velocity, no force acting
    for (int i = 0; i < numParticles; i++) {
        if (fixedParticle[i]) {
            Particle* p = system.getParticle(i);
            p->vel = Vec3(0,0,0);
            p->force = Vec3(0,0,0);
        }
    }

    Vecd ppos = system.getPositions();
    integrator.step(system, dt);
    system.setPreviousPositions(ppos);

    colliderParticles.particleMap.clear();
    for (Particle *p : system.getParticles())
    {
        colliderParticles.addParticle(p);
    }

    if (selectedParticle >= 0) {
        Collision colInfo;
        Particle* p = system.getParticle(selectedParticle);
        p->pos = p->pos + cursorWorldPos;
        p->vel = Vec3(0, 0, 0);

        for (Particle* p : system.getParticles()) {
            checkAndResolveCollision(p);
        }
    }


    for (int iteration = 0; iteration < 2; iteration++) {
        for (ForceSpring* f : springsStretch) {
            Vec3 displacement = f->getParticle2()->pos - f->getParticle1()->pos;
            double length = displacement.norm();
            float correction_factor = (length - restLengthStretch) / length;

            // Apply position correction for Particle 1 and check for collisions
            if (!fixedParticle[f->getParticle1()->id]) {
                f->getParticle1()->pos += displacement * 0.5 * correction_factor;
                checkAndResolveCollision(f->getParticle1());
            }

            // Apply position correction for Particle 2 and check for collisions
            if (!fixedParticle[f->getParticle2()->id]) {
                f->getParticle2()->pos -= displacement * 0.5 * correction_factor;
                checkAndResolveCollision(f->getParticle2());
            }
        }

        // Repeat for shear springs
        for (ForceSpring* f : springsShear) {
            Vec3 displacement = f->getParticle2()->pos - f->getParticle1()->pos;
            double length = displacement.norm();
            float correction_factor = (length - restLengthShear) / length;

            if (!fixedParticle[f->getParticle1()->id]) {
                f->getParticle1()->pos += displacement * 0.5 * correction_factor;
                checkAndResolveCollision(f->getParticle1());
            }

            if (!fixedParticle[f->getParticle2()->id]) {
                f->getParticle2()->pos -= displacement * 0.5 * correction_factor;
                checkAndResolveCollision(f->getParticle2());
            }
        }

        // Repeat for bend springs
        for (ForceSpring* f : springsBend) {
            Vec3 displacement = f->getParticle2()->pos - f->getParticle1()->pos;
            double length = displacement.norm();
            float correction_factor = (length - restLengthBend) / length;

            if (!fixedParticle[f->getParticle1()->id]) {
                f->getParticle1()->pos += displacement * 0.5 * correction_factor;
                checkAndResolveCollision(f->getParticle1());
            }

            if (!fixedParticle[f->getParticle2()->id]) {
                f->getParticle2()->pos -= displacement * 0.5 * correction_factor;
                checkAndResolveCollision(f->getParticle2());
            }
        }
    }


    system.updateForces();
}

void SceneCloth::checkAndResolveCollision(Particle* p) {
    Collision colInfo;

    // Check for collision with the sphere collider
    if (colliderBall.testCollision(p, colInfo)) {
        colliderBall.resolveCollision(p, colInfo, colBounce, colFriction);
    }
    if (colliderCube.testCollision(p, colInfo)) {
        colliderCube.resolveCollision(p, colInfo, colBounce, colFriction);
    }
    if(colliderParticles.testCollision(p, colInfo)) {
        colliderParticles.resolveCollisionParticles(colInfo, colBounce, colFriction);
    }
}

void SceneCloth::mousePressed(const QMouseEvent* e, const Camera& cam)
{
    grabX = e->pos().x();
    grabY = e->pos().y();

    if (!(e->modifiers() & Qt::ControlModifier)) {

        // Get ray direction and origin from the camera
        Vec3 rayDir = cam.getRayDir(grabX, grabY);  // Ray direction
        Vec3 origin = cam.getPos();                 // Camera position (origin of the ray)

        selectedParticle = -1;
        float selectionThreshold = 10.0f;  // Adjust this based on the scale of your scene
        float minDist = std::numeric_limits<float>::max();  // Track the minimum distance
        int closestParticle = -1;  // Track the closest particle's ID

        // Iterate through all particles
        for (int i = 0; i < numParticles; i++) {
            Particle* p = system.getParticle(i);
            Vec3 particlePos = p->pos;

            // Calculate the vector from ray origin to particle position
            Vec3 originToParticle = particlePos - origin;

            // Find the distance from the particle to the ray
            Vec3 crossProd = originToParticle.cross(rayDir);
            float distToRay = crossProd.norm() / rayDir.norm();

            // Check if the particle is within the selection threshold and closer than the previous particle
            if (distToRay < selectionThreshold && distToRay < minDist) {
                minDist = distToRay;  // Update closest distance
                closestParticle = p->id;  // Update closest particle ID
            }
        }

        // Select the closest particle if found
        selectedParticle = closestParticle;

        if (selectedParticle >= 0) {
            cursorWorldPos = system.getParticle(selectedParticle)->pos;
            std::cout << "Selected particle is: [" << cursorWorldPos[0] << "] [" << cursorWorldPos[1] << "] [" << cursorWorldPos[2] << "] " << std::endl;
        }
    }
}


void SceneCloth::mouseMoved(const QMouseEvent* e, const Camera& cam)
{
    int dx = e->pos().x() - grabX;
    int dy = e->pos().y() - grabY;
    grabX = e->pos().x();
    grabY = e->pos().y();

    if (e->modifiers() & Qt::ControlModifier) {

    }
    else if (e->modifiers() & Qt::ShiftModifier) {

    }
    else {
        if (selectedParticle >= 0) {
            double d = -(system.getParticle(selectedParticle)->pos - cam.getPos()).dot(cam.zAxis());
            Vec3 disp = cam.worldSpaceDisplacement(dx, -dy, d);
            cursorWorldPos += disp;
        }
    }
}

void SceneCloth::mouseReleased(const QMouseEvent*, const Camera&)
{
    selectedParticle = -1;
}

void SceneCloth::keyPressed(const QKeyEvent* e, const Camera&)
{
    if (selectedParticle >= 0 && e->key() == Qt::Key_F) {
        fixedParticle[selectedParticle] = true;
        Particle* p = system.getParticle(selectedParticle);
        p->prevPos = p->pos;
        p->vel = Vec3(0,0,0);
        p->force = Vec3(0,0,0);
    }
}
