#include "GameManager.h"
#include "PlayerManager.h"
#include "PlayerAttacks.h"
#include "EventManager.h"
#include "RelicBehaviour.h"
#include "Effect.h"
#include "CameraMovement.h"
#include "Enemy.h"

#include "../../ComponentDeformableMesh.h"
#include "../../ResourceAnimatorController.h"

#include "UI_Char_Frame.h"
#include "InGame_UI.h"
#include "PlayerController.h"

PlayerController::PlayerController() : Alien()
{
}

PlayerController::~PlayerController()
{
}

void PlayerController::Start()
{
	animator = (ComponentAnimator*)GetComponent(ComponentType::ANIMATOR);
	controller = (ComponentCharacterController*)GetComponent(ComponentType::CHARACTER_CONTROLLER);
	attacks = (PlayerAttacks*)GetComponentScript("PlayerAttacks");

	audio = (ComponentAudioEmitter*)GetComponent(ComponentType::A_EMITTER);

	camera = Camera::GetCurrentCamera();

	ComponentDeformableMesh** vec = nullptr;
	uint size = game_object->GetChild("Meshes")->GetComponentsInChildren(ComponentType::DEFORMABLE_MESH, (Component***)&vec, false);

	max_aabb.SetNegativeInfinity();
	AABB new_section;
	for (uint i = 0u; i < size; ++i) {
		new_section = vec[i]->GetGlobalAABB();
		LOG("AABB: %.2f %.2f %.2f, %.2f %.2f %.2f", new_section.minPoint.x, new_section.minPoint.y, new_section.minPoint.z, new_section.maxPoint.x, new_section.maxPoint.y, new_section.maxPoint.z);
		max_aabb.minPoint = { 
			Maths::Min(new_section.minPoint.x, max_aabb.minPoint.x), Maths::Min(new_section.minPoint.y, max_aabb.minPoint.y),Maths::Min(new_section.minPoint.z, max_aabb.minPoint.z) 
		};
		max_aabb.maxPoint = { 
			Maths::Max(new_section.maxPoint.x, max_aabb.maxPoint.x), Maths::Max(new_section.maxPoint.y, max_aabb.maxPoint.y),Maths::Max(new_section.maxPoint.z, max_aabb.maxPoint.z) 
		};
	}
	max_aabb.minPoint -= transform->GetGlobalPosition();
	max_aabb.maxPoint -= transform->GetGlobalPosition();
	LOG("MAX AABB: %.2f %.2f %.2f, %.2f %.2f %.2f", max_aabb.minPoint.x, max_aabb.minPoint.y, max_aabb.minPoint.z, max_aabb.maxPoint.x, max_aabb.maxPoint.y, max_aabb.maxPoint.z);
	GameObject::FreeArrayMemory((void***)&vec);

	ComponentParticleSystem** p_sys = nullptr;
	size = game_object->GetChild("Particles")->GetComponentsInChildren(ComponentType::PARTICLES, (Component***)&p_sys, false);

	for (uint i = 0u; i < size; ++i) {
		particles.insert(std::pair(p_sys[i]->game_object_attached->GetName(), p_sys[i]->game_object_attached));
		p_sys[i]->game_object_attached->SetEnable(false);
	}

	GameObject::FreeArrayMemory((void***)&p_sys);

	controller->SetRotation(Quat::identity());

	/*std::vector<GameObject*> particle_gos = game_object->GetChild("Particles")->GetChildren();

	for (auto it = particle_gos.begin(); it != particle_gos.end(); ++it) {
		particles.insert(std::pair((*it)->GetName(), (*it)));
		(*it)->SetEnable(false);
	}*/

	if (controller_index == 1) {
		keyboard_move_up = SDL_SCANCODE_W;
		keyboard_move_left = SDL_SCANCODE_A;
		keyboard_move_right = SDL_SCANCODE_D;
		keyboard_move_down = SDL_SCANCODE_S;
		keyboard_jump = SDL_SCANCODE_SPACE;
		keyboard_dash = SDL_SCANCODE_LALT;
		keyboard_light_attack = SDL_SCANCODE_V;
		keyboard_heavy_attack = SDL_SCANCODE_B;
		keyboard_revive = SDL_SCANCODE_C;
		keyboard_ultimate = SDL_SCANCODE_X;
	}
	else if (controller_index == 2) {
		keyboard_move_up = SDL_SCANCODE_I;
		keyboard_move_left = SDL_SCANCODE_J;
		keyboard_move_right = SDL_SCANCODE_L;
		keyboard_move_down = SDL_SCANCODE_K;
		keyboard_jump = SDL_SCANCODE_RSHIFT;
		keyboard_dash = SDL_SCANCODE_RALT;
		keyboard_light_attack = SDL_SCANCODE_RCTRL;
		keyboard_heavy_attack = SDL_SCANCODE_RIGHTBRACKET;
		keyboard_revive = SDL_SCANCODE_M;
		keyboard_ultimate = SDL_SCANCODE_COMMA;
	}
}

void PlayerController::Update()
{
	float2 joystickInput = float2(Input::GetControllerHoritzontalLeftAxis(controller_index), Input::GetControllerVerticalLeftAxis(controller_index));

	animator->SetBool("movement_input", joystickInput.Length() > stick_threshold ? true : false);

	if (Input::GetControllerButtonDown(controller_index, controller_ultimate)
		|| Input::GetKeyDown(keyboard_ultimate)) {
		Game_Manager->player_manager->ultimate_buttons_pressed++;
	}
	else if (Input::GetControllerButtonUp(controller_index, controller_ultimate)
		|| Input::GetKeyUp(keyboard_ultimate)) {
		Game_Manager->player_manager->ultimate_buttons_pressed--;
	}

	if (joystickInput.Length() > 0) {
		if (CheckBoundaries(joystickInput))
			if (can_move)
				HandleMovement(joystickInput);
	}
	else
	{
		float2 keyboardInput = float2(0.f, 0.f);
		if (Input::GetKeyRepeat(keyboard_move_left)) {
			keyboardInput.x += 1.f;
			animator->SetBool("movement_input", true);
		}
		if (Input::GetKeyRepeat(keyboard_move_right)) {
			keyboardInput.x -= 1.f;
			animator->SetBool("movement_input", true);
		}
		if (Input::GetKeyRepeat(keyboard_move_up)) {
			keyboardInput.y += 1.f;
			animator->SetBool("movement_input", true);
		}
		if (Input::GetKeyRepeat(keyboard_move_down)) {
			keyboardInput.y -= 1.f;
			animator->SetBool("movement_input", true);
		}
		if (CheckBoundaries(keyboardInput)) {
			if (can_move) {
				HandleMovement(keyboardInput);
			}
		}
	}

	switch (state)
	{
	case PlayerController::PlayerState::IDLE: {

		can_move = true;
		particles["p_run"]->SetEnable(false);

		if (!controller->OnGround())
		{
			can_move = true;
			state = PlayerState::JUMPING;
			animator->PlayState("Air");
			animator->SetBool("air", true);
		}

		if (Input::GetControllerButtonDown(controller_index, controller_light_attack)
		|| Input::GetKeyDown(keyboard_light_attack)) {
			attacks->StartAttack(PlayerAttacks::AttackType::LIGHT);
			state = PlayerState::BASIC_ATTACK;
			audio->StartSound("Hit_Sword");
			can_move = false;
		}
		/*else if (Input::GetControllerButtonDown(controller_index, controller_heavy_attack)
			|| Input::GetKeyDown(keyboard_heavy_attack)) {
			state = PlayerState::BASIC_ATTACK;
			attacks->StartAttack(PlayerAttacks::AttackType::HEAVY);
			audio->StartSound("Hit_Sword");
			can_move = false;
		}*/

		if (Input::GetControllerButtonDown(controller_index, controller_spell)
			|| Input::GetKeyDown(keyboard_spell)) {
			attacks->StartSpell(0);
			state = PlayerState::CASTING;
		}

		if (Input::GetControllerButtonDown(controller_index, controller_dash)
			|| Input::GetKeyDown(keyboard_dash)) {
			animator->PlayState("Roll");
			state = PlayerState::DASHING;
		}

		if (Input::GetControllerButtonDown(controller_index, controller_revive)
			|| Input::GetKeyDown(keyboard_revive)) {
			if (CheckForPossibleRevive()) {
				controller->SetWalkDirection(float3::zero());
				animator->SetBool("reviving", true);
				state = PlayerState::REVIVING;
			}
		}

		if (Input::GetControllerButtonDown(controller_index, controller_jump)
			|| Input::GetKeyDown(keyboard_jump)) {
			state = PlayerState::JUMPING;
			animator->PlayState("Air");
			if (controller->CanJump()) {
				controller->Jump(transform->up * player_data.jump_power);
				animator->SetBool("air", true);
			}
		}

	} break;
	case PlayerController::PlayerState::RUNNING:
	{
		particles["p_run"]->SetEnable(true);
		can_move = true;

		if (!controller->OnGround())
		{
			can_move = true;
			state = PlayerState::JUMPING;
			animator->PlayState("Air");
			animator->SetBool("air", true);
		}

		if (Time::GetGameTime() - timer >= delay_footsteps) {
			timer = Time::GetGameTime();
			audio->StartSound();
		}

		if (Input::GetControllerButtonDown(controller_index, controller_light_attack)
			|| Input::GetKeyDown(keyboard_light_attack)) {
			attacks->StartAttack(PlayerAttacks::AttackType::LIGHT);
			state = PlayerState::BASIC_ATTACK;
			audio->StartSound("Hit_Sword");
			controller->SetWalkDirection(float3::zero());
			can_move = false;
		}
		/*else if (Input::GetControllerButtonDown(controller_index, controller_heavy_attack)
			|| Input::GetKeyDown(keyboard_heavy_attack)) {
			attacks->StartAttack(PlayerAttacks::AttackType::HEAVY);
			state = PlayerState::BASIC_ATTACK;
			audio->StartSound("Hit_Sword");
			controller->SetWalkDirection(float3::zero());
			can_move = false;
		}*/

		if (Input::GetControllerButtonDown(controller_index, controller_dash)
			|| Input::GetKeyDown(keyboard_dash)) {
			animator->PlayState("Roll");
			state = PlayerState::DASHING;
		}

		if (Input::GetControllerButtonDown(controller_index, controller_revive)
			|| Input::GetKeyDown(keyboard_revive)) {
			if (CheckForPossibleRevive()) {
				controller->SetWalkDirection(float3::zero());
				animator->SetBool("reviving", true);
				state = PlayerState::REVIVING;
			}
		}

		if (Input::GetControllerButtonDown(controller_index, controller_spell)
			|| Input::GetKeyDown(keyboard_spell)) {
			attacks->StartSpell(0);
			state = PlayerState::CASTING;
		}

		if (Input::GetControllerButtonDown(controller_index, controller_jump)
			|| Input::GetKeyDown(keyboard_jump)) {
			state = PlayerState::JUMPING;
			animator->PlayState("Air");
			if (controller->CanJump()) {
				controller->Jump(transform->up * player_data.jump_power);
				animator->SetBool("air", true);
			}
		}

	} break;
	case PlayerController::PlayerState::BASIC_ATTACK:
		particles["p_run"]->SetEnable(false);
		can_move = false;

		if (Input::GetControllerButtonDown(controller_index, controller_light_attack)
			|| Input::GetKeyDown(keyboard_light_attack))
			attacks->ReceiveInput(PlayerAttacks::AttackType::LIGHT);
		/*else if (Input::GetControllerButtonDown(controller_index, controller_heavy_attack)
			|| Input::GetKeyDown(keyboard_heavy_attack))
			attacks->ReceiveInput(PlayerAttacks::AttackType::HEAVY);*/

		attacks->UpdateCurrentAttack();

		if ((Input::GetControllerButtonDown(controller_index, controller_dash)
			|| Input::GetKeyDown(keyboard_dash)) && attacks->CanBeInterrupted()) {
			can_move = true;
			animator->PlayState("Roll");
			state = PlayerState::DASHING;
		}

		if ((Input::GetControllerButtonDown(controller_index, controller_jump)
			|| Input::GetKeyDown(keyboard_jump)) && attacks->CanBeInterrupted()) {
			can_move = true;
			state = PlayerState::JUMPING;
			animator->PlayState("Air");
			if (controller->CanJump()) {
				controller->Jump(transform->up * player_data.jump_power);
				animator->SetBool("air", true);
			}
		}

		break;
	case PlayerController::PlayerState::JUMPING:
		particles["p_run"]->SetEnable(false);
		can_move = true;
		if (controller->CanJump())
			animator->SetBool("air", false);
		break;
	case PlayerController::PlayerState::DASHING:
		particles["p_run"]->SetEnable(false);
		can_move = false;
		break;
	case PlayerController::PlayerState::CASTING:
		can_move = false;
		particles["p_run"]->SetEnable(false);
		attacks->UpdateCurrentAttack();
		break;
	case PlayerController::PlayerState::DEAD:
		can_move = false;
		controller->SetWalkDirection(float3::zero());
		break;
	case PlayerController::PlayerState::REVIVING:
		can_move = false;
		controller->SetWalkDirection(float3::zero());
		break;
	case PlayerController::PlayerState::MAX:
		break;
	case PlayerController::PlayerState::HIT:
		can_move = false;
		controller->SetWalkDirection(float3::zero());
		break;
	default:
		break;
	}

	/*---------------KEYBOARD-----------------------*/

	if (state == PlayerState::RUNNING && abs(player_data.currentSpeed) < 0.05F)
		state = PlayerState::IDLE;

	if (state == PlayerState::IDLE && abs(player_data.currentSpeed) > 0.05F) {
		state = PlayerState::RUNNING;
		audio->StartSound();
		timer = Time::GetGameTime();
	}

	if (state == PlayerState::JUMPING && controller->CanJump()) {
		if (abs(player_data.currentSpeed) < 0.1F)
			state = PlayerState::IDLE;
		if (abs(player_data.currentSpeed) > 0.1F)
			state = PlayerState::RUNNING;
	}
	player_data.currentSpeed = 0;
}

bool PlayerController::AnyKeyboardInput()
{
	return Input::GetKeyDown(keyboard_move_up)
		|| Input::GetKeyDown(keyboard_move_down)
		|| Input::GetKeyDown(keyboard_move_left)
		|| Input::GetKeyDown(keyboard_move_right)
		|| Input::GetKeyDown(keyboard_dash)
		|| Input::GetKeyDown(keyboard_jump);
}

void PlayerController::HandleMovement(const float2& joystickInput)
{
	float joystickIntensity = joystickInput.Length();

	float3 vector = float3(joystickInput.x, 0.f, joystickInput.y);
	vector = Camera::GetCurrentCamera()->game_object_attached->transform->GetGlobalRotation().Mul(vector);
	vector.y = 0.f;
	vector.Normalize();

	float angle = atan2f(vector.z, vector.x);
	Quat rot = Quat::RotateAxisAngle(float3::unitY(), -(angle * Maths::Rad2Deg() - 90.f) * Maths::Deg2Rad());

	if (abs(joystickInput.x) >= stick_threshold || abs(joystickInput.y) >= stick_threshold)
	{
		player_data.currentSpeed = (player_data.movementSpeed * joystickIntensity);
		controller->SetRotation(rot);
	}

	if (state == PlayerState::DASHING)
	{
		controller->SetWalkDirection(transform->forward.Normalized() * player_data.movementSpeed * player_data.dash_power / Time::GetScaleTime());
	}
	else
	{
		controller->SetWalkDirection(vector * player_data.currentSpeed / Time::GetScaleTime());
	}

	animator->SetFloat("speed", Maths::Abs(player_data.currentSpeed));
}

void PlayerController::OnAnimationEnd(const char* name) {
	if (strcmp(name, "Roll") == 0) {
		if(abs(player_data.currentSpeed) < 0.01F)
			state = PlayerState::IDLE;
		if (abs(player_data.currentSpeed) > 0.01F)
			state = PlayerState::RUNNING;
	}

	if (strcmp(name, "Spell") == 0) {
		if (abs(player_data.currentSpeed) < 0.01F)
			state = PlayerState::IDLE;
		if (abs(player_data.currentSpeed) > 0.01F)
			state = PlayerState::RUNNING;
	}

	if (strcmp(name, "Hit") == 0) {
		state = PlayerState::IDLE;
		animator->SetBool("reviving", false);
	}

	if (strcmp(name, "RCP") == 0) {
		ActionRevive();
	}
}

void PlayerController::PlayAttackParticle()
{
	if (attacks->GetCurrentAttack())
	{
		particles[attacks->GetCurrentAttack()->info.particle_name]->SetEnable(false);
		particles[attacks->GetCurrentAttack()->info.particle_name]->SetEnable(true);
	}
}

void PlayerController::Die()
{
	animator->PlayState("Death");
	state = PlayerState::DEAD;
	animator->SetBool("dead", true);
	Game_Manager->event_manager->OnPlayerDead(this);
	controller->SetWalkDirection(float3::zero());
}

void PlayerController::Revive()
{
	state = PlayerState::IDLE;
	animator->SetBool("dead", false);
	animator->PlayState("Revive");
	Game_Manager->event_manager->OnPlayerRevive(this);
	player_data.health.IncreaseStat(player_data.health.max_value * 0.5);
	((UI_Char_Frame*)HUD->GetComponentScript("UI_Char_Frame"))->LifeChange(player_data.health.current_value, player_data.health.max_value);
}

void PlayerController::ActionRevive()
{
	player_being_revived->Revive();
	state = PlayerState::IDLE;
	animator->SetBool("reviving", false);
	player_being_revived = nullptr;
}

void PlayerController::ReceiveDamage(float value)
{
	player_data.health.DecreaseStat(value);
	((UI_Char_Frame*)HUD->GetComponentScript("UI_Char_Frame"))->LifeChange(player_data.health.current_value, player_data.health.max_value);
	if (player_data.health.current_value == 0)
		Die();

	if (state != PlayerState::HIT && state != PlayerState::DASHING && state != PlayerState::DEAD) {
		animator->PlayState("Hit");
		attacks->CancelAttack();
		state = PlayerState::HIT;
		controller->SetWalkDirection(float3::zero());
	}
	
}

void PlayerController::PickUpRelic(Relic* _relic)
{
	relics.push_back(_relic);

	for (int i = 0; i < _relic->effects.size(); i++)
	{
		AddEffect(_relic->effects.at(i));
	}
}

void PlayerController::AddEffect(Effect* _effect)
{
	effects.push_back(_effect);

	//RecalculateStats(); 

	if (dynamic_cast<AttackEffect*>(_effect) != nullptr)
	{
		attacks->OnAddAttackEffect(((AttackEffect*)_effect)->GetAttackIdentifier());
	}
}

bool PlayerController::CheckBoundaries(const float2& joystickInput)
{
	float3 next_pos = float3::zero();
	float joystickIntensity = joystickInput.Length();

	float3 vector = float3(joystickInput.x, 0.f, joystickInput.y);
	vector = Camera::GetCurrentCamera()->game_object_attached->transform->GetGlobalRotation().Mul(vector);
	vector.y = 0.f;
	vector.Normalize();

	float angle = atan2f(vector.z, vector.x);
	Quat rot = Quat::RotateAxisAngle(float3::unitY(), -(angle * Maths::Rad2Deg() - 90.f) * Maths::Deg2Rad());

	float speed = 0.f;

	if (abs(joystickInput.x) >= stick_threshold || abs(joystickInput.y) >= stick_threshold)
	{
		speed = (player_data.movementSpeed * joystickIntensity * Time::GetDT() / Time::GetScaleTime());
	}

	if (state == PlayerState::DASHING)
	{
		next_pos = transform->GetGlobalPosition() + transform->forward.Normalized() * player_data.movementSpeed * player_data.dash_power * Time::GetDT() / Time::GetScaleTime();
	}
	else
	{
		next_pos = transform->GetGlobalPosition() + vector * speed * 3.f;
	}

	float3 moved = (next_pos - transform->GetGlobalPosition());
	AABB fake_aabb(max_aabb.minPoint + moved + transform->GetGlobalPosition(), max_aabb.maxPoint + moved + transform->GetGlobalPosition());

	float3 next_cam_pos = moved.Normalized() * 0.5 + camera->game_object_attached->transform->GetGlobalPosition(); // * 0.5 for middle point in camera

	Frustum fake_frustum = camera->frustum;
	fake_frustum.pos = next_cam_pos;

	CameraMovement* cam = (CameraMovement*)camera->game_object_attached->GetComponentScript("CameraMovement");
	for (int i = 0; i < cam->players.size(); ++i)
	{
		if (cam->players[i] != this->game_object)
		{
			PlayerController* p = (PlayerController*)cam->players[i]->GetComponentScript("PlayerController");
			if (p != nullptr) {
				AABB p_tmp = p->max_aabb;
				p_tmp.minPoint += cam->players[i]->transform->GetGlobalPosition();
				p_tmp.maxPoint += cam->players[i]->transform->GetGlobalPosition();

				if (!fake_frustum.Contains(p_tmp))
				{
					LOG("LEAVING BUDDY BEHIND");
					controller->SetWalkDirection(float3::zero());
					return false;
				}
			}
		}
	}

	if (camera->frustum.Contains(fake_aabb)) {
		return true;
	}
	else {
		controller->SetWalkDirection(float3::zero());
		return false;
	}
}

void PlayerController::OnDrawGizmosSelected()
{
	Gizmos::DrawWireSphere(transform->GetGlobalPosition(), revive_range, Color::Cyan()); //snap_range
}

bool PlayerController::CheckForPossibleRevive()
{
	for (int i = 0; i < Game_Manager->player_manager->players_dead.size(); ++i) {
		float distance = this->transform->GetGlobalPosition().Distance(Game_Manager->player_manager->players_dead[i]->transform->GetGlobalPosition());
		if (distance <= revive_range) {
			player_being_revived = Game_Manager->player_manager->players_dead[i];
			return true;
		}
	}
}

void PlayerController::OnHit(Enemy* enemy, float dmg_dealt)
{
	player_data.total_damage_dealt += dmg_dealt;
	for (auto it = effects.begin(); it != effects.end(); ++it)
	{
		if (dynamic_cast<AttackEffect*>(*it) != nullptr)
		{
			AttackEffect* a_effect = (AttackEffect*)(*it);
			if (a_effect->GetAttackIdentifier() == attacks->GetCurrentAttack()->info.name)
			{
				a_effect->OnHit(enemy);
			}
		}
	}
}

void PlayerController::OnEnemyKill()
{
	player_data.total_kills++;
}

void PlayerController::OnTriggerEnter(ComponentCollider* col)
{
	if (strcmp(col->game_object_attached->GetTag(), "EnemyAttack") == 0 && state != PlayerState::DEAD) {
		Alien** alien = nullptr;
		uint size = col->game_object_attached->parent->GetAllComponentsScript(&alien);
		for (int i = 0; i < size; ++i) {
			Enemy* enemy = dynamic_cast<Enemy*>(alien[i]);
			if (enemy) {
				ReceiveDamage(enemy->stats.damage);
				GameObject::FreeArrayMemory((void***)&alien);
				return;
			}
		}
		GameObject::FreeArrayMemory((void***)&alien);
	}
}

void PlayerController::OnUltimateActivation()
{
	std::vector<State*> states = animator->GetCurrentAnimatorController()->GetStates();
	for (auto it = states.begin(); it != states.end(); ++it)
	{
		(*it)->SetSpeed((*it)->GetSpeed() * Time::GetScaleTime());
	}
}

void PlayerController::OnUltimateDeactivation()
{
	std::vector<State*> states = animator->GetCurrentAnimatorController()->GetStates();
	for (auto it = states.begin(); it != states.end(); ++it)
	{
		(*it)->SetSpeed((*it)->GetSpeed() / 0.5f);
	}
}
