#pragma once

#include "Common.h"
#include "GameStates.h"
#include "Scene.h"
#include "MapScene.h"

class Engine
{
public:
	Engine();
	~Engine();

	void Update();
	void Render(sf::RenderWindow* w);

	void SwitchState(States state);

private:
	ResourceCache resources;

	Scene* active_scene;
	MapScene* map_scene; //the main scene where the player walks around

	States game_state;
};
