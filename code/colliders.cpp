#include "colliders.h"
#include <cmath>
#include <iostream>


/*
 * Generic function for collision response from contact plane
 */
void Collider::resolveCollision(Particle* p, const Collision& col, double kElastic, double kFriction) const
{
    //We need to reflect the particle velocity.
    Vec3 current_particle_velocity = p->vel;
    double dot_product_normal = current_particle_velocity.dot(col.normal);  //This should be with next velocity not current (FIX)
    Vec3 normal_velocity = dot_product_normal*col.normal;
    Vec3 tangent_velocity = current_particle_velocity-normal_velocity;

    Vec3 new_normal_velocity = -kElastic*normal_velocity;
    Vec3 new_tangent_velocity = (1-kFriction) * tangent_velocity;

    p->vel = (new_normal_velocity+new_tangent_velocity);
    p->pos = col.position;

}



/*
 * Plane
 */
bool ColliderPlane::isInside(const Particle* p) const
{
    Vec3 particle_current_position = p->pos;
    double dot= particle_current_position.dot(this->planeN) +this->planeD;

    if (dot <= 0)
    {
        return true;
    }

    else
    {
        return false;
    }

}


bool ColliderPlane::testCollision(const Particle* p, Collision& colInfo) const
{
    if(this->isInside(p))
    {
        Vec3 particle_velocity = p->vel;
        Vec3 particle_position = p->pos;

        double numerator =  particle_position.dot(this->planeN)+this->planeD;
        double denominator = particle_velocity.dot(this->planeN); //FIX SUM AT END

        double lambda = -numerator/denominator;

        Vec3 collision_pos = particle_position + lambda*particle_velocity;

        colInfo.normal = this->planeN;
        colInfo.position = collision_pos;

        return true;
    }
    else{return false;}
}



/*
 * Sphere
 */
bool ColliderSphere::isInside(const Particle* p) const
{
    // TODO
    return false;
}


bool ColliderSphere::testCollision(const Particle* p, Collision& colInfo) const
{
    // TODO
    return false;
}



/*
 * AABB
 */
bool ColliderAABB::isInside(const Particle* p) const
{
    // TODO
    return false;
}


bool ColliderAABB::testCollision(const Particle* p, Collision& colInfo) const
{
    // TODO
    return false;
}
