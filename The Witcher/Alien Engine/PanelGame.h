#pragma once

#include "Panel.h"

class PanelGame : public Panel {

public:

	PanelGame(const std::string& panel_name, const SDL_Scancode& key1_down, const SDL_Scancode& key2_repeat = SDL_SCANCODE_UNKNOWN, const SDL_Scancode& key3_repeat_extra = SDL_SCANCODE_UNKNOWN);
	virtual ~PanelGame();

	void PanelLogic();

public:

	bool game_focused = false;

	float posX = 0;
	float posY = 0;
	float width = 960;
	float height = 540;
private:
	ImVec2 viewport_size = { 0.f, 0.f };
	ImVec2 viewport_min = { 0.f, 0.f };
	ImVec2 viewport_max = { 0.f, 0.f };
	ImVec2 current_viewport_size = { 0.f, 0.f };
};

#pragma once
