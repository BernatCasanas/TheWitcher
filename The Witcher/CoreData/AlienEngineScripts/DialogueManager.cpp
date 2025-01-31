#include "GameManager.h"
#include "DialogueManager.h"
#include "EventManager.h"
#include "..\..\ComponentText.h"

DialogueManager::DialogueManager() : Alien()
{
}

DialogueManager::~DialogueManager()
{
}

void DialogueManager::Start()
{
	subtitlesUI = GameObject::FindWithName("Subtitles UI");
	audioEmitter = GetComponent<ComponentAudioEmitter>();
	GameObject* subtitleText = subtitlesUI->GetChild("Subtitles Text");
	text = subtitleText->GetComponent<ComponentText>();
	if(text)
		text->SetAlpha(1.0f);
	GameObject* subtitleBackground = subtitlesUI->GetChild("Subtitles Background");
	if (subtitleBackground)
	{
		image = subtitleBackground->GetComponent<ComponentImage>();
		start_bg_alpha = image->current_color.a;
		image->SetBackgroundColor(image->current_color.r, image->current_color.g, image->current_color.b, 0.0f);
	}

	if(audioEmitter)
		audioEmitter->ChangeVolume(0.5f); // some dialogues are low, so we can change the volume according to this (0->1)
	LoadJSONDialogues();
}

void DialogueManager::LoadJSONDialogues()
{
	// Credits to Yessica
	std::string json_path = std::string("GameData/Subtitles/InGameDialogues.json");
	LOG("READING %s", json_path.data());
	JSONfilepack* jsonDoc = JSONfilepack::GetJSON(json_path.c_str());
	if (jsonDoc)
	{
		JSONArraypack* dialogues = jsonDoc->GetArray("dialogues");
		if (dialogues == nullptr)
		{
			LOG("No dialogues array in fucking dialogues JSON");
			return;
		}

		do
		{
			//LOG("Loading new dialogue data...");
			std::string eventName = dialogues->GetString("eventName");
			std::string subtitles = dialogues->GetString("subtitles");
			float time = dialogues->GetNumber("time");

			dialogueData.push_back(std::make_tuple(eventName, subtitles, time)); // TODO: a dialogue

		} while (dialogues->GetAnotherNode());
	}
	else
		LOG("Couldn't open fucking dialogues JSON");


	JSONfilepack::FreeJSON(jsonDoc);
}

void DialogueManager::Update()
{
	if (text && image && audioEmitter)
	{
		if (playing)
		{
			if ((currentDialogue.subtitlesTime.currentTime += Time::GetDT()) >= currentDialogue.subtitlesTime.totalTime)
			{
				playing = false;
				text->SetEnable(false);
				image->SetBackgroundColor(image->current_color.r, image->current_color.g, image->current_color.b, 0.0f);
				currentDialogue.Reset();
				audioEmitter->ChangeVolume(0.5f);
			}
		}
		else {
			audioEmitter->SetState("GameVolumes", "None");
		}
	}

}

void DialogueManager::Pause(bool pause)
{
	playing = !playing;
	
	if (pause) 
		audioEmitter->PauseByEventName(currentDialogue.audioData.eventName.c_str());
	else
		audioEmitter->ResumeByEventName(currentDialogue.audioData.eventName.c_str());
}

bool DialogueManager::InputNewDialogue(Dialogue& dialogue, float volume)
{
	// TODO: We don't have a way of knowing the sound's duration, so we won't take into account the priority
	// since we don't know when to reset the current dialogue

	/*if ((currentDialogue.audioData.eventName != "noName") && (eventManager->eventPriorities.at(currentDialogue.priority) < eventManager->eventPriorities.at(dialogue.priority)))
	{
		LOG("Dialogue with less priority than the current one will be discarded...");
		return false;
	}; */


	OverrideDialogue(dialogue, volume);

	playing = true;

	return true;
}

bool DialogueManager::InputNewDialogue(int index, float volume)
{
	assert((index <= (dialogueData.size() - 1)) && "Invalid dialogue index");
	if (index > (dialogueData.size() - 1))
	{
		LOG("Invalid dialogue index");
		return false;
	}

	Dialogue dialogue;
	dialogue.audioData.eventName = std::get<0>(dialogueData.at(index));
	dialogue.subtitlesText = std::get<1>(dialogueData.at(index));
	dialogue.subtitlesTime.totalTime = std::get<2>(dialogueData.at(index));

	OverrideDialogue(dialogue, volume);

	playing = true;


	return true;
}


void DialogueManager::OverrideDialogue(Dialogue& newDialogue, float volume)
{
	// Stop playing 
	audioEmitter->StopSoundByName(currentDialogue.audioData.eventName.c_str());
	LOG("Stopped playing dialogue with event name: %s", currentDialogue.audioData.eventName.c_str());

	// Set Data 
	currentDialogue.audioData.eventName = std::string(newDialogue.audioData.eventName.c_str());
	currentDialogue.priority = std::string(newDialogue.priority.c_str());
	currentDialogue.subtitlesText = std::string(newDialogue.subtitlesText.c_str());
	currentDialogue.subtitlesTime = newDialogue.subtitlesTime;

	// Set Subtitles 
	if (text->IsEnabled() == false)
		text->SetEnable(true);
	image->SetBackgroundColor(image->current_color.r, image->current_color.g, image->current_color.b, start_bg_alpha);
	text->SetText(newDialogue.subtitlesText.c_str());

	// Play new
	audioEmitter->SetState("GameVolumes", "Dialogues");
	audioEmitter->ChangeVolume(volume);
	audioEmitter->StartSound(currentDialogue.audioData.eventName.c_str());
	LOG("Started playing dialogue with event name: %s", currentDialogue.audioData.eventName.c_str());
}