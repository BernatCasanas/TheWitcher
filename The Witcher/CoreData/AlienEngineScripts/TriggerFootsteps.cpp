#include "TriggerFootsteps.h"
#include "PlayerController.h"
#include "CameraMovement.h"

TriggerFootsteps::TriggerFootsteps() : Alien()
{
}

TriggerFootsteps::~TriggerFootsteps()
{
}

void TriggerFootsteps::Start()
{
	camera = Camera::GetCurrentCamera()->game_object_attached;
	cam_script = camera->GetComponent<CameraMovement>();
	emitter = game_object->GetComponent<ComponentAudioEmitter>();
}

void TriggerFootsteps::Update()
{
}

void TriggerFootsteps::OnTriggerEnter(ComponentCollider* collider)
{
	ComponentAudioEmitter* emitter = collider->game_object_attached->GetComponent<ComponentAudioEmitter>();

	if (emitter != nullptr)
		emitter->SetSwitchState("Material", GetNameByEnum(material_1).c_str());
}

void TriggerFootsteps::OnTriggerExit(ComponentCollider* collider)
{
	//ComponentAudioEmitter* emitter = (ComponentAudioEmitter*)collider->game_object_attached->GetComponent(ComponentType::A_EMITTER);
	//emitter->SetSwitchState("Material", "Quiet");
}

std::string TriggerFootsteps::GetNameByEnum(GroundMaterial mat)
{
	std::string name;
	switch (mat)
	{
	case GroundMaterial::MUD:
		return name = "Mud";
		break;
	case GroundMaterial::SAND:
		return name = "Sand";
		break;
	case GroundMaterial::STONE:
		return name = "Stone";
		break;
	case GroundMaterial::VEGETATION:
		return name = "Vegetation";
		break;
	case GroundMaterial::WOOD:
		return name = "Wood";
		break;
	}
}
