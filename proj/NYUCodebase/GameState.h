#ifndef GAMESTATE_H
#define GAMESTATE_H

#endif

class GameState {
public:
	virtual void processInput(SDL_Event& event, bool& done) = 0;
	virtual void Render() = 0;
	virtual void Update(float elapsed) = 0;
};
