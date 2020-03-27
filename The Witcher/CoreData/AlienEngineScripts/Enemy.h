#pragma once

#include "..\..\Alien.h"
#include "Macros/AlienScripts.h"

class Enemy : public Alien {

public: 

	struct EnemyStats {
		std::string weapon = "None";
		float health = 0.0F;
		float agility = 0.0F;
		float damage = 0.0F;
		float attack_speed = 0.0F;
		float attack_range = 0.0F;
		float vision_range = 0.0F;
	};

	enum (EnemyType,
		NONE = -1,
		GHOUL,
		NILFGAARD_SOLDIER
	);

	enum (EnemyState,
		NONE = -1,
		IDLE,
		MOVE,
		ATTACK,
		BLOCK,
		DEAD,
		FLEE,
		);

public:

	Enemy();
	virtual ~Enemy();

	void Start();
	virtual void SetStats(const char* json);
	virtual void Move(float3 direction) {}
	void Update() {}
	virtual void CleaningUp() {}

public:
	EnemyType type = EnemyType::NONE;
	EnemyStats stats;
	EnemyState state = EnemyState::NONE;
	ComponentAnimator* animator = nullptr;
	ComponentRigidBody* rb = nullptr;

	GameObject* player_1 = nullptr;
	GameObject* player_2 = nullptr;
};