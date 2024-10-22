#include "forces.h"
#include <glm/glm.hpp>
#include <iostream>


void ForceConstAcceleration::apply() {
    for (Particle* p : particles) {
        p->force += p->mass * this->acceleration;
    }
}

void ForceDrag::apply() {
    for (Particle* p : particles) {
        double velMagnitude = sqrt((p->vel[0]*p->vel[0]+p->vel[1]*p->vel[1]+p->vel[2]*p->vel[2]));
        p->force += (p->vel * -1.0f * this->klinear) - (0*velMagnitude*p->vel*kquadratic);
    }
}

void ForceSpring::apply() {
    if (particles.size() < 2) return;

    Particle* p1 = getParticle1();
    Particle* p2 = getParticle2();

    Vec3 x1 = p1->pos;
    Vec3 x2 = p2->pos;
    Vec3 v1 = p1->vel;
    Vec3 v2 = p2->vel;

    float distance = (x2 - x1).norm();

    Vec3 relative_velocity = v2 - v1;
    Vec3 springDirection = (x2 - x1).normalized();

    float forceDamping = kd * relative_velocity.dot(springDirection);

    // Calculate the stretch or compression
    float stretch = distance - L;

    // Calculate the spring force vector
    float forceStretch = ks * stretch;

    // Total force (spring + damping)
    Vec3 force = (forceStretch + forceDamping) * springDirection;

    // Apply the forces to the particles
    p1->force += force;    // Apply force to p1
    p2->force -= force;    // Apply equal and opposite force to p2
}
