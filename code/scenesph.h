#ifndef SCENESPH_H
#define SCENESPH_H

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <list>
#include "scene.h"
#include "widgetsph.h"
#include "particlesystem.h"
#include "integrators.h"
#include "colliders.h"


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

public slots:
    void updateSimParams();

protected:
    WidgetSPH* widget = nullptr;

    QOpenGLShaderProgram* shader = nullptr;
    QOpenGLVertexArrayObject* vaoSphereL = nullptr;
    QOpenGLVertexArrayObject* vaoSphereH = nullptr;
    QOpenGLVertexArrayObject* vaoCube    = nullptr;
    QOpenGLVertexArrayObject* vaoFloor   = nullptr;
    unsigned int numFacesSphereL = 0, numFacesSphereH = 0;

    IntegratorEuler integrator;
    ParticleSystem system;
    ForceConstAcceleration* fGravity;

    ColliderPlane colliderFloor;
    ColliderSphere colliderSphere;
    ColliderAABB   colliderBox;
    ColliderParticles colliderParticles;

    float kBounce,kFriction;


    int mouseX, mouseY;
};


#endif // SCENESPH_H
