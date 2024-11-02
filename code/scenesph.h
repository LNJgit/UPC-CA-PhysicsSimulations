#ifndef SCENESPH_H
#define SCENESPH_H

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_3_3_Core>
#include <list>
#include "scene.h"
#include "widgetsph.h"
#include "particlesystem.h"
#include "integrators.h"
#include "colliders.h"
#include "sph.h"


class SceneSPH : public Scene
{
    Q_OBJECT
public:
    SceneSPH();
    virtual ~SceneSPH();

    virtual void initialize();
    virtual void reset();
    virtual void update(double dt);
    virtual void paint(const Camera& cam);

    virtual void mousePressed(const QMouseEvent* e, const Camera& cam);
    virtual void mouseMoved(const QMouseEvent* e, const Camera& cam);

    virtual void getSceneBounds(Vec3& bmin, Vec3& bmax) {
        bmin = Vec3(-100, -10, -100);
        bmax = Vec3( 100, 100,  100);
    }
    virtual unsigned int getNumParticles() { return system.getNumParticles(); }

    virtual QWidget* sceneUI() { return widget; }
public:
    void initializeCube(int numParticles, double x, double y, double z);
    void drawColliderCube(const ColliderAABB& collider, QOpenGLShaderProgram* shader, QOpenGLFunctions* glFuncs);
    void createColliderContainer(double width, double height, double depth);
    void initializeGhostParticles(double width, double height, double depth);

public slots:
    void updateSimParams();

protected:
    WidgetSPH* widget = nullptr;

    QOpenGLShaderProgram* shader = nullptr;
    QOpenGLVertexArrayObject* vaoSphereL = nullptr;
    QOpenGLVertexArrayObject* vaoSphereH = nullptr;
    QOpenGLVertexArrayObject* vaoCube1    = nullptr;
    QOpenGLVertexArrayObject* vaoCube2    = nullptr;
    QOpenGLVertexArrayObject* vaoCube3    = nullptr;
    QOpenGLVertexArrayObject* vaoFloor   = nullptr;
    unsigned int numFacesSphereL = 0, numFacesSphereH = 0;

    IntegratorSymplecticEuler integrator;
    ParticleSystem system;
    ForceConstAcceleration* fGravity;

    ColliderPlane colliderFloor;
    ColliderSphere colliderSphere;
    ColliderParticles colliderParticles;
    ColliderAABB colliderBox1,colliderBox2,colliderBox3,colliderBox4;

    float kBounce = 0.2;
    float kFriction = 0.0;

    int numInitialParticles=4000;

    double cellSize = 2.0f;
    double smoothingLength = 2.0f;
    double restDensity = 1000.0f;
    double viscosity = 0.001f;
    SPH sph;


    int mouseX, mouseY;
};


#endif // SCENESPH_H
