#include "GameManager.h"
#include "EventManager.h"
#include "EnemyManager.h"
#include "PlayerManager.h"
#include "PlayerController.h"
#include "RockThrow.h"
#include "RockOrbit.h"
#include "CiriFightController.h"
#include "Scores_Data.h"
#include "RumblerManager.h"
#include "CameraMovement.h"
#include "ParticlePool.h"
#include "UI_DamageCount.h"

CiriFightController::CiriFightController() : Alien()
{
}

CiriFightController::~CiriFightController()
{
	if (material_platform)
		material_platform->material->color = { 1,1,1,1 };
}

void CiriFightController::Start()
{
	clone_positions = game_object->GetChild("ClonePositions")->GetChildren();
	rock_positions = game_object->GetChild("Rock_Positions")->GetChildren();

	emitter = GetComponent<ComponentAudioEmitter>();

	if (platform)
		material_platform = (*platform->GetChildren().begin())->GetComponent<ComponentMaterial>();

	if (wall)
	{
		std::vector<GameObject*> children = wall->GetChildren();
		for (auto it = children.begin(); it != children.end(); ++it)
		{
			if (strcmp((*it)->GetName(), "tube_collider") == 0 || strcmp((*it)->GetName(), "tube_door") == 0)
			{
			}
			else if ((*it)->IsEnabled())
				rings_enabled.push_back(*it);
			else
				rings_disabled.push_back(*it);
		}
		position_respawn = float3(children.back()->transform->GetLocalPosition());
	}

	rocks_respawn = GameObject::FindWithName("Rocks_spawn");
	emitter->StopSoundByName("Play_Ciri_Music");
	emitter->SetSwitchState("Ciri_Boss", "Pre_Fight");
	emitter->StartSound("Play_Ciri_Music");
}

void CiriFightController::Update()
{
	if (scream_cd_timer <= ciri_clones_scream_cd) {
		scream_cd_timer += Time::GetDT();
	}
	else {
		can_mini_scream = true;
	}
	if ((game_object->transform->GetGlobalPosition().Distance(GameManager::instance->player_manager->players[0]->transform->GetGlobalPosition()) < 5 || game_object->transform->GetGlobalPosition().Distance(GameManager::instance->player_manager->players[1]->transform->GetGlobalPosition()) < 5) && !fight_started) {
		fight_started = true;
		phase_change = true;
		tornado = GameManager::instance->particle_pool->GetInstance("ciri_tornado", transform->GetGlobalPosition());
		SpawnRocks();
		emitter->StartSound("Play_Ciri_Tornado");
		GameManager::instance->enemy_manager->CreateEnemy(EnemyType::CIRI_CLONE, clone_positions[0]->transform->GetGlobalPosition());
		GameManager::instance->enemy_manager->CreateEnemy(EnemyType::CIRI_CLONE, clone_positions[1]->transform->GetGlobalPosition());
		emitter->StopSoundByName("Play_Ciri_Music");
		emitter->SetSwitchState("Ciri_Boss", "In_Fight");
		emitter->StartSound("Play_Ciri_Music");
	}
	if (fight_started) {
		switch (phase)
		{
		case 0:
			UpdatePhaseZero();
			break;
		case 1:
			UpdatePhaseOne();
			break;
		case 2:
			UpdatePhaseTwo();
			break;
		case 3:
			UpdatePhaseThree();
			break;
		case 4:
			UpdatePhaseFour();
			break;
		default:
			break;
		}
	}
}

void CiriFightController::UpdatePhaseZero()
{
	if (phase_0_timer <= phase_0_time) {
		phase_0_timer += Time::GetDT();
		this->GetComponent<ComponentCharacterController>()->Move(float3(0, 0.02f, 0));
	}
	else {
		FinishPhaseZero();
	}
}

void CiriFightController::FinishPhaseZero()
{
	phase++;
	this->GetComponent<ComponentCharacterController>()->Move(float3::zero());
}

void CiriFightController::UpdatePhaseOne()
{
}

void CiriFightController::FinishPhaseOne()
{
	phase = 2;
	phase_change = true;
	GameManager::instance->enemy_manager->CreateEnemy(EnemyType::CIRI_CLONE, clone_positions[0]->transform->GetGlobalPosition());
	GameObject::FindWithName("Rock_particles1")->SetEnable(true);
}

void CiriFightController::UpdatePhaseTwo()
{
	MoveWall();
	ThrowEnvironmentRocks();
}

void CiriFightController::FinishPhaseTwo()
{
	changing_platform = false;
	phase = 3;
	phase_change = true;
}

void CiriFightController::UpdatePhaseThree()
{
	MoveWall();
	ThrowEnvironmentRocks();
	if (!first_wall_door)
		UpdatePlatform();
	TransportPlayer();
}

void CiriFightController::FinishPhaseThree()
{
	phase = 4;
	GameManager::instance->particle_pool->ReleaseInstance("ciri_tornado", tornado);
	DestroyRocks();
	GameManager::instance->event_manager->ReceiveDialogueEvent(10, 0.5f);
	GameObject::FindWithName("Butterfly_emitter")->SetEnable(true);
	GameObject::FindWithName("Rock_particles3")->SetEnable(false);
	emitter->StopSoundByName("Play_Ciri_Music");
	emitter->SetSwitchState("Ciri_Boss", "Post_Fight");
	emitter->StartSound("Play_Ciri_Music");
}

void CiriFightController::FinishPhaseFour()
{
	Scores_Data::won_level2 = true;
	GameManager::instance->PrepareDataNextScene(false);
	SceneManager::LoadScene("Final", FadeToBlackType::FADE);
	//Destroy(game_object);
}

void CiriFightController::UpdatePhaseFour()
{
	if (phase_4_timer <= phase_4_time) {
		phase_4_timer += Time::GetDT();
		this->GetComponent<ComponentCharacterController>()->Move(float3(0, -0.02f, 0));
	}
	else {
		if (!died) {
			game_object->GetComponent<ComponentAnimator>()->PlayState("Death");
			died = true;
		}
		if (game_object->transform->GetGlobalPosition().Distance(GameManager::instance->player_manager->players[0]->transform->GetGlobalPosition()) < 5 || game_object->transform->GetGlobalPosition().Distance(GameManager::instance->player_manager->players[1]->transform->GetGlobalPosition()) < 5) {
			FinishPhaseFour();
		}
	}
}

void CiriFightController::OnCloneDead(GameObject* clone)
{
	clones_dead++;
	emitter->StartSound("Play_CiriClone_Death");
	game_object->GetComponent<CiriOriginal>()->GetDamaged(clone_dead_damage);
	if (clones_dead == 2) {
		FinishPhaseOne();
	}
	else if (clones_dead == 5)
		FinishPhaseTwo();
	else if (clones_dead == 8)
		FinishPhaseThree();
	
	if(clones_dead < 6)
		GameManager::instance->enemy_manager->CreateEnemy(EnemyType::CIRI_CLONE, clone_positions[Random::GetRandomIntBetweenTwo(0, 2)]->transform->GetGlobalPosition());
}


void CiriFightController::MoveWall()
{
	if (wall != nullptr)
	{
		wall->transform->AddPosition({ 0, -rescale_platform_value * 60 * Time::GetDT(), 0 });

		if (platform->transform->GetGlobalPosition().y > (*rings_enabled.begin())->transform->GetGlobalPosition().y + 60 && !first_wall_door)
		{
			int random_index = Random::GetRandomIntBetweenTwo(1, 4);

			int i = 1;
			for (auto it = rings_disabled.begin(); it != rings_disabled.end(); ++it)
			{
				if (i == random_index)
				{
					(*it)->SetEnable(true);
					position_respawn = position_respawn + float3(0, 57.5f, 0);
					(*it)->transform->SetLocalPosition(position_respawn);
					rings_enabled.push_back(*it);
					rings_disabled.erase(it);
					break;
				}
				i++;
			}
			(*rings_enabled.begin())->SetEnable(false);
			rings_disabled.push_back(*rings_enabled.begin());
			rings_enabled.erase(rings_enabled.begin());
			changing_platform = true;
		}
		else if (platform->transform->GetGlobalPosition().y > (*rings_enabled.begin())->transform->GetGlobalPosition().y + 60 && first_wall_door)
		{
			first_wall_door = false;
		}
	}
}

void CiriFightController::ScaleWall()
{
	//Escalar wall y collider
	if (wall)
		wall->transform->SetLocalScale(wall->transform->GetLocalScale().x - 0.2, wall->transform->GetLocalScale().y, wall->transform->GetLocalScale().z - 0.2);
}

void CiriFightController::UpdatePlatform()
{
	if (platform != nullptr && !desactived_mid_platform)
	{
		if (changing_platform && circle == nullptr)
		{
			std::vector<GameObject*> children = platform->GetChildren();
			for (auto it = children.begin(); it != children.end(); ++it)
			{
				if (strcmp((*it)->GetName(), "extern_circle") == 0)
				{
					circle = (*it);
					changing_platform = false;
					
					if (GameManager::instance->rumbler_manager)
						GameManager::instance->rumbler_manager->StartRumbler(RumblerType::DECREASING, 0, 2.0);
				}
				else if (strcmp((*it)->GetName(), "mid_circle") == 0)
				{
					if (material_platform)
						material_platform->material->color = { 1,1,1,1 };
					material_platform = (*it)->GetComponent<ComponentMaterial>();
				}
			}
		}
		else if (changing_platform)
		{
			std::vector<GameObject*> children = platform->GetChildren();
			for (auto it = children.begin(); it != children.end(); ++it)
			{
				if (strcmp((*it)->GetName(), "mid_circle") == 0)
				{
					circle->GetComponent<ComponentMaterial>()->material->color = { 1,1,1,1 };
					circle->SetEnable(false);
					circle = (*it);
					desactived_mid_platform = true;
					changing_platform = false;
					ScaleWall();
					if (GameManager::instance->rumbler_manager)
						GameManager::instance->rumbler_manager->StartRumbler(RumblerType::DECREASING, 0, 2.0);
					break;
				}
			}
		}
	}

	if (material_platform)
	{
		material_platform->material->color.x += rescale_platform_value * 0.01;
		material_platform->material->color.y -= rescale_platform_value * 0.01;
		material_platform->material->color.z -= rescale_platform_value * 0.01;
	}

	if (circle)
	{
		circle->transform->SetLocalPosition(circle->transform->GetLocalPosition().x, circle->transform->GetLocalPosition().y - (rescale_platform_value * 2), circle->transform->GetLocalPosition().z);
		GameObject::FindWithName("Rock_particles1")->SetEnable(false);
		
		if (changing_platform)
		{
			if (material_platform)
				material_platform->material->color = { 1,1,1,1 };
			GameObject::FindWithName("Rock_particles3")->SetEnable(true);
			circle->SetEnable(false);
			circle = nullptr;
			ScaleWall();
		}
	}
}

void CiriFightController::ThrowEnvironmentRocks()
{
	throw_time += rescale_platform_value * Time::GetDT() * 60;
	if (((int)throw_time) % 10 == 0 && !rock_throwed)
	{
		float random_x = (float)Random::GetRandomIntBetweenTwo(1, 15);
		float random_z = (float)Random::GetRandomIntBetweenTwo(1, 15);
		float random_index = (float)Random::GetRandomIntBetweenTwo(1, 100) / 100;
		int random_negative = Random::GetRandomIntBetweenTwo(1, 4);
		float3 position = { random_x + random_index, 17, random_z + random_index };
		switch (random_negative)
		{
		case 1:
			break;
		case 2:
			position = { -(random_x + random_index), 17, random_z + random_index };
			break;
		case 3:
			position = { random_x + random_index, 17, -(random_z + random_index) };
			break;
		case 4:
			position = { -(random_x + random_index), 17, -(random_z + random_index) };
			break;
		default:
			break;
		}
		GameObject* rocky = GameObject::Instantiate(rock, position);
		if (rocky->GetComponent<RockThrow>()) {
			rocky->GetComponent<RockThrow>()->type = RockThrow::RockType::FALL;
			rock_throwed = true;
		}
	}
	else if (((int)throw_time) % 10 != 0 && rock_throwed)
		rock_throwed = false;
	
}

void CiriFightController::TransportPlayer()
{
	for (uint i = 0; i < GameManager::instance->player_manager->players.size(); ++i)
	{
		if (platform->transform->GetGlobalPosition().y > GameManager::instance->player_manager->players[i]->transform->GetGlobalPosition().y - 3)
		{
			
			if (GameManager::instance->player_manager->players[i]->controller_index == 1)
			{
				GameManager::instance->player_manager->players[0]->transform->SetGlobalPosition(GameManager::instance->player_manager->players[1]->transform->GetGlobalPosition());
				GameManager::instance->player_manager->players[0]->GetComponent<ComponentCharacterController>()->SetPosition(GameManager::instance->player_manager->players[0]->transform->GetGlobalPosition());
				GameManager::instance->player_manager->players[0]->state->type = StateType::IDLE;
			}
			else
			{
				GameManager::instance->player_manager->players[1]->transform->SetGlobalPosition(GameManager::instance->player_manager->players[0]->transform->GetGlobalPosition());
				GameManager::instance->player_manager->players[1]->GetComponent<ComponentCharacterController>()->SetPosition(GameManager::instance->player_manager->players[1]->transform->GetGlobalPosition());
				GameManager::instance->player_manager->players[1]->state->type = StateType::IDLE;
			}
				

			GameObject::FindWithName("Main Camera")->transform->SetGlobalPosition(GameObject::FindWithName("Main Camera")->GetComponent<CameraMovement>()->CalculateMidPoint() + GameObject::FindWithName("Main Camera")->GetComponent<CameraMovement>()->trg_offset);
			GameManager::instance->player_manager->players[i]->ReceiveDamage(200);
		}
	}
}

void CiriFightController::SpawnRocks()
{
	rocks.clear();

	for (int i = 0; i < 5; ++i) {
		rocks.push_back(GameObject::Instantiate(rock_orbit, float3::zero(), true, rock_positions[i]));
		if (phase > 1) {
			rocks.back()->GetComponent<RockOrbit>()->init_velocity = 0.035f;
		}
	}

	rocks_available = 5;

	game_object->GetChild("Rock_Positions")->transform->SetGlobalPosition(rocks_respawn->transform->GetGlobalPosition());
}

void CiriFightController::DestroyRocks()
{
	for (int i = 0; i < 5; ++i) {
		Destroy(rocks[i]);
	}

	rocks.clear();
}
