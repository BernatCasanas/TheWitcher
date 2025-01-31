#pragma once

#include "..\..\Alien Engine\Alien.h"
#include "Macros/AlienScripts.h"

class ALIEN_ENGINE_API RockThrow : public Alien {

public:
	enum(RockType,
		NONE = -1,
		THROW,
		FALL
		);

	float throw_lifetime = 10.0f;
	float throw_timer = 0.0f;
	float3 throw_rotation = { 0.0f, 2.0f, 10.0f };

	bool throwable = false;

	float3 self_rotation = { 0.0f, 5.0f, 2.0f };
	float3 throw_direction;
	float throw_speed = 10.0f;

	int target = 0;

	RockType type = RockType::NONE;

	RockThrow();
	virtual ~RockThrow();
	
	void Start();
	void Update();
	void ReleaseExplosionParticle();
	void OnTriggerEnter(ComponentCollider* collider);

	GameObject* particle_instance = nullptr;

	bool collided = false;

};

ALIEN_FACTORY RockThrow* CreateRockThrow() {
	RockThrow* alien = new RockThrow();
	// To show in inspector here

	return alien;
} 
