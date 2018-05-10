#define NDEBUG
#define STB_IMAGE_IMPLEMENTATION
#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <assert.h>

#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

#include "FlareMap.h"
#include "GameState.h"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "Matrix.h"
#include "Entity.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

// GLOBALS
SDL_Window* displayWindow;
Matrix projectionMatrix;
Matrix viewMatrix;
Matrix modelMatrix;
ShaderProgram untextured;
ShaderProgram textured;
enum GameMode { STATE_MAIN_MENU, STATE_LEVEL_1, STATE_LEVEL_2, STATE_LEVEL_3, STATE_GAME_OVER, STATE_WIN };
GameMode mode = STATE_MAIN_MENU;
int WINDOW_W = 1280;
int WINDOW_H = 720;
int LEVEL_HEIGHT = 12;
int LEVEL_WIDTH = 150;
float TILE_SIZE = 4.0f / LEVEL_HEIGHT;
float RES = WINDOW_W / WINDOW_H;
class Player;

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	stbi_image_free(image);
	return retTexture;
}
void DrawText(ShaderProgram* shader, int fontTexture, std::string text, float size, float spacing, Matrix modelMatrix, Matrix viewMatrix) {
	float texture_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0;
		float texture_y = (float)(spriteIndex / 16) / 16.0;
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glUseProgram(shader->programID);
	

	shader->SetModelMatrix(modelMatrix);
	shader->SetProjectionMatrix(projectionMatrix);
	shader->SetViewMatrix(viewMatrix);
	for (int i = 0; i < vertexData.size(); i += 12) {
		float position[] = {
			vertexData[i], vertexData[i + 1],
			vertexData[i + 2], vertexData[i + 3],
			vertexData[i + 4], vertexData[i + 5],

			vertexData[i + 6], vertexData[i + 7],
			vertexData[i + 8], vertexData[i + 9],
			vertexData[i + 10], vertexData[i + 11],
		};
		float textureCord[] = {
			texCoordData[i], texCoordData[i + 1],
			texCoordData[i + 2], texCoordData[i + 3],
			texCoordData[i + 4], texCoordData[i + 5],

			texCoordData[i + 6], texCoordData[i + 7],
			texCoordData[i + 8], texCoordData[i + 9],
			texCoordData[i + 10], texCoordData[i + 11] };
		glVertexAttribPointer(shader->positionAttribute, 2, GL_FLOAT, false, 0, position);
		glEnableVertexAttribArray(shader->positionAttribute);

		glVertexAttribPointer(shader->texCoordAttribute, 2, GL_FLOAT, false, 0, textureCord);
		glEnableVertexAttribArray(shader->texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
}

class Interactive : public Entity {
public:
	virtual void interact(int interactionID) = 0;
	Interactive(float _x, float _y, float tileSize, int t_ind, int SPRITE_COUNT_X, int SPRITE_COUNT_Y, float img_width, float img_height) : Entity(_x, _y, tileSize, t_ind, SPRITE_COUNT_X, SPRITE_COUNT_Y, img_width, img_height) {}
	bool isSolid() { return solid; }
protected:
	bool solid;
};

class CoinBlock : public Interactive {
public:
	CoinBlock(float _x, float _y, float tileSize, int SPRITE_COUNT_X, int SPRITE_COUNT_Y, float img_width, float img_height) : Interactive(_x, _y, tileSize, 133, SPRITE_COUNT_X, SPRITE_COUNT_Y, img_width, img_height) {
		hasGravity = false;
		solid = true;
		Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
		coinSound = Mix_LoadWAV("coin.wav");
		Mix_VolumeChunk(coinSound, 5);

	}
	void interact(int interactionID) { if (interactionID == 2) { Mix_PlayChannel(-1, coinSound, 0); t_Ind = 192; }}
private:
	
	Mix_Chunk *coinSound;
	
};

class Lever : public Interactive {
public:
	Lever(float _x, float _y, float tileSize, int t_IndON, int t_IndOff, int SPRITE_COUNT_X, int SPRITE_COUNT_Y, float img_width, float img_height) : Interactive(_x, _y, tileSize, t_IndOff, SPRITE_COUNT_X, SPRITE_COUNT_Y, img_width, img_height), t_IndOff(t_IndOff), t_IndON(t_IndON){
		hasGravity = false;
		solid = false;
	}
	void interact(int interactionID) { 
		if (interactionID == 0) { 
			onOff = true;
			if (onOff) { t_Ind = t_IndON; }
			else { t_Ind = t_IndOff; }
		}
	}
	bool isOn() const { return onOff; }
private:
	int t_IndOff;
	int t_IndON;
	bool onOff = false;
};

class Enemy : public Entity {
public:
	virtual void AI(const std::vector<std::vector<Tile*>>& collisionChart) = 0;
	Enemy(float _x, float _y, float tileSize, int t_ind, int SPRITE_COUNT_X, int SPRITE_COUNT_Y, float img_width, float img_height) : Entity(_x, _y, tileSize, t_ind, SPRITE_COUNT_X, SPRITE_COUNT_Y, img_width, img_height) {}
	void dead() { alive = false; }
	bool isAlive() const { return alive; }
protected:
	bool alive;
};

class Slime : public Enemy {
public:
	Slime(float _x, float _y, float tileSize, int SPRITE_COUNT_X, int SPRITE_COUNT_Y, float img_width, float img_height) : Enemy(_x, _y, tileSize, 260, SPRITE_COUNT_X, SPRITE_COUNT_Y, img_width, img_height) {
		hasGravity = true;
		x_vel = -0.01;
	}
	void AI(const std::vector<std::vector<Tile*>>& collisionChart) {
		int nextY = getCenterY() + 0.5;
		int nextX;
		if (x_vel < 0) { nextX = getCenterX() - 1; }
		else { nextX = getCenterX() + 1; }
		if (nextX > LEVEL_WIDTH - 2) { turnAround(); }
		if (collisionChart[nextY][nextX]->solid == false | collisionChart[nextY - 1][nextX]->solid == true) { turnAround(); }
		

		if (abs(walkDis) > 0.25) {
			walkInd++;
			if (walkInd > walkFrames - 1) {
				walkInd = 0;
			}
			t_Ind = walkAnimation[walkInd];
			walkDis = 0;
		}
		walkDis += x_vel;


		if (!alive) { t_Ind = 198; }
	}
	void turnAround() {
		x_vel *= -1;
		mirrorTEX = !mirrorTEX;
	}
private:
	//--------------------------ANIMATIONS-----------------------------
	float walkDis = 0;
	int walkInd = 0;
	const int walkFrames = 3;
	const int walkAnimation[3] = { 259, 260, 261 };
};

class Player : public Entity {
public:
	Player() {}
	Player(float _x, float _y, float tileSize, int t_ind, int SPRITE_COUNT_X, int SPRITE_COUNT_Y, float img_width, float img_height) : Entity(_x, _y, tileSize, t_ind, SPRITE_COUNT_X, SPRITE_COUNT_Y, img_width, img_height) {
		Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
		jumpSound = Mix_LoadWAV("jump.wav");
		killSound = Mix_LoadWAV("kill.wav");
		Mix_VolumeChunk(jumpSound, 5);
	}
	void move(float x) {
		if (x != 0) { walking = true; climbing = false; }
		else { walking = false; }

		if (x < 0 && !mirrorTEX) { mirrorTEX = true; }
		if (x > 0 && mirrorTEX) { mirrorTEX = false; }

		x_vel = speed * x;
	}
	void stopClimb() {
		climbing = false;
		hasGravity = true;
	}
	bool isClimbing() const { return climbing; }
	void climb(float y) {
		if (!climbing) { climbStart = getCenterY(); }
		if (y != 0) { climbing = true; onFloor = false; }

		y_vel = speed * y;
	}
	void jump() {
		if (onFloor) {
			Mix_PlayChannel(-1, jumpSound, 0);
			y_vel = -0.25;
			onFloor = false;
		}
	}

	void action(std::vector<Interactive*> objects) {
		for (Interactive* object : objects) {
			int objX = object->getCenterX();
			int objY = object->getCenterY();

			int x = getCenterX();
			int y = getCenterY();
			if (objX == x && objY == y) { object->interact(0); }
		}
	}
	bool checkEnemyCollision(Enemy* e) {
		if (e->isAlive()) {
			int eX = e->getCenterX();
			int eY = e->getCenterY();

			int x = getCenterX() - 0.2;
			int y = getCenterY() + 0.5;
			if (eX == x && eY == y) {
				y_vel = -0.1;
				e->dead();
				Mix_PlayChannel(-1, killSound, 0);
				return false;
			}
			x = getCenterX() + 0.01;
			y = getCenterY() + 0.5;
			if (eX == x && eY == y) {
				y_vel = -0.1;

				e->dead();
				Mix_PlayChannel(-1, killSound, 0);
				return false;
			}

			x = getCenterX() - 0.2;
			y = getCenterY() - 0.5;
			if (eX == x && eY == y) {
				y_vel = -0.1;
				e->dead();
				Mix_PlayChannel(-1, killSound, 0);
				return false;
			}


			x = getCenterX() + 0.01;
			y = getCenterY() - 0.5;
			if (eX == x && eY == y) {
				return true;
			}

			x = getCenterX() + 0.5;
			y = getCenterY();
			if (eX == x && eY == y) {
				return true;
			}

			x = getCenterX() - 0.5;
			if (eX == x && eY == y) {
				return true;
			}
		}
		return false;
	}

	void checkObjectCollision(std::vector<Interactive*> objects) {
		for (Interactive* object : objects) {
			int objX = object->getCenterX();
			int objY = object->getCenterY();

			int x = getCenterX() - 0.2;
			int y = getCenterY() + 0.5;
			if (objX == x && objY == y) {
				if (object->isSolid()) { collisionBot(y); object->interact(1); }
			}

			x = getCenterX() + 0.01;
			y = getCenterY() + 0.5;
			if (objX == x && objY == y) {
				if (object->isSolid()) { collisionBot(y); object->interact(1); }
			}

			x = getCenterX() - 0.2;
			y = getCenterY() - 0.5;
			if (objX == x && objY == y) {
				if (object->isSolid()) { collisionTop(y); object->interact(2); }
			}

			x = getCenterX() + 0.01;
			y = getCenterY() - 0.5;
			if (objX == x && objY == y) {
				if (object->isSolid()) { collisionTop(y); object->interact(2); }
			}

			x = getCenterX() + 0.5;
			y = getCenterY();
			if (objX == x && objY == y) {
				if (object->isSolid()) { collisionRight(x); object->interact(3); }
			}

			x = getCenterX() - 0.5;
			if (objX == x && objY == y) {
				if (object->isSolid()) { collisionLeft(x); object->interact(4); }
			}
		}
	}
	bool isAlive() const { return alive; }
	void dead() { alive = false; }

	void update(float elapsed, float gravity) {
		hasGravity = true;

		if (y_vel == 0) { onFloor = true; }
		else { onFloor = false; }

		if (!onFloor && !climbing) { t_Ind = 57; }
		else if (climbing) {
			hasGravity = false;
			if (abs(climbStart - getCenterY()) > 0.15) { climbInd++; climbStart = getCenterY(); }
			if (climbInd > climbFrames - 1) {
				climbInd = 0;
			}
			t_Ind = climbAnimation[climbInd];
		}
		else if (walking) {
			walkInd++;
			if (walkInd > walkFrames - 1) {
				walkInd = 0;
			}
			t_Ind = walkAnimation[walkInd];
		}
		else {
			t_Ind = 49;
		}
	}
private:
	bool alive = true;
	bool onFloor = false;
	float speed = 0.1;

	Mix_Chunk *jumpSound;
	Mix_Chunk *killSound;
	//--------------------------ANIMATIONS-----------------------------
	bool walking = false;
	int walkInd = 0;
	const int walkFrames = 3;
	const int walkAnimation[3] = { 50, 58, 59 };

	bool climbing = false;
	int climbInd = 0;
	float climbStart = 0;
	const int climbFrames = 2;
	const int climbAnimation[2] = { 54, 55, };
};

class MainMenu : public GameState {
public:
	MainMenu() {}
	void setFont(GLuint font) { textTex = font; }
	void processInput(SDL_Event& event, bool& done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				mode = STATE_LEVEL_1;
			}
		}
	}
	void Update(float elapsed) {
		int time = ((float)SDL_GetTicks() - elapsed) / 500;
		if (time != currentTime) { blink = !blink; currentTime = time; }
	}
	void Render() {
		glClear(GL_COLOR_BUFFER_BIT);
		modelMatrix.Identity();
		modelMatrix.Translate(-1.3, 1, 0);
		DrawText(&textured, textTex, "Adventure Land!", 0.4, -0.2, modelMatrix, viewMatrix);
		if (blink) {
			modelMatrix.Identity();
			modelMatrix.Translate(-1.55, -1, 0);
			DrawText(&textured, textTex, "Press any key to play!", 0.25, -0.1, modelMatrix, viewMatrix);
		}
		SDL_GL_SwapWindow(displayWindow);
	}
private:
	Matrix modelMatrix;
	Matrix viewMatrix;
	GLuint textTex;
	bool blink = true;
	int currentTime = 0;
};


class GameOver : public GameState {
public:
	GameOver() {}
	void setFont(GLuint font) { textTex = font; }
	void processInput(SDL_Event& event, bool& done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				mode = STATE_MAIN_MENU;
			}
		}
	}
	void Update(float elapsed) {
		int time = ((float)SDL_GetTicks() - elapsed) / 500;
		if (time != currentTime) { blink = !blink; currentTime = time; }
	}
	void Render() {
		glClear(GL_COLOR_BUFFER_BIT);
		modelMatrix.Identity();
		modelMatrix.Translate(-1.3, 1, 0);
		DrawText(&textured, textTex, "Game Over", 0.5, -0.2, modelMatrix, viewMatrix);
		if (blink) {
			modelMatrix.Identity();
			modelMatrix.Translate(-1.55, -1, 0);
			DrawText(&textured, textTex, "Press any key to play again!", 0.25, -0.1, modelMatrix, viewMatrix);
		}
		SDL_GL_SwapWindow(displayWindow);
	}
private:
	Matrix modelMatrix;
	Matrix viewMatrix;
	GLuint textTex;
	bool blink = true;
	int currentTime = 0;
};


class Win : public GameState {
public:
	Win() {}
	void setFont(GLuint font) { textTex = font; }
	void processInput(SDL_Event& event, bool& done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				mode = STATE_MAIN_MENU;
			}
		}
	}
	void Update(float elapsed) {
		int time = ((float)SDL_GetTicks() - elapsed) / 500;
		if (time != currentTime) { blink = !blink; currentTime = time; }
	}
	void Render() {
		glClear(GL_COLOR_BUFFER_BIT);
		modelMatrix.Identity();
		modelMatrix.Translate(-1.3, 1, 0);
		DrawText(&textured, textTex, "You Win!", 0.5, -0.2, modelMatrix, viewMatrix);
		if (blink) {
			modelMatrix.Identity();
			modelMatrix.Translate(-1.55, -1, 0);
			DrawText(&textured, textTex, "Press any key", 0.25, -0.1, modelMatrix, viewMatrix);
		}
		SDL_GL_SwapWindow(displayWindow);
	}
private:
	Matrix modelMatrix;
	Matrix viewMatrix;
	GLuint textTex;
	bool blink = true;
	int currentTime = 0;
};

class GameLevel : public GameState {
	virtual void Update(float elapsed) = 0;
public:
	GameLevel(const std::string& mapname) { 
		map.Load(mapname); 
		createTileCollisionMap();
		Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
		music = Mix_LoadMUS("game.mp3");
		Mix_VolumeMusic(5);
		Mix_PlayMusic(music, -1);
	}
	void checkPlayerEnemyCollision() {
		for (Enemy* e : enemies) {
			if (e->isAlive()) {
				if (player->checkEnemyCollision(e)) { player->dead(); }
			}
		}
	}
	void setup() {
		world.clear();
		objects.clear();
		levers.clear();
		enemies.clear();
		for (const FlareMapEntity& e : map.entities) {
			if (e.type == "Player") {
				player = new Player(e.x, e.y, TILE_SIZE, 49, 30, 30, 692, 692);
				player->setOrigin(-3.55, 2);
				player->setTex(textures[0]);
				world.push_back(player);
			}
			else if (e.type == "Interactive") {
				if (e.name == "Block") {
					CoinBlock* block = new CoinBlock(e.x, e.y, TILE_SIZE, 30, 30, 692, 692);
					block->setOrigin(-3.55, 2);
					block->setTex(textures[0]);
					world.push_back(block);
					objects.push_back(block);
				}
				else if (e.name == "Lever") {
					Lever* lever = new Lever(e.x, e.y, TILE_SIZE, 250, 252, 30, 30, 692, 692);
					lever->setOrigin(-3.55, 2);
					lever->setTex(textures[0]);
					levers.push_back(lever);
					world.push_back(lever);
					objects.push_back(lever);
				}
				else if (e.name == "LaserLever") {
					Lever* lever = new Lever(e.x, e.y, TILE_SIZE, 886, 887, 30, 30, 692, 692);
					lever->setOrigin(-3.55, 2);
					lever->setTex(textures[0]);
					levers.push_back(lever);
					world.push_back(lever);
					objects.push_back(lever);
				}
			}
			else if (e.type == "Enemy") {
				if (e.name == "Slime") {
					Slime* newSlime = new Slime(e.x, e.y, TILE_SIZE, 30, 30, 692, 692);
					newSlime->setOrigin(-3.55, 2);
					newSlime->setTex(textures[0]);
					enemies.push_back(newSlime);
					world.push_back(newSlime);
				}
			}
		}
	}

	void addTexture(const GLuint& textID) { textures.push_back(textID); }

	void processInput(SDL_Event& event, bool& done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		int x = 0;
		int y = 0;
		if (!keys[SDL_SCANCODE_D] && !keys[SDL_SCANCODE_A]) { x = 0; }
		if (keys[SDL_SCANCODE_D] && keys[SDL_SCANCODE_A]) { x = 0; }

		else {
			if (keys[SDL_SCANCODE_D]) {
				x = 1;
			}
			if (keys[SDL_SCANCODE_A]) {
				x = -1;
			}
		}
		if (keys[SDL_SCANCODE_SPACE]) {
			player->action(objects);
		}
		int tileY = player->getCenterY();
		int tileX = player->getCenterX();

		if (tileY > 0 && tileY < LEVEL_HEIGHT) {
			if (map.mapData[tileY][tileX] == 102 | map.mapData[tileY][tileX] == 103) {
				if (!keys[SDL_SCANCODE_W] && !keys[SDL_SCANCODE_S]) { player->climb(0); }
				if (keys[SDL_SCANCODE_W] && keys[SDL_SCANCODE_S]) { player->climb(0); }

				else {
					if (keys[SDL_SCANCODE_W]) {
						player->climb(-1);
					}
					if (keys[SDL_SCANCODE_S]) {
						player->climb(1);
					}
				}
			}
			else {
				player->stopClimb();
				if (keys[SDL_SCANCODE_W]) {
					player->jump();
				}
			}
		}
		player->move(x);
	}
	void Render() {
		glClear(GL_COLOR_BUFFER_BIT);
		drawLevel(x_min, x_max);
		SDL_GL_SwapWindow(displayWindow);
	}

	void scrollScreen() {
		if ((player->getCenterX() * 0.09295) + projectionMatrix.m[3][0] > 1) { projectionMatrix.Translate(-0.1 * TILE_SIZE, 0, 0); }
		if (projectionMatrix.m[3][0] < 0) {
			if ((player->getCenterX() * 0.092958) + projectionMatrix.m[3][0] < 0.9) { projectionMatrix.Translate(0.1 * TILE_SIZE, 0, 0); }
		}
		else {
			projectionMatrix.m[3][0] = 0;
		}
		if (projectionMatrix.m[3][0] < (-LEVEL_WIDTH + 20) * 0.09295) {
			projectionMatrix.m[3][0] = (-LEVEL_WIDTH + 20) * 0.09295;
		}
	}
	void changeDrawBounds() {
		int tileX = player->getCenterX();
		x_min = tileX - 21;
		if (x_min < 0) { x_min = 0; }
		x_max = x_min + 42;
		if (x_max > LEVEL_WIDTH) { x_max = LEVEL_WIDTH; }
	}
	void checkPlayerOutofBounds() {
		float checkX = player->getCenterX() - 0.5;
		if (checkX < 0) { player->collisionLeft(-1); }
		if (checkX + 1 > LEVEL_WIDTH) { player->collisionRight(LEVEL_WIDTH); }
	}
	
	bool isSolid(int tileID) {
		for (const int nonSolid : nonSolidTiles) {
			if (tileID == nonSolid) { return false; }
		}
		return true;
	}
	void createTileCollisionMap() {
		for (int y = 0; y < LEVEL_HEIGHT; y++) {
			std::vector<Tile*> tileRow;
			for (int x = 0; x < LEVEL_WIDTH; x++) {
				Tile* newTile = new Tile();
				int TILE_ID = map.mapData[y][x];
				if (isSolid(TILE_ID)) { newTile->solid = true; }
				else { newTile->solid = false; }
				tileRow.push_back(newTile);
			}
			collisionChart.push_back(tileRow);
		}
	}

	void drawLevel(int x_min, int x_max) const {	
		int SPRITE_COUNT_X, SPRITE_COUNT_Y;
		double x_offset, y_offset;
		int SPRITESHEET_WIDTH = 0, SPRITESHEET_HEIGHT = 0;
		std::vector<int> tileTexID;
		std::vector<float> vertexData;
		std::vector<double> texCoordData;
		for (int y = 0; y < LEVEL_HEIGHT; y++) {
			for (int x = x_min; x < x_max; x++) {
				int TILE_ID = map.mapData[y][x];
				if (TILE_ID > 899) { //TILE_ID IN SPRITESHEET2
					TILE_ID -= 900;
					SPRITE_COUNT_X = 14;
					x_offset = (1.0 / SPRITESHEET_WIDTH);
					y_offset = (1.0 / SPRITESHEET_HEIGHT);
					SPRITE_COUNT_Y = 7;
					SPRITESHEET_WIDTH = 294;
					SPRITESHEET_HEIGHT = 147;
					tileTexID.push_back(1);
				}
				else {
					SPRITE_COUNT_X = 30; //TILE_ID IN SPRITESHEET
					SPRITE_COUNT_Y = 30;
					x_offset = (2.0f / SPRITESHEET_WIDTH);
					y_offset = (2.0f / SPRITESHEET_HEIGHT);
					SPRITESHEET_WIDTH = 692;
					SPRITESHEET_HEIGHT = 692;
					tileTexID.push_back(0);
				}
				double u = (double)(((TILE_ID) % SPRITE_COUNT_X) / (double)SPRITE_COUNT_X);
				u += x_offset;
				double v = (double)((TILE_ID) / SPRITE_COUNT_X) / (double)SPRITE_COUNT_Y;
				v += y_offset;
				double spriteWidth = (20.0f / (double)SPRITESHEET_WIDTH);
				double spriteHeight = (20.0f / (double)SPRITESHEET_HEIGHT);
				vertexData.insert(vertexData.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
				});
				texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),
					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
				});
			}
		}
		textured.SetModelMatrix(modelMatrix);
		textured.SetProjectionMatrix(projectionMatrix);
		textured.SetViewMatrix(viewMatrix);
		for (int i = 0; i < vertexData.size(); i += 12) {
			float position[] = {
				vertexData[i], vertexData[i + 1],
				vertexData[i + 2], vertexData[i + 3],
				vertexData[i + 4], vertexData[i + 5],

				vertexData[i + 6], vertexData[i + 7],
				vertexData[i + 8], vertexData[i + 9],
				vertexData[i + 10], vertexData[i + 11],
			};
			float textureCord[] = {
				texCoordData[i], texCoordData[i + 1],
				texCoordData[i + 2], texCoordData[i + 3],
				texCoordData[i + 4], texCoordData[i + 5],

				texCoordData[i + 6], texCoordData[i + 7],
				texCoordData[i + 8], texCoordData[i + 9],
				texCoordData[i + 10], texCoordData[i + 11] };
			glVertexAttribPointer(textured.positionAttribute, 2, GL_FLOAT, false, 0, position);
			glEnableVertexAttribArray(textured.positionAttribute);

			glVertexAttribPointer(textured.texCoordAttribute, 2, GL_FLOAT, false, 0, textureCord);
			glEnableVertexAttribArray(textured.texCoordAttribute);
			glBindTexture(GL_TEXTURE_2D, textures[tileTexID[i / 12]]);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		for (Entity* e : world) {
			e->draw(textured,projectionMatrix);
		}
		player->draw(textured, projectionMatrix);
	}
	void checkPlayerAlive() {
		int tileY = player->getCenterY();
		int tileX = player->getCenterX();
		if (tileY > LEVEL_HEIGHT) { player->dead(); }
		if (tileY > 0 && tileY < LEVEL_HEIGHT) {
			if (map.mapData[tileY][tileX] == 981) { player->dead(); }
		}
		checkPlayerEnemyCollision();
	}
protected:
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	std::vector<GLuint> textures;

	Player* player;

	std::vector<Entity*> world;
	std::vector<Enemy*> enemies;
	std::vector<Interactive*> objects;
	std::vector<Lever*> levers;
	std::vector<std::vector<Tile*>> collisionChart;

	int x_min = 0;
	int x_max = 22;
	int nonSolidTiles[11] = { 17, 46, 102, 103, 136, 137, 120, 198, 253, 255, 981 };
	Mix_Music *music;

	float gravity = 0.75;
	FlareMap map;
	bool cutScene1 = true;
	
	float totalTime = 0;
};
class Level1 : public GameLevel {
public:
	Level1(const std::string& mapname) : GameLevel(mapname) {}
	void checkPlayerWin() {
		int tileY = player->getCenterY();
		int tileX = player->getCenterX();
		if (tileY > 0 && tileY < LEVEL_HEIGHT) {
			if (map.mapData[tileY][tileX] == 137) { projectionMatrix.SetPosition(0, 0, 0); mode = STATE_LEVEL_2; }
		}
	}
	void Update(float elapsed) {
		checkPlayerAlive();
		if (!player->isAlive()) { projectionMatrix.SetPosition(0, 0, 0); setup(); mode = STATE_GAME_OVER; }
		checkPlayerWin();
		scrollScreen();

		player->update(elapsed, gravity);
		checkPlayerOutofBounds();
		changeDrawBounds();

		int tileX = player->getCenterX();
		if (cutScene1 && tileX == 58) {
			float shiftX = 0;
			while (shiftX > -3.15) {
				projectionMatrix.Translate(-1.5 * elapsed, 0, 0);
				Render();
				shiftX += (-1.5 * elapsed);
			}
			cutScene1 = false;
		}

		if (levers[0]->isOn()) {
			for (int i = 62; i < 69; i++) {
				map.mapData[11][i] = 158;
				collisionChart[11][i]->solid = true;
			}
			map.mapData[11][62] = 129;
			map.mapData[11][68] = 159;
		}

		for (Enemy* e : enemies) { e->AI(collisionChart); }

		for (Entity* e : world) {
			e->update(elapsed, gravity);
			e->checkTileCollision(LEVEL_WIDTH, LEVEL_HEIGHT, collisionChart);
		}
		player->checkObjectCollision(objects);

		totalTime += elapsed;
	}
};
class Level2 : public GameLevel {
public:
	Level2(const std::string& mapname) : GameLevel(mapname) {}
	void checkPlayerWin() {
		int tileY = player->getCenterY();
		int tileX = player->getCenterX();
		if (tileY > 0 && tileY < LEVEL_HEIGHT) {
			if (map.mapData[tileY][tileX] == 137) { projectionMatrix.SetPosition(0, 0, 0); mode = STATE_LEVEL_3; }
		}
	}
	void Update(float elapsed) {
		checkPlayerWin();
		checkPlayerAlive();
		if (!player->isAlive()) { projectionMatrix.SetPosition(0, 0, 0); setup(); mode = STATE_GAME_OVER; }
		scrollScreen();

		player->update(elapsed, gravity);
		checkPlayerOutofBounds();
		changeDrawBounds();

		if (levers[0]->isOn()) {
			for (int i = 1; i < 10; i++) {
				map.mapData[i][12] = 198;
				collisionChart[i][12]->solid = false;
			}
		}
		if (!levers[0]->isOn()) {
			for (int i = 1; i < 10; i++) {
				map.mapData[i][12] = 981;
			}
		}
		if (levers[1]->isOn()) {
			for (int i = 1; i < 7; i++) {
				map.mapData[i][71] = 198;
				collisionChart[i][71]->solid = false;
			}
		}
		if (!levers[1]->isOn()) {
			for (int i = 1; i < 7; i++) {
				map.mapData[i][71] = 981;
			}
		}
		for (Enemy* e : enemies) { e->AI(collisionChart); }

		for (Entity* e : world) {
			e->update(elapsed, gravity);
			e->checkTileCollision(LEVEL_WIDTH, LEVEL_HEIGHT, collisionChart);
		}
		player->checkObjectCollision(objects);

		totalTime += elapsed;
	}
};

class Level3 : public GameLevel {
public:
	Level3(const std::string& mapname) : GameLevel(mapname) {}
	void checkPlayerWin() {
		int tileY = player->getCenterY();
		int tileX = player->getCenterX();
		if (tileY > 0 && tileY < LEVEL_HEIGHT) {
			if (map.mapData[tileY][tileX] == 137) { projectionMatrix.SetPosition(0, 0, 0); mode = STATE_WIN; }
		}
	}
	void Update(float elapsed) {
		checkPlayerWin();
		checkPlayerAlive();
		if (!player->isAlive()) { projectionMatrix.SetPosition(0, 0, 0); setup(); mode = STATE_GAME_OVER; }
		scrollScreen();

		player->update(elapsed, gravity);
		checkPlayerOutofBounds();
		changeDrawBounds();
		for (Enemy* e : enemies) { e->AI(collisionChart); }

		for (Entity* e : world) {
			e->update(elapsed, gravity);
			e->checkTileCollision(LEVEL_WIDTH, LEVEL_HEIGHT, collisionChart);
		}
		player->checkObjectCollision(objects);

		totalTime += elapsed;
	}
};
MainMenu mainMenu;
Level1 L1("L1.txt");
Level2 L2("L2.txt");
Level3 L3("L3.txt");
GameOver gameOver;
Win win;

void setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif
	glViewport(0, 0, WINDOW_W, WINDOW_H);
	untextured.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	textured.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	projectionMatrix.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	viewMatrix.Translate(-3.55, 2, 0);

	const GLuint tiles = LoadTexture(RESOURCE_FOLDER"spritesheet.png");
	const GLuint tiles2 = LoadTexture(RESOURCE_FOLDER"spritesheet2.png");
	const GLuint font = LoadTexture(RESOURCE_FOLDER"font1.png");
	mainMenu.setFont(font);
	gameOver.setFont(font);
	win.setFont(font);

	L1.addTexture(tiles);
	L1.addTexture(tiles2);
	L1.setup();

	L2.addTexture(tiles);
	L2.addTexture(tiles2);
	L2.setup();

	L3.addTexture(tiles);
	L3.addTexture(tiles2);
	L3.setup();
}

void processInput(SDL_Event& event, bool& done) {
	switch (mode) {
	case STATE_MAIN_MENU:
		mainMenu.processInput(event, done);
		break;
	case STATE_LEVEL_1:
		L1.processInput(event, done);
		break;
	case STATE_LEVEL_2:
		L2.processInput(event, done);
		break;
	case STATE_LEVEL_3:
		L3.processInput(event, done);
		break;
	case STATE_GAME_OVER:
		gameOver.processInput(event, done);
		break;
	case STATE_WIN:
		win.processInput(event, done);
		break;
	}
}

void Update(float elapsed) {
	switch (mode) {
	case STATE_MAIN_MENU:
		mainMenu.Update(elapsed);
		break;
	case STATE_LEVEL_1:
		L1.Update(elapsed);
		break;
	case STATE_LEVEL_2:
		L2.Update(elapsed);
		break;
	case STATE_LEVEL_3:
		L3.Update(elapsed);
		break;
	case STATE_GAME_OVER:
		gameOver.Update(elapsed);
		break;
	case STATE_WIN:
		win.Update(elapsed);
		break;
	}
}

void Render() {
	switch (mode) {
	case STATE_MAIN_MENU:
		mainMenu.Render();
		break;
	case STATE_LEVEL_1:
		L1.Render();
		break;
	case STATE_LEVEL_2:
		L2.Render();
		break;
	case STATE_LEVEL_3:
		L3.Render();
		break;
	case STATE_GAME_OVER:
		gameOver.Render();
		break;
	case STATE_WIN:
		win.Render();
		break;
	}
}

int main(int argc, char *argv[])
{
	setup();

	SDL_Event event;
	float lastFrameTicks = 0.0;

	bool done = false;
	while (!done) {
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		processInput(event, done);
		if (elapsed < 1) { Update(elapsed); }
		Render();
	}
	

	SDL_Quit();
	return 0;
}
