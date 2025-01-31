#include "AttackTrigger.h"
#include "SoundMaterial.h"
#include "PlayerController.h"
#include "PlayerAttacks.h"
#include "Enemy.h"

AttackTrigger::AttackTrigger()
{

}

AttackTrigger::~AttackTrigger()
{
}

void AttackTrigger::Update()
{
	if (rbdy)
	{
		rbdy->SetPosition(game_object->parent->transform->GetGlobalPosition());
		rbdy->SetRotation(game_object->parent->transform->GetGlobalRotation());
		transform->SetLocalScale({ 1,1,1 });
	}
		
}

void AttackTrigger::OnTriggerEnter(ComponentCollider* collider)
{
	if (!player || !player->attacks->GetCurrentAttack())
	{
		return;
	}

	//Here we will be able to get the audio material and play the sound of the surface we hit
	SoundMaterial* s_material = collider->game_object_attached->GetComponent<SoundMaterial>();
	if (s_material && player->attacks->GetCurrentAttack()->CanHit(collider->game_object_attached))
	{
		player->audio->SetSwitchState("Material", s_material->GetMaterialName().c_str());
		player->audio->StartSound("Play_Impact");
		if (strcmp(collider->game_object_attached->GetTag(), "Enemy") != 0)
			player->attacks->GetCurrentAttack()->enemies_hit.push_back(collider->game_object_attached);
	}

	if (strcmp(collider->game_object_attached->GetTag(), "Enemy") == 0)
	{
		Enemy* enemy = collider->game_object_attached->GetComponent<Enemy>();
		if(enemy && !enemy->IsDead() && !enemy->is_immune)
		{
			if (player->attacks->GetCurrentAttack()->CanHit(collider->game_object_attached))
			{
				float damage = player->attacks->GetCurrentDMG();
				float3 knock = enemy->can_get_interrupted ? player->attacks->GetKnockBack(enemy->transform) : float3::zero();

				float damage_dealt = enemy->GetDamaged(damage, player, knock);
				player->OnHit(enemy, damage_dealt);
			}
		}
	}
}

void AttackTrigger::Start()
{
	if (!player_obj)
	{
		LOG("Error: No Player found on AttackTrigger");
		return;
	}

	if (player_obj)
		player = player_obj->GetComponent<PlayerController>();

	rbdy = GetComponent<ComponentRigidBody>();
}
