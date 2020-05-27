#include "Drowned.h"
#include "MusicController.h"
#include "PlayerController.h"
#include "PlayerAttacks.h"

Drowned::Drowned() : Enemy()
{
}

Drowned::~Drowned()
{
}

void Drowned::StartEnemy()
{
	type = EnemyType::DROWNED;
	state = DrownedState::IDLE;
	m_controller = Camera::GetCurrentCamera()->game_object_attached->GetComponent<MusicController>();
	Enemy::StartEnemy();
}

void Drowned::SetStats(const char* json)
{
	std::string json_path = ENEMY_JSON + std::string(json) + std::string(".json");
	LOG("READING ENEMY STAT GAME JSON WITH NAME %s", json_path.data());

	JSONfilepack* stat = JSONfilepack::GetJSON(json_path.c_str());

	JSONArraypack* stat_weapon = stat->GetArray("Drowned");
	int i = 0;
	if (stat_weapon)
	{
		stat_weapon->GetFirstNode();
		for (int i = 0; i < stat_weapon->GetArraySize(); ++i)
			if (stat_weapon->GetNumber("Type") != (int)drowned_type)
				stat_weapon->GetAnotherNode();
			else
				break;

		stats["Health"] = Stat("Health", stat_weapon->GetNumber("Health"));
		stats["Health"].SetMaxValue(stat_weapon->GetNumber("MaxHealth"));
		stats["Health"].SetMinValue(stat_weapon->GetNumber("MinHealth"));
		stats["Agility"] = Stat("Agility", stat_weapon->GetNumber("Agility"));
		stats["Agility"].SetMaxValue(stat_weapon->GetNumber("MaxAgility"));
		stats["Agility"].SetMinValue(stat_weapon->GetNumber("MinAgility"));
		stats["Damage"] = Stat("Damage", stat_weapon->GetNumber("Damage"));
		stats["Damage"].SetMaxValue(stat_weapon->GetNumber("MaxDamage"));
		stats["Damage"].SetMinValue(stat_weapon->GetNumber("MinDamage"));
		stats["AttackSpeed"] = Stat("AttackSpeed", stat_weapon->GetNumber("AttackSpeed"));
		stats["AttackSpeed"].SetMaxValue(stat_weapon->GetNumber("MaxAttackSpeed"));
		stats["AttackSpeed"].SetMinValue(stat_weapon->GetNumber("MinAttackSpeed"));
		stats["AttackRange"] = Stat("AttackRange", stat_weapon->GetNumber("AttackRange"));
		stats["JumpRange"] = Stat("JumpRange", stat_weapon->GetNumber("JumpAttackRange"));
		stats["VisionRange"] = Stat("VisionRange", stat_weapon->GetNumber("VisionRange"));
		stats["JumpForce"] = Stat("JumpForce", stat_weapon->GetNumber("JumpForce"));
		stats["HitSpeed"] = Stat("HitSpeed", stat_weapon->GetNumber("HitSpeed"));
		stats["HitSpeed"].SetMaxValue(stat_weapon->GetNumber("MaxHitSpeed"));

			stats["GetOffRange"] = Stat("GetOffRange", stat_weapon->GetNumber("GetOffRange"));
	/*	else if (drowned_type == DrownedType::GRAB)
			stats["BlockRange"] = Stat("BlockRange", stat_weapon->GetNumber("BlockRange"));*/

		stat_weapon->GetAnotherNode();
	}

	JSONfilepack::FreeJSON(stat);
}

void Drowned::CleanUpEnemy()
{
	ReleaseAllParticles();
}

float Drowned::GetDamaged(float dmg, PlayerController* player, float3 knock_back)
{
	float damage = Enemy::GetDamaged(dmg, player);

	if (can_get_interrupted || stats["Health"].GetValue() == 0.0F) {
		state = DrownedState::HIT;
		animator->PlayState("Hit");
		stats["HitSpeed"].IncreaseStat(increase_hit_animation);
		animator->SetCurrentStateSpeed(stats["HitSpeed"].GetValue());
		audio_emitter->StartSound("Play_Drowner_Hit");
	}

	if (stats["HitSpeed"].GetValue() == stats["HitSpeed"].GetMaxValue())
	{
		stats["HitSpeed"].SetCurrentStat(stats["HitSpeed"].GetBaseValue());
		animator->SetCurrentStateSpeed(stats["HitSpeed"].GetValue());
		can_get_interrupted = false;
	}

	SpawnParticle("hit_particle", particle_spawn_positions[1]->transform->GetLocalPosition());

	character_ctrl->velocity = PxExtendedVec3(0.0f, 0.0f, 0.0f);

	if (stats["Health"].GetValue() == 0.0F) {

		animator->SetBool("dead", true);
		OnDeathHit();

		if (player->attacks->GetCurrentAttack() && player->attacks->GetCurrentAttack()->IsLast())
		{
			state = DrownedState::DYING;
			audio_emitter->StartSound("Play_Drowner_Hit");

			Decapitate(player);
		}
	}

	return damage;
}

void Drowned::Stun(float time)
{
	if (state != DrownedState::STUNNED && state != DrownedState::DEAD && state != DrownedState::DYING)
	{
		state = DrownedState::STUNNED;
		animator->PlayState("Dizzy");
		audio_emitter->StartSound("Play_Dizzy_Enemy");
		current_stun_time = Time::GetGameTime();
		stun_time = time;
	}
}

bool Drowned::IsDead()
{
	return (state == DrownedState::DEAD ? true : false);
}