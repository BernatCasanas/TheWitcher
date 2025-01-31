#include "PlayerController.h"
#include "Pickup.h"
#include "Scores_Data.h"

Pickup::Pickup() : Alien()
{
}

Pickup::~Pickup()
{
}

void Pickup::Start()
{
	start_time = Time::GetGameTime();
	audio_emitter = GetComponent<ComponentAudioEmitter>();
}

void Pickup::Update()
{
	if ((Time::GetGameTime() - start_time) > duration)
		GameObject::Destroy(this->game_object);
}

void Pickup::OnTriggerEnter(ComponentCollider* collider)
{
	if (strcmp(collider->game_object_attached->GetTag(), "Player") == 0) {

		PlayerController* player = collider->game_object_attached->GetComponent<PlayerController>();

		if (player && player->state->type != StateType::DEAD)
		{
			switch (picky) //This was made by the mastermind of V�ctor, not mine Att: Ivan
			{
			case PickUps::HEALTH_ORB:
			{
				player->IncreaseStat(stat_to_change, value);
				player->SpawnParticle("allow_combo_expanse", player->particle_spawn_positions[1]->transform->GetLocalPosition());
				audio_emitter->StartSound("Play_Orb");
				break;
			}
			case PickUps::COIN:
			{
				if (player->controller_index == 1)
				{
					Scores_Data::coin_points_1 += value;
				}
				else {
					Scores_Data::coin_points_2 += value;
				}
				audio_emitter->StartSound("Play_Coin");
				break;
			}
			}
			GameObject::Destroy(this->game_object);
			LOG("PICKUP");
		}
	}
}