#include "Enemy.h"
#include "EnemyManager.h"
#include "PlayerController.h"
#include "PlayerAttacks.h"

void Enemy::Awake()
{
	((EnemyManager*)(GameObject::FindWithName("GameManager")->GetComponentScript("EnemyManager")))->AddEnemy(this);
}

void Enemy::StartEnemy()
{
	animator = (ComponentAnimator*)GetComponent(ComponentType::ANIMATOR);
	character_ctrl = (ComponentCharacterController*)GetComponent(ComponentType::CHARACTER_CONTROLLER);
	state = EnemyState::IDLE;
	std::string json_str;

	switch (type)
	{
	case EnemyType::NONE:
		json_str = "None";
		break;
	case EnemyType::GHOUL:
		json_str = "ghoul";
		break;
	case EnemyType::NILFGAARD_SOLDIER:
		json_str = "nilfgaardsoldier";
		break;
	default:
		break;
	}

	SetStats(json_str.data());
}

void Enemy::CleanUpEnemy()
{
	delete animator;
	animator = nullptr;
	delete character_ctrl;
	character_ctrl = nullptr;

	for (auto it_pc = player_controllers.begin(); it_pc != player_controllers.end();)
	{
		delete (*it_pc);
		it_pc = player_controllers.erase(it_pc);
	}
}

void Enemy::SetStats(const char* json)
{
	std::string json_path = std::string("Configuration/") + std::string(json) + std::string(".json");
	LOG("READING ENEMY STAT GAME JSON WITH NAME %s", json_path.data());

	JSONfilepack* stat = JSONfilepack::GetJSON(json_path.c_str());
	if (stat)
	{
		stats.max_health = stats.current_health = stat->GetNumber("Health");
		stats.agility = stat->GetNumber("Agility");
		stats.damage = stat->GetNumber("Damage");
		stats.attack_speed = stat->GetNumber("AttackSpeed");
		stats.attack_range = stat->GetNumber("AttackRange");
		stats.vision_range = stat->GetNumber("VisionRange");
	}

	JSONfilepack::FreeJSON(stat);
}

void Enemy::OnTriggerEnter(ComponentCollider* collider)
{
	if (strcmp(collider->game_object_attached->GetTag(), "PlayerAttack") == 0 && state != EnemyState::DEAD) {
		PlayerController* player = static_cast<PlayerController*>(collider->game_object_attached->GetComponentScriptInParent("PlayerController"));
		float dmg_received = player->attacks->GetCurrentDMG();
		player->OnHit(this, GetDamaged(dmg_received));		

		if (state == EnemyState::DYING)
			player->OnEnemyKill();
	}
}

float Enemy::GetDamaged(float dmg)
{
	float aux_health = stats.current_health;
	stats.current_health -= dmg;

	if (stats.current_health <= 0.0F) {
		stats.current_health = 0.0F;
		state = EnemyState::DYING;
		animator->PlayState("Death");
	}
	else
	{
		state = EnemyState::HIT;
		animator->PlayState("Hit");
		character_ctrl->SetWalkDirection(float3::zero());
	}

	return aux_health - stats.current_health;
}
