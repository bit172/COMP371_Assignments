//
// COMP 371 Assignment Framework
//
// Created by Nicolas Bergeron on 15/7/15.
//         with help from Jordan Rutty
//
// Copyright (c) 2014-2019 Concordia University. All rights reserved.
//

#include "ParticleSystem.h"
#include "ParticleDescriptor.h"
#include "ParticleEmitter.h"
#include "EventManager.h"
#include "World.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/common.hpp>

using namespace glm;


ParticleSystem::ParticleSystem(ParticleEmitter* emitter, ParticleDescriptor* descriptor)
: mpDescriptor(descriptor), mpEmitter(emitter), timeSinceLastParticleEmitted(0.0f)
{
    assert(mpDescriptor != nullptr);
    assert(mpEmitter != nullptr);
    
    // Pre-allocate the maximum number of particles at a give time, according to
    // lifetime and emission rate
    int maxParticles = static_cast<int>(mpDescriptor->emissionRate * (mpDescriptor->totalLifetime + mpDescriptor->totalLifetimeRandomness)) + 1;
    
    mInactiveParticles.resize(maxParticles);
    for (std::list<Particle*>::iterator it = mInactiveParticles.begin(); it != mInactiveParticles.end(); ++it)
    {
        *it = new Particle();
    }
}

ParticleSystem::~ParticleSystem()
{
	for (std::list<Particle*>::iterator it = mParticleList.begin(); it != mParticleList.end(); ++it)
	{
        World::GetInstance()->RemoveBillboard(&(*it)->billboard);
		delete *it;
	}

    for (std::list<Particle*>::iterator it = mInactiveParticles.begin(); it != mInactiveParticles.end(); ++it)
    {
        delete *it;
    }
    
    mParticleList.resize(0);
	mInactiveParticles.resize(0);
}

void ParticleSystem::Update(float dt)
{
    // Emit particle according to the emission rate
    float averageTimeBetweenEmission = 1.0f / mpDescriptor->emissionRate;
//    float randomValue = EventManager::GetRandomFloat(0.0f, averageTimeBetweenEmission);
    timeSinceLastParticleEmitted += dt;
    
    if (timeSinceLastParticleEmitted > averageTimeBetweenEmission && mInactiveParticles.size() > 0)
    {
        timeSinceLastParticleEmitted = 0.0f;
        
        
        // emit particle
        // transfer a particle from the inactive pool to the active pool
        Particle* newParticle = mInactiveParticles.back();
        mParticleList.push_back(newParticle);
        mInactiveParticles.pop_back();
        World::GetInstance()->AddBillboard(&newParticle->billboard);
        
        // Set particle initial parameters
        newParticle->billboard.position = mpEmitter->GetPosition();
        newParticle->billboard.size = mpDescriptor->initialSize + EventManager::GetRandomFloat(-1.0f, 1.0f) * mpDescriptor->initialSizeRandomness;
        newParticle->billboard.color = mpDescriptor->initialColor;
        newParticle->currentTime = 0.0f;
        newParticle->lifeTime = mpDescriptor->totalLifetime + mpDescriptor->totalLifetimeRandomness * EventManager::GetRandomFloat(-1.0f, 1.0f);
        newParticle->velocity = mpDescriptor->velocity;

        newParticle->billboard.angle = mpDescriptor->initialRotationAngle + EventManager::GetRandomFloat(0.0f, mpDescriptor->initialRotationAngleRandomness);
        
        // @TODO 7 - Initial Random Particle Velocity vector
        //
        // Adjust the random velocity according to a random vertical angle variation on a cone
        //
        // Step 1 : You can rotate the velocity vector by a random number between 0 and
        //          mpDescriptor->velocityAngleRandomness.
        // Step 2 : You can rotate the result in step 1 by an random angle from 0 to
        //          360 degrees about the original velocity vector

		float randomAngle = EventManager::GetRandomFloat(0, mpDescriptor->velocityAngleRandomness);
		
		// Create rotation matrix
		mat4 rMatrix = glm::rotate(mat4(1.0f), radians(randomAngle), vec3(1.0f, 1.0f, 1.0f));
		
		// Perform rotation on velocity
		vec3 rotatedVelocity = vec3(rMatrix * vec4(newParticle->velocity, 0));
		
		// Rotate result
		rMatrix = rotate(mat4(1.0f), radians(EventManager::GetRandomFloat(0.0f, 360.0f)), normalize(rotatedVelocity));

		newParticle->velocity = vec3(rMatrix * vec4(rotatedVelocity, 0));

        // ...
    }
    
    
	for (std::list<Particle*>::iterator it = mParticleList.begin(); it != mParticleList.end(); it++)
	{
		Particle* p = *it;
		p->currentTime += dt;
		p->billboard.position += p->velocity * dt;

		// @TODO 6 - Update each particle parameters
		//
		// Update the velocity of the particle from the acceleration in the descriptor
		// Update the size of the particle according to its growth
		// Update The color is also updated in 3 phases
		//
		//
		// Phase 1 - Initial: from t = [0, fadeInTime] - Linearly interpolate between initial color and mid color
		// Phase 2 - Mid:     from t = [fadeInTime, lifeTime - fadeOutTime] - color is mid color
		// Phase 3 - End:     from t = [lifeTime - fadeOutTime, lifeTime]


		// ...

		// Update velocity with acceleration
		p->velocity += mpDescriptor->acceleration * dt;

		// Update size with growth
		p->billboard.size += mpDescriptor->sizeGrowthVelocity * dt;

		// Update color with time
		vec4 color(mpDescriptor->initialColor);

		// Phase 1 - Initial: from t = [0, fadeInTime]
		if (p->currentTime <= mpDescriptor->fadeInTime)
		{
			float delta = (p->currentTime) / (mpDescriptor->fadeInTime);
			color = glm::mix(mpDescriptor->initialColor, mpDescriptor->midColor, delta);
			p->billboard.color = color;

		}
		// Phase 2 - Mid:     from t = [fadeInTime, lifeTime - fadeOutTime]
		else if (p->currentTime > mpDescriptor->fadeInTime
			&& p->currentTime < (mpDescriptor->totalLifetime - mpDescriptor->fadeOutTime))
		{
			color = mpDescriptor->midColor;
			p->billboard.color = color;
		}
		else if (p->currentTime >= (mpDescriptor->totalLifetime - mpDescriptor->fadeOutTime)
			&& p->currentTime <= mpDescriptor->totalLifetime)
		{
			float delta = (p->currentTime - (mpDescriptor->totalLifetime - mpDescriptor->fadeOutTime)) / 
				          (mpDescriptor->totalLifetime - mpDescriptor->fadeOutTime);
			color = glm::mix(mpDescriptor->midColor, mpDescriptor->endColor, delta);
			p->billboard.color = color;
		}
       
        // ...
        
        // Do not touch code below...
        
        // Particles are destroyed if expired
        // Move from the particle to inactive list
        // Remove the billboard from the world
        if (p->currentTime > p->lifeTime)
        {
            mInactiveParticles.push_back(*it);
            
            World::GetInstance()->RemoveBillboard(&(p->billboard));
            mParticleList.remove(*it++);
        }
    }
}
