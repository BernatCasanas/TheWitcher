#pragma once

#include "..\..\Alien Engine\Alien.h"
#include "Macros/AlienScripts.h"
#include "TriggerCamera.h"

class ALIEN_ENGINE_API CameraMovement : public Alien {
public:
	//enum class CameraState {
	//	NONE = -1,
	//	DYNAMIC, STATIC, MOVING, AXIS,
	//	MAX
	//};
	enum (CameraState,
		DYNAMIC, MOVING_TO_DYNAMIC, 
		FREE, FREE_TO_DYNAMIC,
		STATIC, MOVING_TO_STATIC,
		AXIS, MOVING_TO_AXIS, CINEMATIC, MOVING_TO_CINEMATIC,
		CINEMATIC_TO_AXIS, CINEMATIC_TO_STATIC
		);
	enum (
		CameraAxis,
		NONE,X,Y,Z
	);

	CameraMovement();
	virtual ~CameraMovement();
	
	void Start();
	void Update();
	void OnDrawGizmos();
	void SearchAndAssignPlayers();
	float3 CalculateMidPoint();
	float3 CalculateAxisMidPoint();
	void LookAtMidPoint();
	float3 CalculateCameraPos(const float& vertical, const float& top_view, const float& dst);

	TransitionInfo curr_transition;
	CameraState state = CameraState::DYNAMIC;
	std::vector<GameObject*> players;
	uint num_curr_players = 0u;
	float3 trg_offset;
	float3 first_pos;
	CameraAxis axis = CameraAxis::NONE;
	float current_transition_time = 0.f;

	float distance = 10.f;
	float hor_angle = 0.f;
	float vert_angle = 35.f;
	float transition_time = 0.f;
	bool smooth_camera = true;
	float smooth_cam_vel = 5.f;

	bool search_players = false;

	bool is_cinematic = false;
	GameObject* cutscene_game_object = nullptr;
	//TEST ALLOW MOVEMENT WHEN OTHER IS BLOCKING
	float3 prev_middle = float3::zero();
	int closest_player = -1;
	CameraState prev_state = CameraState::FREE;

	//Limit Camera
	GameObject* limiter1 = nullptr;
	GameObject* limiter2 = nullptr;
};

ALIEN_FACTORY CameraMovement* CreateCameraMovement() {
	CameraMovement* alien = new CameraMovement();
	// To show in inspector here
	SHOW_TEXT("First parameters");
	SHOW_IN_INSPECTOR_AS_ENUM(CameraMovement::CameraState, alien->state);

	SHOW_IN_INSPECTOR_AS_INPUT_FLOAT(alien->distance);
	SHOW_IN_INSPECTOR_AS_INPUT_FLOAT(alien->hor_angle);
	SHOW_IN_INSPECTOR_AS_INPUT_FLOAT(alien->vert_angle);
	SHOW_IN_INSPECTOR_AS_ENUM(CameraMovement::CameraAxis, alien->axis);

	SHOW_IN_INSPECTOR_AS_CHECKBOX_BOOL(alien->smooth_camera);
	SHOW_IN_INSPECTOR_AS_DRAGABLE_FLOAT(alien->smooth_cam_vel);
	
	SHOW_IN_INSPECTOR_AS_CHECKBOX_BOOL(alien->is_cinematic);
	SHOW_IN_INSPECTOR_AS_GAMEOBJECT(alien->cutscene_game_object);

	SHOW_IN_INSPECTOR_AS_CHECKBOX_BOOL(alien->search_players);

	SHOW_SEPARATOR();
	SHOW_IN_INSPECTOR_AS_GAMEOBJECT(alien->limiter1);
	SHOW_IN_INSPECTOR_AS_GAMEOBJECT(alien->limiter2);

	return alien;
}
