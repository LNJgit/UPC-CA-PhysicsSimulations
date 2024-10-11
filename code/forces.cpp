#include "forces.h"
#include <glm/glm.hpp>


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

    // TODO
}
