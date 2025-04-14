#include "CutsceneManager.h"


void CutsceneManager::queueCommand(Command* cmd) {
	commands.push(cmd);
	active = true;
}

void CutsceneManager::update(float dt) {
	if (!active || commands.empty())
		return;

	Command* current = commands.front();
	current->update(dt);

	if (current->isDone()) {
		delete current;
		commands.pop();
	}

	active = !commands.empty();
}

void CutsceneManager::draw() {
	if (!active || commands.empty())
		return;
	Command* current = commands.front();
	current->draw();
}