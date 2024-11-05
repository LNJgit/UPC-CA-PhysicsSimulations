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


    float stretch = distance - L;


    float forceStretch = ks * stretch;


    Vec3 force = (forceStretch + forceDamping) * springDirection;

    p1->force += force;
    p2->force -= force;
}


ForceGravitationalAttractor::ForceGravitationalAttractor()
    : attractorPosition(Vec3(0, 0, 0)), attractorMass(1.0), gravitationalConstant(1.0) {}


ForceGravitationalAttractor::ForceGravitationalAttractor(const Vec3& attractorPos, double attractorMass, double gConstant)
    : attractorPosition(attractorPos), attractorMass(attractorMass), gravitationalConstant(gConstant) {}


ForceGravitationalAttractor::~ForceGravitationalAttractor() {}


void ForceGravitationalAttractor::apply() {
    for (Particle* p : particles) {
        Vec3 direction = attractorPosition - p->pos;
        double distanceSquared = direction.squaredNorm();

        if (distanceSquared > 0.0001) {
            double forceMagnitude = (gravitationalConstant * attractorMass * p->mass) / distanceSquared;
            Vec3 force = forceMagnitude * direction.normalized();

            p->force += force;
        }
    }
}


void ForceGravitationalAttractor::setAttractorPosition(const Vec3& pos) {
    attractorPosition = pos;
}

Vec3 ForceGravitationalAttractor::getAttractorPosition() const {
    return attractorPosition;
}

void ForceGravitationalAttractor::setAttractorMass(double mass) {
    attractorMass = mass;
}

double ForceGravitationalAttractor::getAttractorMass() const {
    return attractorMass;
}

void ForceGravitationalAttractor::setGravitationalConstant(double g) {
    gravitationalConstant = g;
}

double ForceGravitationalAttractor::getGravitationalConstant() const {
    return gravitationalConstant;
}
