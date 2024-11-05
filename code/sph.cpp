#include "sph.h"
#include <cmath>
#include <iostream>

SPH::SPH(double cellSize, double smoothingLength)
    : cellSize(cellSize), smoothingLength(smoothingLength) {}

int SPH::getHashValue(Particle* p) const
{
    const int offset = 100000;
    Vec3 position = p->pos;

    int x = static_cast<int>(std::floor(position[0] / cellSize)) + offset;
    int y = static_cast<int>(std::floor(position[1] / cellSize)) + offset;
    int z = static_cast<int>(std::floor(position[2] / cellSize)) + offset;

    int hashKey = ((x * 73856093) ^ (y * 19349663) ^ (z * 83492791)) % 104729;
    return hashKey;
}

void SPH::addParticle(Particle* p)
{
    ParticleSPH particleSPH = { p, 0.0, 0.0, Vec3(0.0, 0.0, 0.0) };
    int hashKey = getHashValue(p);
    particleMap[hashKey].push_back(particleSPH);
    Particles.push_back(particleSPH);
}

double SPH::initializeReferenceDensity() {
    double totalDensity = 0.0;

    for (auto& particle : Particles) {
        getNeighbors(particle);
        computeDensity(particle);
        totalDensity += particle.density;
    }

    // Calculate the average density and set it as the reference density
    double restDensity = totalDensity / Particles.size();
    return restDensity;
}

int SPH::getHashValueForOffset(const Particle* p, int dx, int dy, int dz) const
{
    const int offset = 100000;
    Vec3 particle_position = p->pos;

    int x = static_cast<int>(std::floor(particle_position[0] / cellSize)) + dx + offset;
    int y = static_cast<int>(std::floor(particle_position[1] / cellSize)) + dy + offset;
    int z = static_cast<int>(std::floor(particle_position[2] / cellSize)) + dz + offset;

    int hashKey = ((x * 73856093) ^ (y * 19349663) ^ (z * 83492791)) % 104729;
    return hashKey;
}

void SPH::getNeighbors(ParticleSPH& p)
{
    int hashValue = getHashValue(p.p);
    p.neighbors.clear();
    int neighborRange = 1;

    std::vector<int> neighboringHashes = { hashValue };

    for (int dx = -neighborRange; dx <= neighborRange; ++dx) {
        for (int dy = -neighborRange; dy <= neighborRange; ++dy) {
            for (int dz = -neighborRange; dz <= neighborRange; ++dz) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                int neighborHash = getHashValueForOffset(p.p, dx, dy, dz);
                neighboringHashes.push_back(neighborHash);
            }
        }
    }

    for (int hashKey : neighboringHashes) {
        auto it = particleMap.find(hashKey);
        if (it == particleMap.end()) continue;

        std::vector<ParticleSPH>& particles = it->second;

        for (ParticleSPH& neighbor : particles) {
            Vec3 distanceVector = neighbor.p->pos - p.p->pos;
            float distanceSquared = distanceVector.norm() * distanceVector.norm();
            if (distanceSquared == 0) {
                continue;
            }
            if (distanceSquared <= smoothingLength * smoothingLength) {
                p.neighbors.push_back(&neighbor);
            }
        }
    }
}

void SPH::computeDensity(ParticleSPH& particle) {
    particle.density = 0.0;

    for (ParticleSPH* neighbor : particle.neighbors) {
        double distance = (particle.p->pos - neighbor->p->pos).norm();
        particle.density += neighbor->p->mass * smoothingKernel(smoothingLength, distance) * 1000000;
        particle.p->density = particle.density;
    }
}

double SPH::smoothingKernel(double h, double r) {
    if (r < 0 || r > h) {
        return 0.0;
    }

    double factor = 315.0 / (64.0 * M_PI * pow(h, 9));
    double term = h * h - r * r;
    return factor * term * term * term;
}

double SPH::smoothingKernelDerivative(double r, double h) {
    if (r < 0 || h > r) {
        return 0.0;
    }
    double volume = 3.1415 * pow(r, 8) / 4;
    double value = std::max(0.0, r * r - h * h);
    return -6 * h * value * value / volume;
}

Vec3 SPH::smoothingKernelGradient(const ParticleSPH& p1, const ParticleSPH& p2) {
    Vec3 distanceVector = p2.p->pos - p1.p->pos;
    double h = distanceVector.norm();
    double r = this->smoothingLength;

    if (h == 0.0) return Vec3(0.0, 0.0, 0.0);

    double kernelDerivative = smoothingKernelDerivative(r, h);
    Vec3 gradient = kernelDerivative * (distanceVector / h);

    return gradient;
}

void SPH::computePressure(ParticleSPH& particle, double restDensity, double c_s) {
    particle.pressure = std::abs(c_s * c_s * (particle.density - restDensity));
    particle.p->pressure = particle.pressure;
}

double SPH::viscosityKernel(double r, double h) {
    if (r < 0 || r > h) {
        return 0.0;
    }
    double factor = 15.0 / (2.0 * M_PI * pow(h, 3));
    return factor * (-pow(r, 3) / (2 * pow(h, 3)) + pow(r, 2) / pow(h, 2) + h / (2 * r) - 1);
}

double SPH::viscosityKernelLaplacian(double r, double h) {
    if (r < 0 || r > h) {
        return 0.0;
    }
    return (45.0 / (M_PI * pow(h, 5))) * (1.0 - r / h);
}

void SPH::computePressureForces(ParticleSPH& particle, double viscosity) {
    for (ParticleSPH* neighborPtr : particle.neighbors) {
        ParticleSPH& neighbor = *neighborPtr;
        Vec3 distanceVector = particle.p->pos - neighbor.p->pos;
        double distance = distanceVector.norm();
        if (distance < 0.001) continue;

        // Pressure term
        double pressureAverage = (particle.pressure + neighbor.p->pressure) / 2.0;
        double pressureTerm = -pressureAverage * neighbor.p->mass;

        // Pressure force gradient
        Vec3 gradient = smoothingKernelGradient(particle, neighbor);
        Vec3 pressureForce = pressureTerm * gradient; // Reduced coefficient

        // Viscosity term
        Vec3 relativeVelocity = neighbor.p->vel - particle.p->vel;
        double laplacian = viscosityKernelLaplacian(distance, smoothingLength);
        Vec3 viscosityForce = viscosity * laplacian * relativeVelocity;

        // Total force contribution
        particle.p->force += -pressureForce + viscosityForce;

        // Debug for excessive forces
        if (neighbor.p->ghost && particle.p->force.norm() > 100) {
            std::cout << "Pressure Force: " << pressureForce << "\n";
            std::cout << "Viscosity Force: " << viscosityForce << "\n";
            std::cout << "Total Force: " << particle.p->force << "\n";
        }
    }
}

void SPH::clearParticles()
{
    particleMap.clear();
}

void SPH::setCellSize(int size)
{
    this->cellSize = size;
}

void SPH::setSmoothingLength(float radius)
{
    this->smoothingLength = radius;
}

double SPH::getSmoothingLength()
{
    return smoothingLength;
}



