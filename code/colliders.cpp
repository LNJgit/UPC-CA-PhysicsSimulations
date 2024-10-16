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
    Vec3 particle_current_position = p->pos;
    Vec3 center_of_sphere = this->getCenter();
    double distance_from_center = (particle_current_position-center_of_sphere).norm();

    if (distance_from_center < this->getRadius())
    {
        return true;
    }
    else{ return false;}
}


bool ColliderSphere::testCollision(const Particle* p, Collision& colInfo) const
{
    if (this->isInside(p))
    {
        Vec3 particle_velocity = p->vel;
        Vec3 particle_position = p->pos;
        Vec3 center_of_sphere = this->getCenter();

        double a = particle_velocity.dot(particle_velocity);
        double b = 2*particle_velocity.dot(particle_position-this->getCenter());
        double c = center_of_sphere.dot(center_of_sphere)+particle_position.dot(particle_position)-2*particle_position.dot(center_of_sphere);
        double discriminant = b * b - 4 * a * c;
        double sqrt_discriminant = std::sqrt(discriminant);
        double t;
        double t1 = (-b - sqrt_discriminant) / (2 * a);
        double t2 = (-b + sqrt_discriminant) / (2 * a);

        if (t1 > 0 || t2 > 0) {
            // Determine the earliest intersection time
            double t = (t1 > 0) ? t1 : t2;
        }

        colInfo.position = particle_position + particle_velocity *t;
        colInfo.normal   = (colInfo.position-center_of_sphere).normalized();
        return true;
    }
    else{return false;}
}



/*
 * AABB
 */
bool ColliderAABB::isInside(const Particle* p) const
{
    Vec3 current_particle_position = p->pos;
    Vec3 max_bound = this->bmax;
    Vec3 min_bound = this->bmin;

    // Check if the particle's position is within bounds on all axes
    return (min_bound[0] < current_particle_position[0] && current_particle_position[0] < max_bound[0]) &&
           (min_bound[1] < current_particle_position[1] && current_particle_position[1] < max_bound[1]) &&
           (min_bound[2] < current_particle_position[2] && current_particle_position[2] < max_bound[2]);
}



bool ColliderAABB::testCollision(const Particle* p, Collision& colInfo) const
{
    // Get the particle's position and velocity
    Vec3 current_position = p->pos;
    Vec3 velocity = p->vel;
    Vec3 min_bound = this->getMin();
    Vec3 max_bound = this->getMax();

    // Initialize the collision normal and collision point
    Vec3 normal(0, 0, 0);
    Vec3 collision_point;

    // If the particle is NOT inside the AABB, return false early
    if (!this->isInside(p)) {
        return false;
    }

    // The particle is inside the AABB, now we check for the closest boundary
    double min_dist = std::numeric_limits<double>::max();
    std::string collidedFace;

    // Check each axis for the closest face and compute the collision point
    for (int i = 0; i < 3; ++i) {
        double distToMin = std::abs(current_position[i] - min_bound[i]);
        double distToMax = std::abs(current_position[i] - max_bound[i]);

        // Determine if the particle is closer to the min or max boundary
        if (distToMin < min_dist) {
            min_dist = distToMin;
            collision_point = current_position;
            collision_point[i] = min_bound[i]; // Set to min boundary
            normal = Vec3(0, 0, 0);
            normal[i] = -1; // Min face normal

            // Log the face hit
            if (i == 0) collidedFace = "Left (X-) face";
            if (i == 1) collidedFace = "Bottom (Y-) face";
            if (i == 2) collidedFace = "Back (Z-) face";
        }

        if (distToMax < min_dist) {
            min_dist = distToMax;
            collision_point = current_position;
            collision_point[i] = max_bound[i]; // Set to max boundary
            normal = Vec3(0, 0, 0);
            normal[i] = 1; // Max face normal

            // Log the face hit
            if (i == 0) collidedFace = "Right (X+) face";
            if (i == 1) collidedFace = "Top (Y+) face";
            if (i == 2) collidedFace = "Front (Z+) face";
        }
    }

    // Set collision information but DO NOT move the particle yet
    colInfo.normal = normal;
    colInfo.position = collision_point;

    // Log the collision point and face
    std::cout << "Collision point: "
              << collision_point[0] << " "
              << collision_point[1] << " "
              << collision_point[2] << std::endl;
    std::cout << "Collided with: " << collidedFace << std::endl;

    return true;
}
