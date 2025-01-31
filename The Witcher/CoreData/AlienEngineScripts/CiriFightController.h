#pragma once

#include "..\..\Alien Engine\Alien.h"
#include "Macros/AlienScripts.h"

#include "CiriOriginal.h"
#include "Ciri.h"

class ALIEN_ENGINE_API CiriFightController : public Alien {
public:
	bool fight_started = false;
	float ciri_clones_scream_cd = 15.0f;
	float scream_cd_timer = 0.0f;
	bool can_mini_scream = true;
	float phase_0_timer = 0.0f;
	float phase_0_time = 5.0f;
	float phase_4_timer = 0.0f;
	float phase_4_time = 5.0f;
	bool phase_change = false;
	float clone_dead_damage = 12.50f;
	bool died = false;

	int clones_dead = 0;

	int phase = 0;

	int rocks_available = 5;

	std::vector<GameObject*> clone_positions;
	std::vector<GameObject*> rock_positions;
	std::vector<GameObject*> rocks;
	Prefab rock;
	Prefab rock_orbit;

	// Platform
	GameObject* platform = nullptr;
	GameObject* circle = nullptr;
	GameObject* tornado = nullptr;
	GameObject* rocks_respawn = nullptr;
	ComponentMaterial* material_platform = nullptr;
	int count_circle = 58;
	bool changing_platform = false;
	bool desactived_mid_platform = false;
	float rescale_platform_value = 0.1;

	// Wall
	GameObject* wall = nullptr;
	std::vector<GameObject*> rings_enabled;
	std::vector<GameObject*> rings_disabled;
	float3 position_respawn = { 0, 0, 0 };
	bool first_wall_door = true;

	// Rocks
	float throw_time = 0;
	bool rock_throwed = false;
	std::vector<GameObject*> rocks_throwed;

private:
	ComponentAudioEmitter* emitter = nullptr;

public:

	CiriFightController();
	virtual ~CiriFightController();
	
	void Start();
	void Update();

	void UpdatePhaseZero();
	void FinishPhaseZero();
	void UpdatePhaseOne();
	void FinishPhaseOne();
	void UpdatePhaseTwo();
	void FinishPhaseTwo();
	void UpdatePhaseThree();
	void FinishPhaseThree();
	void FinishPhaseFour();
	void UpdatePhaseFour();

	void OnCloneDead(GameObject* clone);

	void MoveWall();
	void ScaleWall();
	void UpdatePlatform();
	void ThrowEnvironmentRocks();
	void TransportPlayer();

	void SpawnRocks();
	void DestroyRocks();

};

ALIEN_FACTORY CiriFightController* CreateCiriFightController() {

	CiriFightController* cirifightcontroller = new CiriFightController();
	SHOW_IN_INSPECTOR_AS_DRAGABLE_FLOAT(cirifightcontroller->ciri_clones_scream_cd);

	SHOW_IN_INSPECTOR_AS_GAMEOBJECT(cirifightcontroller->platform);
	SHOW_IN_INSPECTOR_AS_GAMEOBJECT(cirifightcontroller->wall);
	SHOW_IN_INSPECTOR_AS_PREFAB(cirifightcontroller->rock);
	SHOW_IN_INSPECTOR_AS_PREFAB(cirifightcontroller->rock_orbit);

	return cirifightcontroller;
} 
