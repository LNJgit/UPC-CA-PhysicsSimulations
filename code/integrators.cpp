#include "integrators.h"


void IntegratorEuler::step(ParticleSystem &system, double dt) {
    Vecd x0 = system.getState();
    Vecd dx = system.getDerivative();
    Vecd x1 = x0 + dt*dx;
    system.setState(x1);
}


void IntegratorSymplecticEuler::step(ParticleSystem &system, double dt) {
    Vecd current_velocity = system.getVelocities();
    Vecd next_velocity = current_velocity + system.getAccelerations()*dt;
    Vecd next_position = system.getPositions() + dt*next_velocity;
    system.updateForces();
    system.setPositions(next_position);
    system.setVelocities(next_velocity);

}

void IntegratorRK2::step(ParticleSystem &system, double dt) {
    Vecd x0 = system.getState();
    Vecd k1 = system.getDerivative();
    Vecd x1 = x0 + dt*k1;
    system.setState(x1);
    Vecd k2 = system.getDerivative();
    Vecd x2 = x0 + (dt/2)*(k1+k2);
    system.setState(x2);
}

void IntegratorMidpoint::step(ParticleSystem &system, double dt) {
    Vecd x0 = system.getState();
    Vecd dx = system.getDerivative();
    Vecd xmid = x0 + (dt*dx)/2;
    system.setState(xmid);
    Vecd fmid = system.getDerivative();
    Vecd x1 = x0 + dt*fmid;
    system.setState(x1);


}
void IntegratorVerlet::step(ParticleSystem &system, double dt) {
    // Get current and previous positions, and current accelerations
    Vecd current_position = system.getPositions();
    Vecd previous_position = system.getPreviousPositions();
    Vecd accelerations = system.getAccelerations();

    // Verlet position update
    Vecd next_position = 2 * current_position - previous_position + dt * dt * accelerations;

    // Update previous position for the next iteration
    system.setPreviousPositions(current_position);
    system.setPositions(next_position);

    // Estimate velocity for external collision handling
    Vecd estimated_velocity = (next_position - previous_position) / (2 * dt);
    system.setVelocities(estimated_velocity);

    // Recalculate forces for the next step
    system.updateForces();
}

