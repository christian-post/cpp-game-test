#include "CutsceneManager.h"
#include "Commands.h"


void CutsceneManager::queueCommand(Command* cmd, bool blocking) {
	commands.push({ cmd, blocking });
	active = true;
}

void CutsceneManager::update(float deltaTime) {
	// advance the global cooldown timer(s)
	Command_Textbox::updateCooldown(deltaTime);
	// handle the Commands logic
	for (auto it = nonBlocking.begin(); it != nonBlocking.end(); ) {
		(*it)->update(deltaTime);
		if ((*it)->isDone() && !(*it)->isPersistent()) {
			delete* it;
			it = nonBlocking.erase(it);
		}
		else {
			++it;
		}
	}

	if (!active || commands.empty())
		return;

	QueuedCommand& current = commands.front();

	if (!current.blocking) {
		nonBlocking.push_back(current.cmd);
		commands.pop();
	}
	else {
		current.cmd->update(deltaTime);
		if (current.cmd->isDone()) {
			delete current.cmd;
			commands.pop();
		}
	}

	if (commands.empty() && nonBlocking.size() > 0) {
		for (auto it = nonBlocking.begin(); it != nonBlocking.end(); ) {
			if ((*it)->isPersistent()) {
				delete* it;
				it = nonBlocking.erase(it);
			}
			else {
				++it;
			}
		}
	}
	active = !commands.empty() || !nonBlocking.empty();
}

void CutsceneManager::draw() {
	if (!active) return;

	if (!commands.empty()) {
		commands.front().cmd->draw();
	}

	for (auto* cmd : nonBlocking)
		cmd->draw();
}
