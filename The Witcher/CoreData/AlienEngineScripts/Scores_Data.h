#pragma once

struct Scores_Data 
{
	static int player1_kills;
	static int player2_kills;
	static bool won_level1;
	static bool won_level2;
	static std::string last_scene;
	static bool dead;
	static float3 last_checkpoint_position;
};

int Scores_Data::player1_kills = 25;
int Scores_Data::player2_kills = 16;
bool Scores_Data::won_level1 = false;
bool Scores_Data::won_level2 = false;
bool Scores_Data::dead = false;
std::string Scores_Data::last_scene;
float3 Scores_Data::last_checkpoint_position = float3::inf();