#include "MainMenu_Buttons.h"
#include "Extra_Menus.h"

MainMenu_Buttons::MainMenu_Buttons() : Alien()
{
}

MainMenu_Buttons::~MainMenu_Buttons()
{
}

void MainMenu_Buttons::NewGame()
{
	SceneManager::LoadScene("Intro", FadeToBlackType::HORIZONTAL_CURTAIN);
}

void MainMenu_Buttons::ExitGame()
{
	AlienEngine::QuitApp();
}

void MainMenu_Buttons::Controls()
{
	GameObject::FindWithName("Extra_Menus")->SetEnable(true);
	GameObject::FindWithName("Extra_Menus")->GetComponent<Extra_Menus>()->MenuSpawn(Extra_Menus::MENU::CONTROLS);
	GameObject::FindWithName("Main_Menu_UI")->SetEnable(false);
}

void MainMenu_Buttons::Credits()
{
	SceneManager::LoadScene("CreditsMenu", FadeToBlackType::FADE);
}


void MainMenu_Buttons::Muffin()
{
	AlienEngine::OpenURL("https://overpowered-team.github.io/TheWitcher/index.html");
}

void MainMenu_Buttons::Settings()
{
	GameObject::FindWithName("Extra_Menus")->SetEnable(true);

	GameObject::FindWithName("Extra_Menus")->GetComponent<Extra_Menus>()->MenuSpawn(Extra_Menus::MENU::SETTINGS);
	GameObject::FindWithName("Extra_Menus")->GetComponent<ComponentCanvas>()->allow_navigation = true;

	GameObject::FindWithName("Main_Menu_UI")->SetEnable(false);

}

void MainMenu_Buttons::AddVolume()
{
	volume += 5.0f;
	if (volume >= 100.0f)
		volume = 100.0f;

	ComponentAudioEmitter* emitter = GetComponent<ComponentAudioEmitter>();
	emitter->SetRTPCValue("GeneralVolume", volume);

}

void MainMenu_Buttons::SubtractVolume()
{
	ComponentAudioEmitter* emitter = GetComponent<ComponentAudioEmitter>();

}

void MainMenu_Buttons::FullScreen()
{
	if (Screen::IsFullScreen())
		Screen::SetFullScreen(false);
	else
		Screen::SetFullScreen(true);
	
}


