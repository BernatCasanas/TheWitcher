#pragma once

#include "..\..\Alien Engine\Alien.h"
#include "Macros/AlienScripts.h"
#include <tuple>

class DialogueManager; 
struct Dialogue; 
class PlayerController; 
class EventManager;


class ALIEN_ENGINE_API DialogueEventChecker : public Alien {

public:
	DialogueEventChecker();
	virtual ~DialogueEventChecker();
	void Start();
	void Update();

	
private: 
	void LoadJSONLogicalDialogues();

	// TODO: add chekers
	void CheckKills(Dialogue& dialogue);

private:
	// vector of cheker functions and their associated dialogue data
	std::vector<std::tuple<void(*)(Dialogue), Dialogue>> checkers;
	DialogueManager* dialogueManager; 
	EventManager* eventManager; 
	std::vector <std::tuple<std::string, std::string, float>> logicalDialogueData;
};

ALIEN_FACTORY DialogueEventChecker* CreateDialogueEventChecker() {
	DialogueEventChecker* alien = new DialogueEventChecker();
	// To show in inspector here

	return alien;
}


