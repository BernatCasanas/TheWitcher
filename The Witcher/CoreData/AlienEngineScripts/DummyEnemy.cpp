#include "DummyEnemy.h"
#include "PlayerAttacks.h"
#include "AttackTrigger.h"
#include "PlayerController.h"
#include "GameManager.h"
#include "RumblerManager.h"


DummyEnemy::DummyEnemy() : Alien()
{
}

DummyEnemy::~DummyEnemy()
{
}


void DummyEnemy::Start()
{
	impactSound = GetComponent<ComponentAudioEmitter>();
}

void DummyEnemy::Update()
{
	if (showing_combo)
	{
		if (time_showing + 2.f <= Time::GetTimeSinceStart())
		{
			DestroyCombo();
		}
	}
	
	if (wiggle)
	{
		wiggleDuration += Time::GetDT();
		if (wiggleDuration > maxWiggleTime)
		{
			wiggle = false;
		}
		perlinNoisePos += 50 * Time::GetDT();
		float angle = DegToRad(maxAngle);
		game_object->transform->SetLocalRotation(Quat::RotateX(Maths::PerlinNoise(0, perlinNoisePos, 0.8f, 0.8f) * angle - (angle * 0.5f)));
	}
}

void DummyEnemy::OnTriggerEnter(ComponentCollider* col)
{
	if (strcmp(col->game_object_attached->GetTag(), "PlayerAttack") == 0)
	{
		
		
		wiggle = true;
		wiggleDuration = 0;
		if (impactSound)
			impactSound->StartSound();
		AttackTrigger* attack_trigger = col->game_object_attached->GetComponent<AttackTrigger>();
		if (attack_trigger)
		{
			if (player_controller == nullptr)
			{
				player_controller = attack_trigger->player;
			}
			
			if (col->game_object_attached->GetComponent<AttackTrigger>()->player == player_controller && current_buttons.size() >= 3)
			{
				DestroyCombo();
			}

			if (col->game_object_attached->GetComponent<AttackTrigger>()->player != player_controller)
			{
				player_controller = col->game_object_attached->GetComponent<AttackTrigger>()->player;
				DestroyCombo();
			}
			
			float position = current_buttons.size() * 5.f - 10.f;

			if (player_controller->attacks->GetCurrentAttack()->info.name.back() == 'L')
			{
				//Input::DoRumble(player_controller->controller_index, 1.f, 120.0f);
				GameManager::instance->rumbler_manager->StartRumbler(RumblerType::LIGHT_ATTACK, player_controller->controller_index);
				current_buttons.push_back(GameObject::Instantiate(button_x, float3(position, 0, 0), false, game_object->GetChild("Combo_UI")));
			}
			else
			{
				//Input::DoRumble(player_controller->controller_index, 1.f, 120.0f);
				GameManager::instance->rumbler_manager->StartRumbler(RumblerType::HEAVY_ATTACK, player_controller->controller_index);
				current_buttons.push_back(GameObject::Instantiate(button_y, float3(position, 0, 0), false, game_object->GetChild("Combo_UI")));
			}

			showing_combo = true;
			time_showing = Time::GetTimeSinceStart();
		}
		
	}
}

//void DummyEnemy::OnCollisionEnter(const Collision& collision)
//{
//	ComponentCollider* col = collision.collider;
//	if (strcmp(col->game_object_attached->GetTag(), "PlayerAttack") == 0)
//	{
//		LOG("Entered ");
//		wiggle = true;
//		if (player == nullptr)
//		{
//			player = col->game_object_attached->GetComponent<AttackTrigger>()->player;
//		}
//
//		if (col->game_object_attached->GetComponent<AttackTrigger>()->player == player)
//		{
//			if (current_buttons.size() >= 5)
//			{
//				DestroyCombo();
//			}
//
//			float position = current_buttons.size() * 5.f - 10.f;
//
//			if (player->attacks->GetCurrentAttack()->info.name.back() == 'L')
//			{
//				current_buttons.push_back(GameObject::Instantiate(button_x, float3(position, 0, 0), false, game_object->GetChild("Combo_UI")));
//			}
//			else
//			{
//				current_buttons.push_back(GameObject::Instantiate(button_y, float3(position, 0, 0), false, game_object->GetChild("Combo_UI")));
//
//			}
//
//			showing_combo = true;
//			time_showing = Time::GetTimeSinceStart();
//		}
//	}
//}

void DummyEnemy::DestroyCombo()
{
	auto iter = current_buttons.begin();
	for (; iter != current_buttons.end(); ++iter)
	{
		(*iter)->Destroy((*iter));
	}
	current_buttons.clear();
	showing_combo = false;
	player_controller = nullptr;
}