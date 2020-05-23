#include "GameManager.h"
#include "ParticlePool.h"
#include "Ghoul.h"
#include "MusicController.h"
#include "EnemyManager.h"
#include "PlayerAttacks.h"
#include "PlayerController.h"
#include "MusicController.h"


Ghoul::Ghoul() : Enemy()
{
}

Ghoul::~Ghoul()
{
}

void Ghoul::StartEnemy()
{
    type = EnemyType::GHOUL;
    state = GhoulState::AWAKE;
    m_controller = Camera::GetCurrentCamera()->game_object_attached->GetComponent<MusicController>();
    Enemy::StartEnemy();
}

void Ghoul::SetStats(const char* json)
{
    std::string json_path = ENEMY_JSON + std::string(json) + std::string(".json");
    LOG("READING ENEMY STAT GAME JSON WITH NAME %s", json_path.data());

    JSONfilepack* stat = JSONfilepack::GetJSON(json_path.c_str());

    JSONArraypack* stat_weapon = stat->GetArray("Ghoul");
    int i = 0;
    if (stat_weapon)
    {
        stat_weapon->GetFirstNode();
        for (int i = 0; i < stat_weapon->GetArraySize(); ++i)
            if (stat_weapon->GetNumber("Type") != (int)ghoul_type)
                stat_weapon->GetAnotherNode();
            else
                break;

        stats["Health"] = Stat("Health", stat_weapon->GetNumber("MinHealth"), stat_weapon->GetNumber("Health"), stat_weapon->GetNumber("MaxHealth"));
        stats["Agility"] = Stat("Agility", stat_weapon->GetNumber("MinAgility"), stat_weapon->GetNumber("Agility"), stat_weapon->GetNumber("MaxAgility"));
        stats["Damage"] = Stat("Damage", stat_weapon->GetNumber("MinDamage"), stat_weapon->GetNumber("Damage"), stat_weapon->GetNumber("MaxDamage"));
        stats["AttackSpeed"] = Stat("AttackSpeed", stat_weapon->GetNumber("MinAttackSpeed"), stat_weapon->GetNumber("AttackSpeed"), stat_weapon->GetNumber("MaxAttackSpeed"));
        stats["AttackRange"] = Stat("AttackRange", stat_weapon->GetNumber("AttackRange"));
        stats["JumpRange"] = Stat("JumpRange", stat_weapon->GetNumber("JumpAttackRange"));
        stats["VisionRange"] = Stat("VisionRange", stat_weapon->GetNumber("VisionRange"));
        stats["JumpForce"] = Stat("JumpForce", stat_weapon->GetNumber("JumpForce"));
        stats["HitSpeed"] = Stat("HitSpeed", stat_weapon->GetNumber("HitSpeed"));
        stats["HitSpeed"].SetMaxValue(stat_weapon->GetNumber("MaxHitSpeed"));
    }

    JSONfilepack::FreeJSON(stat);
}

void Ghoul::CleanUpEnemy()
{
    ReleaseAllParticles();
}

void Ghoul::JumpImpulse()
{
    if (distance > stats["AttackRange"].GetValue())
    {
        float3 jump_direction = direction * stats["Agility"].GetValue() * stats["JumpForce"].GetValue();
        character_ctrl->Move(jump_direction * Time::GetDT() * Time::GetScaleTime());
    }
    else
    {
        animator->PlayState("Slash");
        animator->SetCurrentStateSpeed(stats["AttackSpeed"].GetValue());
        state = GhoulState::ATTACK;
    }
}

void Ghoul::Stun(float time)
{
    if (state != GhoulState::STUNNED && state != GhoulState::DEAD && state != GhoulState::DYING)
    {
        state = GhoulState::STUNNED;
        animator->PlayState("Dizzy");
        current_stun_time = Time::GetGameTime();
        audio_emitter->StartSound("Play_Dizzy_Enemy");
        stun_time = time;
    }
}

bool Ghoul::IsDead()
{
    return (state == GhoulState::DEAD ? true : false);
}

void Ghoul::SetState(const char* state_str)
{
	if (state_str == "Awake")
		state = GhoulState::AWAKE;
    else if (state_str == "Idle")
        state = GhoulState::IDLE;
    else if (state_str == "Move")
        state = GhoulState::MOVE;
    else if (state_str == "Attack")
        state = GhoulState::ATTACK;
    else if (state_str == "Jump")
        state = GhoulState::JUMP;
    else if (state_str == "Hit")
        state = GhoulState::HIT;
    else if (state_str == "Dying")
        state = GhoulState::DYING;
    else if (state_str == "Dead")
        state = GhoulState::DEAD;
    else if (state_str == "Stunned")
        state = GhoulState::STUNNED;
    else
        LOG("Incorrect state name: %s", state_str);
}

void Ghoul::Action()
{
    // Check if inside range or just entered
    if (distance < stats["AttackRange"].GetValue())
    {
        animator->PlayState("Slash");
        animator->SetCurrentStateSpeed(stats["AttackSpeed"].GetValue());
        state = GhoulState::ATTACK;
    }
    else if (distance < stats["JumpRange"].GetValue() && distance > stats["AttackRange"].GetValue())
    {
        animator->PlayState("Jump");
        state = GhoulState::JUMP;
    }
}

void Ghoul::CheckDistance()
{
	if (state == GhoulState::AWAKE)
		return; 

    if (distance < stats["JumpRange"].GetValue())
    {
        animator->SetFloat("speed", 0.0F);
        character_ctrl->velocity = PxExtendedVec3(0.0f, 0.0f, 0.0f);
        Action();
    }

    if (distance > stats["VisionRange"].GetValue())
    {
        state = GhoulState::IDLE;
        character_ctrl->velocity = PxExtendedVec3(0.0f, 0.0f, 0.0f);
        animator->SetFloat("speed", 0.0F);
        if (m_controller && is_combat) {
            is_combat = false;
            m_controller->EnemyLostSight((Enemy*)this);
        }
    }
    if (distance < stats["VisionRange"].GetValue()) {
        if (m_controller && !is_combat)
        {
            is_combat = true;
            m_controller->EnemyInSight((Enemy*)this);
        }
    }
}

void Ghoul::OnTriggerEnter(ComponentCollider* collider)
{
    if (strcmp(collider->game_object_attached->GetTag(), "PlayerAttack") == 0 && state != GhoulState::DEAD) {
        PlayerController* player = collider->game_object_attached->GetComponentInParent<PlayerController>();
        if (player && player->attacks->GetCurrentAttack()->CanHit(this))
        {
            float dmg_received = player->attacks->GetCurrentDMG();
            float3 knock = (this->transform->GetGlobalPosition() - player->game_object->transform->GetGlobalPosition()).Normalized();
            knock = knock * player->attacks->GetCurrentAttack()->info.stats["KnockBack"].GetValue();

            player->OnHit(this, GetDamaged(dmg_received, player, knock));
            last_player_hit = player;

            HitFreeze(player->attacks->GetCurrentAttack()->info.freeze_time);
        }
    }
}

void Ghoul::OnAnimationEnd(const char* name)
{
    if (strcmp(name, "Slash") == 0) {
        can_get_interrupted = true;
        stats["HitSpeed"].SetCurrentStat(stats["HitSpeed"].GetBaseValue());
        ReleaseParticle("EnemyAttackParticle");
        if (distance < stats["VisionRange"].GetValue() && distance > stats["JumpRange"].GetValue())
        {
            state = GhoulState::MOVE;
        }
        else
        {
            state = GhoulState::IDLE;
        }
    }
    else if (strcmp(name, "Jump") == 0)
    {
        if (distance < stats["VisionRange"].GetValue())
            state = GhoulState::MOVE;
        else
            state = GhoulState::IDLE;
    }
    else if (strcmp(name, "Hit") == 0)
    {
        ReleaseParticle("hit_particle");

        state = GhoulState::IDLE;
    }
}


void Ghoul::OnDrawGizmosSelected()
{
	if(wander_radius > 0.0f)
		Gizmos::DrawWireSphere(start_pos, wander_radius, Color::Blue());
	
}

void Ghoul::PlaySFX(const char* sfx_name)
{
    if (sfx_name == "Hit")
        audio_emitter->StartSound("GhoulHit");
    else if (sfx_name == "Death")
        audio_emitter->StartSound("GhoulDeath");
    else
        LOG("Sound effect with name %s not found!", sfx_name);
}