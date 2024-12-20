#ifndef COLLIDERS_H
#define COLLIDERS_H

#include "defines.h"
#include "particle.h"


struct Collision {
    Vec3 position;
    Vec3 normal;
    const Particle* p1;
    const Particle* p2;
    bool collisionWall;
};


// Abstract interface
class Collider
{
public:
    Collider() {}
    virtual ~Collider() {}

    virtual bool isInside(const Particle* p) const = 0;
    virtual bool testCollision(const Particle* p, Collision& colInfo) const = 0;

    virtual void resolveCollision(Particle* p, const Collision& col, double kElastic, double kFriction) const;
    static void resolveCollisions(Particle* p, const std::vector<Collision>& collisions, double kElastic, double kFriction);
};


// Plane
class ColliderPlane : public Collider
{
public:
    ColliderPlane() { planeN = Vec3(0,0,0); planeD = 0; }
    ColliderPlane(const Vec3& n, double d) : planeN(n), planeD(d) {}
    virtual ~ColliderPlane() {}

    void setPlane(const Vec3& n, double d) { this->planeN = n; this->planeD = d; }

    virtual bool isInside(const Particle* p) const;
    virtual bool testCollision(const Particle* p, Collision& colInfo) const;
    Vec3 getNormal() {return planeN;}
    double getD() {return planeD;}

protected:
    Vec3 planeN;
    double planeD;
};


// Sphere
class ColliderSphere : public Collider
{
public:
    ColliderSphere() { center = Vec3(0,0,0); radius = 0; }
    ColliderSphere(const Vec3& c, double r) : center(c), radius(r) {}
    virtual ~ColliderSphere() {}

    void setCenter(const Vec3& c) { center = c; }
    void setRadius(double r) { radius = r; }
    Vec3 getCenter() const { return center; }
    double getRadius() const { return radius; }

    virtual bool isInside(const Particle* p) const;
    virtual bool testCollision(const Particle* p, Collision& colInfo) const;

protected:
    Vec3 center;
    double radius;
};


class ColliderParticles : public Collider {
public:
    std::unordered_map<int, std::vector<Particle*>> particleMap;
    ColliderParticles() = default;  // Default constructor
    virtual ~ColliderParticles() = default;

    int getHashValue(const Particle* p) const;
    bool isInside(const Particle* p) const;
    void addParticle(Particle *p);
    int getHashValueForOffset(const Particle* p, int dx, int dy, int dz) const;
    bool testCollision(const Particle* p, Collision& colInfo) const;
    bool isWithinRadius(const Vec3& vec1, const Vec3& vec2, double radius) const; // Mark as const
    void setCellSize(double cellSize);

    void resolveCollisionParticles(const Collision& col, double kBounce, double kFriction);

protected:// Assuming this is declared
    double cellSize;
};


// Axis Aligned Bounding Box
class ColliderAABB : public Collider
{
public:
    ColliderAABB() { bmin = bmax = center = size = Vec3(0,0,0); }
    ColliderAABB(const Vec3& bmin, const Vec3& bmax)
        : bmin(bmin), bmax(bmax), center(0.5*(bmin + bmax)), size(bmax - bmin)  {}
    virtual ~ColliderAABB() {}

    void setFromBounds(const Vec3& bmin, const Vec3& bmax) {
        this->bmin = bmin; this->bmax = bmax; center = 0.5*(bmin + bmax); size = bmax - bmin;
    }
    void setFromCenterSize(const Vec3& center, const Vec3& size) {
        this->center = center; this->size = size;
        Vec3 hs = 0.5*size;
        this->bmin = center - hs; this->bmax = center + hs;
    }
    Vec3 getSize() const { return size; }
    Vec3 getCenter() const { return center; }
    Vec3 getMin() const { return bmin; }
    Vec3 getMax() const { return bmax; }

    virtual bool isInside(const Particle* p) const ;
    virtual bool testCollision(const Particle* p, Collision& colInfo) const;

protected:
    Vec3 bmin;
    Vec3 bmax;
    Vec3 center;
    Vec3 size;
};


#endif // COLLIDERS_H
