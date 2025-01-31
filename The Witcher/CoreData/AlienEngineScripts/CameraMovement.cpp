#include "CameraMovement.h"
#include "PlayerController.h"
#include "CutsceneCamera.h"

CameraMovement::CameraMovement() : Alien()
{
}

CameraMovement::~CameraMovement()
{
}

void CameraMovement::Start()
{
    //Get the players in the scene
    SearchAndAssignPlayers();
    trg_offset = CalculateCameraPos(hor_angle, vert_angle, distance);
    transform->SetGlobalPosition(CalculateMidPoint() + trg_offset);
    first_pos = CalculateMidPoint() + trg_offset;
    LookAtMidPoint();
    prev_state = CameraState::FREE;
}

void CameraMovement::Update()
{
    switch (state) {
    case CameraState::DYNAMIC: {
        float3 pos = transform->GetGlobalPosition();
        float3 futurePos = pos + ((CalculateMidPoint() + trg_offset) - pos) * smooth_cam_vel * Time::GetDT();
        if (limiter1 != nullptr && limiter2 != nullptr) {
            float3 l1 = limiter1->transform->GetGlobalPosition();
            float3 l2 = limiter2->transform->GetGlobalPosition();
            float d = (futurePos.x - l1.x) * (l2.z - l1.z) - (futurePos.z - l1.z) * (l2.x - l1.x);
            if (d < 0) {
                if (smooth_camera) {
                    transform->SetGlobalPosition(futurePos);
                }
                else
                    transform->SetGlobalPosition(CalculateMidPoint() + trg_offset);
            }
            else {
                float2 vec = float2((l2 - l1).x, (l2 - l1).z);
                float2 proj = vec * ((float2(futurePos.x, futurePos.z) - float2(l1.x, l1.z)).Dot(vec)) / (vec.Dot(vec));
                transform->SetGlobalPosition(float3(l1.x + proj.x, futurePos.y, l1.z + proj.y));
            }
        }
        else {
            if (smooth_camera) {
                transform->SetGlobalPosition(futurePos);
            }
            else
                transform->SetGlobalPosition(CalculateMidPoint() + trg_offset);
        }
        break;
    }
    case CameraState::FREE: {
            float3 pos = players[closest_player]->transform->GetGlobalPosition() + prev_middle;

            auto frustum = GetComponent<ComponentCamera>()->frustum;
            frustum.pos = pos;
            bool inside = true;
            AABB aabbs[2] = { players[0]->GetComponent<PlayerController>()->max_aabb, players[1]->GetComponent<PlayerController>()->max_aabb };
            for (int i = 0; i < players.size(); ++i)
            {
                aabbs[i].minPoint += players[i]->transform->GetGlobalPosition();
                aabbs[i].maxPoint += players[i]->transform->GetGlobalPosition();
                if (!frustum.Contains(aabbs[i]))
                {
                    inside = false;
                }
            }
            if (inside)
                transform->SetGlobalPosition(transform->GetGlobalPosition() + (pos - transform->GetGlobalPosition()) * smooth_cam_vel * Time::GetDT());

            inside = true;
            pos = CalculateMidPoint() + trg_offset;
            frustum.pos = pos;
            for (int i = 0; i < players.size(); ++i)
            {
                if (!frustum.Contains(aabbs[i]))
                {
                    inside = false;
                }
            }
            if (inside) {
                if (prev_state == CameraState::FREE)
                    prev_state = CameraState::FREE_TO_DYNAMIC;

                state = prev_state;
            }
        break;
    }
    case CameraState::FREE_TO_DYNAMIC: {
        float3 pos = transform->GetGlobalPosition();
        transform->SetGlobalPosition(pos + ((CalculateMidPoint() + trg_offset) - pos) * smooth_cam_vel * Time::GetDT());
        break;
    }
    case CameraState::STATIC:
            LookAtMidPoint();
        break;
    case CameraState::AXIS: {
        if (smooth_camera)
            transform->SetGlobalPosition(transform->GetGlobalPosition() + ((CalculateAxisMidPoint()) - transform->GetGlobalPosition()) * smooth_cam_vel * Time::GetDT());
        else {
            transform->SetGlobalPosition(CalculateAxisMidPoint()/* + trg_offset*/);
        }
        LookAtMidPoint();
        //LOG("POSITION X: %f Y: %f Z: %f", transform->GetGlobalPosition().x, transform->GetGlobalPosition().y, transform->GetGlobalPosition().z);
        break;
    }
    case CameraState::CINEMATIC_TO_AXIS:
    {
        float min_dist = 0.5f;
        if (trg_offset.Distance(transform->GetGlobalPosition()) < min_dist) {
            LOG("Finished transition");
            first_pos = trg_offset;
            transform->SetGlobalPosition(trg_offset);
            trg_offset = transform->GetGlobalPosition() - CalculateMidPoint();
            state = CameraState::AXIS;
        }
        LookAtMidPoint();
        break;
    }
    case CameraState::MOVING_TO_AXIS: {
        if (is_cinematic)
        {
            prev_state = CameraState::MOVING_TO_AXIS;
            state = CameraState::MOVING_TO_CINEMATIC;
        }
        else {
            Tween::TweenMoveTo(this->game_object, trg_offset, transition_time);
            state = CameraState::CINEMATIC_TO_AXIS;
        }
        break;
    }
    case CameraState::CINEMATIC_TO_STATIC:
    {

        float min_dist = 0.1f;
        if (trg_offset.Distance(transform->GetGlobalPosition()) < min_dist) {
            LOG("Finished transition");
            transform->SetGlobalPosition(trg_offset);
            state = CameraState::STATIC;
        }
        LookAtMidPoint();
        break;
    }
    case CameraState::MOVING_TO_STATIC: {
        if (is_cinematic)
        {
            prev_state = CameraState::MOVING_TO_STATIC;
            state = CameraState::MOVING_TO_CINEMATIC;
        }
        else {
            Tween::TweenMoveTo(this->game_object, trg_offset, transition_time);
            state = CameraState::CINEMATIC_TO_STATIC;
        }
        break;
    }
    case CameraState::MOVING_TO_DYNAMIC:
    {
        if (is_cinematic)
        {
            prev_state = CameraState::MOVING_TO_DYNAMIC;
            state = CameraState::MOVING_TO_CINEMATIC;
        }
        else {
            current_transition_time += Time::GetDT();
            float3 trg_pos = CalculateMidPoint() + trg_offset;
            float3 curr_pos = transform->GetGlobalPosition();
            float min_dist = 0.1f;
            if ((trg_pos - curr_pos).Length() < min_dist) {
                LOG("Finished transition");
                transform->SetGlobalPosition(trg_pos);
                state = CameraState::DYNAMIC;
            }
            else {
                float time_percent = (current_transition_time / curr_transition.transition_time);//A value from 0 to 1, 0 meaning it has just started and 1 meaning it has finished
                transform->SetGlobalPosition(transform->GetGlobalPosition() + (trg_pos - curr_pos) * (time_percent));//Faster on the beggining
            }
            LookAtMidPoint();
        }
        break;
    }
    case CameraState::MOVING_TO_CINEMATIC:
    {
        //current_transition_time += Time::GetDT();
        //float3 trg_pos = CalculateMidPoint() + cutscene_game_object->transform->GetGlobalPosition();
        //float3 curr_pos = transform->GetGlobalPosition();
        //float min_dist = 0.1f;
        //if ((trg_pos - curr_pos).Length() < min_dist) {
        //    LOG("Finished transition");
        //    transform->SetGlobalPosition(trg_pos);
            if (cutscene_game_object) {
                cutscene_game_object->GetComponent<CutsceneCamera>()->PrepareCutscene();
                state = CameraState::CINEMATIC;
            }
        //}
        //else {
        //    float time_percent = (current_transition_time / curr_transition.transition_time);//A value from 0 to 1, 0 meaning it has just started and 1 meaning it has finished
        //    transform->SetGlobalPosition(transform->GetGlobalPosition() + (trg_pos - curr_pos) * (time_percent));//Faster on the beggining
        //}
        break;
    }
    case CameraState::CINEMATIC: 
    {
        if(cutscene_game_object)
            cutscene_game_object->GetComponent<CutsceneCamera>()->ExecuteCutscene();
        break;
    }
    }
}

void CameraMovement::LookAtMidPoint()
{
    float3 direction = (CalculateMidPoint() - transform->GetGlobalPosition()).Normalized();

    transform->SetGlobalRotation(Quat::LookAt(float3::unitZ(), direction, float3::unitY(), float3::unitY()));
}

float3 CameraMovement::CalculateCameraPos(const float& ang1, const float& ang2, const float& dst)
{
    float angle1 = math::DegToRad(ang1);
    float angle2 = math::DegToRad(ang2);

    return float3(cos(angle1) * cos(angle2), sin(angle2), sin(angle1) * cos(angle2)) * dst;
}

void CameraMovement::OnDrawGizmos()
{
    float3 mid_point = float3::zero();
    if (players.size() == 0 || search_players) {
        players.clear();
        SearchAndAssignPlayers();
        search_players = false;
    }
    else {
         mid_point = CalculateMidPoint();
    }
    Gizmos::DrawWireSphere(mid_point, .15f, Color::Cyan(), 0.5F); // mid point

    float3 cam_pos = mid_point + CalculateCameraPos(hor_angle, vert_angle, distance);
    Gizmos::DrawWireSphere(cam_pos, 0.15f, Color::Green());

    Gizmos::DrawLine(mid_point, cam_pos, Color::Red()); // line mid -> future camera pos

    if (limiter1 != nullptr && limiter2 != nullptr) {
        Gizmos::DrawSphere(limiter1->transform->GetGlobalPosition(), 0.3f, Color::Blue());
        Gizmos::DrawSphere(limiter2->transform->GetGlobalPosition(), 0.3f, Color::Blue());
        Gizmos::DrawLine(limiter1->transform->GetGlobalPosition(), limiter2->transform->GetGlobalPosition(), Color::Black());
        float3 mid = (limiter1->transform->GetGlobalPosition() + limiter2->transform->GetGlobalPosition()) * 0.5f;
        float3 dir = mid + (limiter1->transform->GetGlobalPosition() - limiter2->transform->GetGlobalPosition()).Cross(float3::unitY()).Normalized();
        Gizmos::DrawLine(mid, dir, Color::Blue());
        Gizmos::DrawSphere(dir, 0.2f, Color::Cyan());
    }
}

void CameraMovement::SearchAndAssignPlayers()
{
    auto objs = GameObject::FindGameObjectsWithTag("Player");
    for (auto i = objs.begin(); i != objs.end(); i++) {
        if (std::find(players.begin(), players.end(), *i) == players.end())
            players.push_back(*i);
    }
    num_curr_players = objs.size();
}

float3 CameraMovement::CalculateMidPoint()
{
    float3 mid_pos(0,0,0);
    for (std::vector<GameObject*>::iterator it = players.begin(); it != players.end(); ++it)
    {
        if ((*it) != nullptr)
        {
            mid_pos += (*it)->transform->GetGlobalPosition();
        }
    }

    return (players.size() == 0) ? mid_pos : mid_pos / players.size();
}

float3 CameraMovement::CalculateAxisMidPoint()
{
    float3 mid_pos(0, 0, 0);
    for (std::vector<GameObject*>::iterator it = players.begin(); it != players.end(); ++it)
    {
        mid_pos += (*it)->transform->GetGlobalPosition();
    }
    switch ((CameraAxis)axis)
    {
    case CameraAxis::X://X
    {
        float3 mid;
        mid = float3((mid_pos.x) * 0.5f, 0, 0);
        return float3(mid.x + trg_offset.x, first_pos.y, first_pos.z);
        break;
    }
    case CameraAxis::Y://Y
    {
        float3 mid;
        mid = float3(0, (mid_pos.y) * 0.5f, 0);
        return float3(first_pos.x, mid.y + trg_offset.y, first_pos.z);
        break;
    }
    case CameraAxis::Z://Z
    {
        float3 mid;
        mid = float3(0, 0, (mid_pos.z) * 0.5f);
        return float3(first_pos.x, first_pos.y, mid.z + trg_offset.z);
        break;
    }
    }
    return mid_pos;
}
