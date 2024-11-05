#include "colliders.h"
#include <cmath>
#include <iostream>

void Collider::resolveCollisions(Particle* p, const std::vector<Collision>& collisions, double kElastic, double kFriction)
{
    Vec3 totalNormal(0, 0, 0);
    Vec3 totalCorrection(0, 0, 0);

    for (const Collision& col : collisions) {
        totalNormal += col.normal;

        double penetration_depth = p->radius - (p->pos - col.position).dot(col.normal);
        if (penetration_depth > 0) {
            totalCorrection += col.normal * penetration_depth;
        }
    }

    if (totalNormal.norm() > 0) {
        Vec3 normal = totalNormal.normalized();

        Vec3 vel_normal = normal * (p->vel.dot(normal));
        Vec3 vel_tangent = p->vel - vel_normal;

        vel_normal *= -kElastic;
        vel_tangent *= (1 - kFriction);

        p->vel = vel_normal + vel_tangent;
    }

    p->pos += totalCorrection * 0.8; // Reduced position correction to damp oscillations
}

/*
 * Generic function for collision response from contact plane
 */
void Collider::resolveCollision(Particle* p, const Collision& col, double kElastic, double kFriction) const
{
    // Compute the next velocity by reflecting based on the collision normal
    Vec3 current_velocity = p->vel;
    double dot_product_normal = current_velocity.dot(col.normal);
    Vec3 normal_velocity = dot_product_normal * col.normal;
    Vec3 tangent_velocity = current_velocity - normal_velocity;

    // Apply elasticity and friction to get new velocities
    Vec3 new_normal_velocity = -kElastic * normal_velocity;
    Vec3 new_tangent_velocity = (1 - kFriction) * tangent_velocity;

    // Update the particle's velocity
    p->vel = new_normal_velocity + new_tangent_velocity;

    // // Position correction to prevent clipping or excessive bounces
    // double penetration_depth = -(col.normal.dot(p->pos - col.position)) + p->radius;

    // // Soft position correction to avoid high speeds
    // double correction_factor = 0.5; // Adjust this for smoother responses, if needed
    // if (penetration_depth > 0) {
    //     p->pos += col.normal * (penetration_depth * correction_factor);
    // }
}

/*
 * Plane
 */
bool ColliderPlane::isInside(const Particle* p) const
{
    // Calculate the distance from the particle center to the plane
    double distance_to_plane = p->pos.dot(this->planeN) + this->planeD;

    // Use a small offset for early detection (e.g., 10% of particle radius)
    double early_detection_offset = 0.1 * p->radius;

    // Return true if the particle is within its radius + offset from the plane
    return (distance_to_plane <= p->radius + early_detection_offset);
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
    Vec3 particle_position = p->pos;
    Vec3 center_of_sphere = this->getCenter();
    double distance_from_center = (particle_position - center_of_sphere).norm();

    // Use a small offset for early detection
    double early_detection_offset = 0.1 * p->radius;

    // Check if the particle's surface is within the sphere radius plus offset
    return (distance_from_center <= this->getRadius() + p->radius + early_detection_offset);
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
    Vec3 particle_position = p->pos;
    Vec3 max_bound = this->bmax;
    Vec3 min_bound = this->bmin;

    // Early detection offset (e.g., 10% of the particle's radius)
    double early_detection_offset = 0.9 * p->radius;

    // Check if the particle is within its radius + offset from each boundary of the AABB
    return (min_bound[0] - early_detection_offset < particle_position[0] && particle_position[0] < max_bound[0] + early_detection_offset) &&
           (min_bound[1] - early_detection_offset < particle_position[1] && particle_position[1] < max_bound[1] + early_detection_offset) &&
           (min_bound[2] - early_detection_offset < particle_position[2] && particle_position[2] < max_bound[2] + early_detection_offset);
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


    return true;
}


int ColliderParticles::getHashValue(const Particle* p) const
{
    Vec3 particle_position = p->pos;
    int x = static_cast<int>(std::floor(particle_position[0] / cellSize));
    int y = static_cast<int>(std::floor(particle_position[1] / cellSize));
    int z = static_cast<int>(std::floor(particle_position[2] / cellSize));

    int hashKey = x * 73856093 ^ y * 19349663 ^ z * 83492791;
    return hashKey;
}

bool ColliderParticles::isInside(const Particle *p) const {
    if (p == nullptr) {
        return false; // Handle null pointer
    }

    int hashValue = getHashValue(p);

    // Check if the hashValue exists in the particleMap
    auto it = particleMap.find(hashValue);
    if (it != particleMap.end()) {
        const auto& particles = it->second; // Access the vector using iterator

        // Iterate through the particles to check for collisions or conditions
        for (const auto& particle : particles) {
            // Check if the current particle is the same as particle p
            if (particle == p) {
                continue; // Skip self-collision check
            }

            const Vec3& pos1 = p->pos; // Use const reference to avoid copies
            const Vec3& pos2 = particle->pos; // Use const reference


            double radius = p->radius; // Assuming p has a radius member

            if (isWithinRadius(pos1, pos2, radius)) {
                return true; // Found a collision or condition
            }
        }
    }

    return false; // Return false if no collision found
}


bool ColliderParticles::testCollision(const Particle* p, Collision& colInfo) const {
    if (p == nullptr) {
        return false; // Handle null pointer
    }

    int hashValue = getHashValue(p);

    // Check if the hashValue exists in the particleMap
    auto it = particleMap.find(hashValue);
    if (it != particleMap.end()) {
        auto& particles = it->second; // Access the vector using iterator

        // Iterate through the particles to check for collisions or conditions
        for (auto& particle : particles) {
            // Check if the current particle is the same as particle p
            if (particle == p) {
                continue; // Skip self-collision check
            }

            const Vec3& pos1 = p->pos; // Use const reference to avoid copies
            const Vec3& pos2 = particle->pos; // Use const reference


            double radius = p->radius; // Assuming p has a radius member

            if (isWithinRadius(pos1, pos2, radius)) {
                colInfo.position = (pos1 + pos2) * 0.5; // Midpoint as collision position
                colInfo.normal = (pos1 - pos2).normalized(); // Normal vector
                colInfo.p1 = p;
                colInfo.p2 = particle;
                return true; // Found a collision or condition
            }
        }
    }

    return false; // Return false if no collision found
}


void ColliderParticles::addParticle(Particle *p) {
    int hashValue = getHashValue(p);
    particleMap[hashValue].push_back(p); // This assumes particleMap is properly initialized
}

bool ColliderParticles::isWithinRadius(const Vec3& vec1, const Vec3& vec2, double radius) const { // Add const here
    Vec3 difference = vec1 - vec2;
    return difference.norm() < 2*radius; // Check for collision
}

#include <random>  // for random number generation

void ColliderParticles::resolveCollisionParticles(const Collision& col, double kBounce, double kFriction)
{
    Particle* p1 = const_cast<Particle*>(col.p1);
    Particle* p2 = const_cast<Particle*>(col.p2);

    // Relative velocity between particles
    Vec3 relative_velocity = p1->vel - p2->vel;

    // Normal vector from p1 to p2
    Vec3 collision_normal = (p1->pos - p2->pos).normalized();

    // Velocity component along the normal
    double velocity_along_normal = relative_velocity.dot(collision_normal);

    // If velocity is separating, no collision to resolve
    if (velocity_along_normal > 0) {
        return;
    }

    // Restitution (elasticity factor)
    double restitution = kBounce;

    // Tangential (frictional) component of velocity
    Vec3 tangent_velocity = relative_velocity - (velocity_along_normal * collision_normal);

    // Apply friction to tangential velocity (reducing its magnitude)
    Vec3 friction_impulse = -(1 - kFriction) * tangent_velocity;

    // Calculate normal impulse (same as before)
    double normal_impulse_magnitude = -(1 + restitution) * velocity_along_normal;
    Vec3 normal_impulse = normal_impulse_magnitude * collision_normal;

    // Update velocities of both particles
    p1->vel += (normal_impulse + friction_impulse);
    p2->vel -= (normal_impulse + friction_impulse);

    // --- Position Correction with Conditional Z-Axis Offset ---

    // Calculate the overlap distance (penetration depth)
    double penetration_depth = p1->radius + p2->radius - (p1->pos - p2->pos).norm();

    // If there is penetration, resolve it by moving the particles apart
    if (penetration_depth > 0) {
        double correction_factor = 0.5;  // Could be adjusted for different mass particles

        // Position correction along the normal
        Vec3 correction = penetration_depth * correction_factor * collision_normal;

        p1->pos += correction;
        p2->pos -= correction;
    }
}



void ColliderParticles::setCellSize(double cellSize)
{
    this->cellSize = 2;
}
