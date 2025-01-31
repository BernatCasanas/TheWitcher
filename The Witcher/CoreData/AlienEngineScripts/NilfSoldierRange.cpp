#include "NilfSoldierRange.h"
#include "PlayerController.h"
#include "EnemyManager.h"
#include "ArrowScript.h"
#include "MusicController.h"

NilfSoldierRange::NilfSoldierRange() : NilfgaardSoldier()
{
}

NilfSoldierRange::~NilfSoldierRange()
{
}

void NilfSoldierRange::UpdateEnemy()
{
	Enemy::UpdateEnemy();

	switch (state)
	{
	case NilfgaardSoldierState::IDLE:
		if ((distance > stats["FleeRange"].GetValue() && distance < stats["VisionRange"].GetValue()) || is_obstacle)
			state = NilfgaardSoldierState::MOVE;
		if (distance < stats["FleeRange"].GetValue())
			state = NilfgaardSoldierState::AUXILIAR;
		break;

	case NilfgaardSoldierState::MOVE:
		if (distance > stats["VisionRange"].GetValue())//sorry alex for having this piece of code in a lot of places Att: Ivan
		{
			if (m_controller && is_combat)
			{
				is_combat = false;
				m_controller->EnemyLostSight((Enemy*)this);
			}
		}
		Move(direction);
		if (distance < stats["FleeRange"].GetValue())
			state = NilfgaardSoldierState::AUXILIAR;
		break;

	case NilfgaardSoldierState::ATTACK:
		if (distance > stats["VisionRange"].GetValue())
		{
			if (m_controller && is_combat)
			{
				is_combat = false;
				m_controller->EnemyLostSight((Enemy*)this);
			}
		}
		RotateSoldier();
		break;
	case NilfgaardSoldierState::HIT:
	{
		velocity += velocity * knock_slow * Time::GetDT();
		velocity.y += gravity * Time::GetDT();
		character_ctrl->Move(velocity * Time::GetDT());
	}
	break;
	case NilfgaardSoldierState::AUXILIAR:
	{
		current_flee_distance = transform->GetGlobalPosition().Length();
		if (math::Abs(last_flee_distance - current_flee_distance) < flee_min_distance)
		{
			Action();
		}
		Flee();
	}
	break;

	case NilfgaardSoldierState::STUNNED:
		if (Time::GetGameTime() - current_stun_time > stun_time)
		{
			state = NilfgaardSoldierState::IDLE;
			animator->PlayState("Idle");
			animator->SetBool("stunned", false);
		}
		break;

	case NilfgaardSoldierState::DYING:
	{
		animator->PlayState("Death");
		last_player_hit->OnEnemyKill((uint)type);
		state = NilfgaardSoldierState::DEAD;
		if (m_controller && is_combat)
		{
			is_combat = false;
			m_controller->EnemyLostSight((Enemy*)this);
		}
		if (is_obstacle)
		{
			game_object->parent->parent->GetComponent<BlockerObstacle>()->ReleaseMyself(this);
		}
		break;
	}

	case NilfgaardSoldierState::DEAD:
		IDontWannaGoMrStark();
		break;

	}
}

void NilfSoldierRange::CheckDistance()
{
	if ((distance < stats["AttackRange"].GetValue()) && distance > stats["FleeRange"].GetValue())
	{
		animator->SetFloat("speed", 0.0F);
		character_ctrl->velocity = PxExtendedVec3(0.0f, 0.0f, 0.0f);
		Action();
	}
	else if (distance < stats["FleeRange"].GetValue())
	{
		state = NilfgaardSoldierState::AUXILIAR;
		last_flee_distance = 0.0f;
	}
	else if (distance > stats["VisionRange"].GetValue() && !is_obstacle)
	{
		state = NilfgaardSoldierState::IDLE;
		character_ctrl->velocity = PxExtendedVec3(0.0f, 0.0f, 0.0f);
		animator->SetFloat("speed", 0.0F);
		if (m_controller && !is_combat)
		{
			is_combat = false;
			m_controller->EnemyLostSight((Enemy*)this);
		}
	}

	if (distance < stats["VisionRange"].GetValue())
	{
		if (m_controller && !is_combat)
		{
			is_combat = true;
			m_controller->EnemyInSight((Enemy*)this);
		}
	}
}

void NilfSoldierRange::Action()
{
	animator->PlayState("Shoot");
	animator->SetCurrentStateSpeed(stats["AttackSpeed"].GetValue());
	state = NilfgaardSoldierState::ATTACK;
}

void NilfSoldierRange::Flee()
{
	float3 velocity_vec = -direction * stats["Agility"].GetValue();
	character_ctrl->Move(velocity_vec * Time::GetDT() * Time::GetScaleTime());
	animator->SetFloat("speed", stats["Agility"].GetValue());

	float angle = atan2f(-direction.z, -direction.x);
	Quat rot = Quat::RotateAxisAngle(float3::unitY(), -(angle * Maths::Rad2Deg() - 90.f) * Maths::Deg2Rad());
	transform->SetGlobalRotation(rot);

	if (distance > stats["RecoverRange"].GetValue())
	{
		state = NilfgaardSoldierState::ATTACK;
		character_ctrl->velocity = PxExtendedVec3(0.0f, 0.0f, 0.0f);
		animator->SetFloat("speed", 0.0F);
		Action();
	}

	last_flee_distance = current_flee_distance;
}

void NilfSoldierRange::ShootAttack()
{
	PlayAttackSFX();
	float3 arrow_pos = transform->GetGlobalPosition() + direction.Mul(1).Normalized() + float3(0.0F, 1.0F, 0.0F);
	GameObject* arrow_go = GameObject::Instantiate(arrow, arrow_pos);
	ComponentRigidBody* arrow_rb = arrow_go->GetComponent<ComponentRigidBody>();
	arrow_go->GetComponent<ArrowScript>()->damage = stats["Damage"].GetValue();
	float angle = 10.f * Maths::Deg2Rad();

	int rand = Random::GetRandomIntBetweenTwo(0, 2);
	if (rand != 0) {
		Quat dir = Quat::RotateAxisAngle(float3::unitY(), (rand == 1) ? angle : -angle);
		arrow_rb->SetRotation(RotateProjectile() * dir);
		arrow_rb->AddForce(dir * direction.Mul(20), ForceMode::IMPULSE);
	}
	else {
		arrow_rb->SetRotation(RotateProjectile());
		arrow_rb->AddForce(direction.Mul(20), ForceMode::IMPULSE);
	}
}
