#pragma once

#include "..\..\Alien Engine\Alien.h"
#include "Macros/AlienScripts.h"

class PlayerAttacks;

class ALIEN_ENGINE_API PlayerController : public Alien {

public:

	enum (PlayerState,
		IDLE,
		RUNNING,
		BASIC_ATTACK,
		JUMPING,
		DASHING,
		CASTING,

		MAX
		);

	struct Stat {
		float max_value, current_value, base_value;
		std::string name;

		Stat(std::string name, float base_value, float max_value, float current_value) { this->name = name; this->base_value = base_value; this->max_value = max_value; this->current_value = current_value; }

		void IncreaseStat(float value);
		void DecreaseStat(float value);
	};

	struct PlayerData {
		float movementSpeed = 200.0F;
		float rotationSpeed = 120.0F;
		float currentSpeed = 0.f;
		float dash_power = 1.5f;
		float jump_power = 25.f;
		Stat health = Stat("Health", 100.0f, 100.0f, 100.0f);
		Stat power = Stat("Strength", 10.0f, 10.0f, 10.0f);
		Stat chaos = Stat("Chaos", 150.0f, 150.0f, 150.0f);
		Stat attack_speed = Stat("Attack Speed", 1.0f, 1.0f, 1.0f);
		//Stat movement_speed = Stat("Movement Speed", 1.0f, 1.0f, 1.0f);
	};

public:
	PlayerController();
	virtual ~PlayerController();

	void Start();
	void Update();

	void HandleMovement(float2 joystickInput);
	void OnAttackEffect();
	void OnAnimationEnd(const char* name);
	void PlaySpell();

public:
	int controller_index = 1;
	PlayerState state = PlayerState::IDLE;
	PlayerAttacks* attacks = nullptr;
	PlayerData player_data;

	ComponentAnimator* animator = nullptr;
	ComponentCharacterController* controller = nullptr;
	bool can_move = false;
	float stick_threshold = 0.1f;
	
	bool keyboard_input = false;

	//Keyboard input
	SDL_Scancode keyboard_move_up;
	SDL_Scancode keyboard_move_left;
	SDL_Scancode keyboard_move_right;
	SDL_Scancode keyboard_move_down;
	SDL_Scancode keyboard_jump;
	SDL_Scancode keyboard_dash;
	SDL_Scancode keyboard_light_attack;
	SDL_Scancode keyboard_heavy_attack;
	SDL_Scancode keyboard_spell;

	//Joystick input
	Input::CONTROLLER_BUTTONS controller_jump = Input::CONTROLLER_BUTTON_A;
	Input::CONTROLLER_BUTTONS controller_dash = Input::CONTROLLER_BUTTON_RIGHTSHOULDER;
	Input::CONTROLLER_BUTTONS controller_light_attack = Input::CONTROLLER_BUTTON_X;
	Input::CONTROLLER_BUTTONS controller_heavy_attack = Input::CONTROLLER_BUTTON_Y;
	Input::CONTROLLER_BUTTONS controller_spell = Input::CONTROLLER_BUTTON_B;


	//Particles
	
	GameObject* p_run = nullptr;
	ComponentParticleSystem* c_run = nullptr;

	GameObject* p_attack = nullptr;
	ComponentParticleSystem* c_attack = nullptr;

	GameObject* p_spell = nullptr;
	ComponentParticleSystem* c_spell = nullptr;

	float delay_footsteps = 0.5f;
private:

	float angle = 0.0f;
	float timer = 0.f;
	ComponentAudioEmitter* audio = nullptr;
};

ALIEN_FACTORY PlayerController* CreatePlayerController() {
	PlayerController* player = new PlayerController();
	// To show in inspector here
	SHOW_IN_INSPECTOR_AS_SLIDER_INT(player->controller_index, 1, 2);
	SHOW_IN_INSPECTOR_AS_DRAGABLE_FLOAT(player->player_data.movementSpeed);
	SHOW_IN_INSPECTOR_AS_DRAGABLE_FLOAT(player->player_data.rotationSpeed);
	SHOW_IN_INSPECTOR_AS_ENUM(PlayerController::PlayerState, player->state);
	SHOW_IN_INSPECTOR_AS_DRAGABLE_FLOAT(player->stick_threshold);
	SHOW_IN_INSPECTOR_AS_DRAGABLE_FLOAT(player->player_data.dash_power);
	SHOW_IN_INSPECTOR_AS_DRAGABLE_FLOAT(player->player_data.jump_power);

	SHOW_IN_INSPECTOR_AS_GAMEOBJECT(player->p_run);
	SHOW_IN_INSPECTOR_AS_GAMEOBJECT(player->p_attack);
	SHOW_IN_INSPECTOR_AS_GAMEOBJECT(player->p_spell);
	SHOW_VOID_FUNCTION(PlayerController::OnAttackEffect, player);
	SHOW_VOID_FUNCTION(PlayerController::PlaySpell, player);

	SHOW_IN_INSPECTOR_AS_SLIDER_FLOAT(player->delay_footsteps, 0.01f, 1.f);

	return player;
}