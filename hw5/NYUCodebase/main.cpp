#define NDEBUG
#define STB_IMAGE_IMPLEMENTATION
#ifdef _WINDOWS
	#include <GL/glew.h>
#endif

#include "Matrix.h"
#include "Entity.h"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "GameState.h"
#include "SatCollision.h"

#include <ctime>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <assert.h>
#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <math.h>

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

// GLOBALS
SDL_Window* displayWindow;
Matrix projectionMatrix;
Matrix viewMatrix;
ShaderProgram untextured;
ShaderProgram textured;
enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER };
GameMode mode = STATE_GAME_LEVEL;
int WINDOW_W = 1280;
int WINDOW_H = 720;
int LEVEL_HEIGHT = 16;
int LEVEL_WIDTH = 100;
float TILE_SIZE = 0.25;
float RES = WINDOW_W / WINDOW_H;

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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

class Meteor : public Entity {
public:
	Meteor() {}
	Meteor(float _x, float _y, float tileSize, int t_ind, int SPRITE_COUNT_X, int SPRITE_COUNT_Y) : Entity(_x, _y, tileSize, t_ind, SPRITE_COUNT_X, SPRITE_COUNT_Y) {
		centerModel();
		rotateRandom();
		scaleRandom();
		y_vel = MIN_VEL + ((MAX_VEL-MIN_VEL) * (float)rand() / (float)RAND_MAX);
	}

	void scaleRandom() {
		modelMatrix.Scale(randScale, randScale, 1);
	}

	void rotateRandom() {
		modelMatrix.Rotate(randRotate);
	}

private:
	int randScale = rand() % 5 + 1;
	float randRotate = 3.1415926535 * 2 * (float)rand() / (float)RAND_MAX;

	float MIN_VEL = 0.05;
	float MAX_VEL = 0.1;
};

class Player : public Entity {
public:
	Player() {}
	Player(float _x, float _y, float tileSize, int t_ind, int SPRITE_COUNT_X, int SPRITE_COUNT_Y) : Entity(_x, _y, tileSize, t_ind, SPRITE_COUNT_X, SPRITE_COUNT_Y) {}
	
	void turn(std::string dir) {
		float prev_y = y_vel;
		float prev_x = x_vel;

		if (dir == "left") {

			y_vel = (prev_y * cos(turn_speed)) - prev_x * sin(turn_speed);
			x_vel = (prev_y * sin(turn_speed)) + prev_x * cos(turn_speed);

			modelMatrix.Rotate((float)(turn_speed));		
		}
		if (dir == "right") {
			y_vel = (prev_y * cos(turn_speed)) + prev_x * sin(turn_speed);
			x_vel = -(prev_y * sin(turn_speed)) + prev_x * cos(turn_speed);

			modelMatrix.Rotate((float)(-turn_speed));
		}
	}

	void boost() {
		y_vel += accel;
	}

private:
	float turn_speed = 0.01;
	float accel = 0.005;
};


class MainMenu : public GameState {
public:
	MainMenu() {}
	void processInput(SDL_Event& event, bool& done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				mode = STATE_GAME_LEVEL;
			}
		}
	}
	void Update(float elapsed) {}
	void Render() {
		glClear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapWindow(displayWindow);
	}
private:
};

class GameLevel : public GameState {
public:
	GameLevel() {}

	void addPlayer() {
		player = Player(0, 0, 0.5, 1, 1, 1);
		player.setTex(textures[0]);
		player.centerModel();
		world.push_back(&player);
	}

	void addMeteor() {
		srand(time(NULL));
		for (size_t i = 0; i < MAX_METEOR; i++) {
			float x = -3.55 + ((3.55 * 2) * (float)rand() / (float)RAND_MAX);
			float y = -2.5 + ((2.5 * 2) * (float)rand() / (float)RAND_MAX);
			Meteor* meteor = new Meteor(x, y, 0.25, 1, 1, 1);
			int tex = rand() % 8 + 1;
			meteor->setTex(textures[tex]);
			world.push_back(meteor);
		}
	}

	void addTexture(const GLuint& textID) { textures.push_back(textID); }

	void processInput(SDL_Event& event, bool& done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}

		if (keys[SDL_SCANCODE_D]) { player.turn("right"); }
		if (keys[SDL_SCANCODE_A]) { player.turn("left"); }
		if (keys[SDL_SCANCODE_W]) { player.boost(); }
	}
	void Render() {
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		for (Entity* e : world) {
			e->draw(textured, projectionMatrix);
		}
		SDL_GL_SwapWindow(displayWindow);
	}
	void Update(float elapsed) {
		for (Entity* self : world) {
			self->update(elapsed);
		}
		for (int i = 1; i < world.size(); ++i) {
			world[0]->checkCollision(world[i]);
		}

	}
private:
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	std::vector<GLuint> textures;

	Player player;
	std::vector<Entity*> world;

	int MAX_METEOR = 5;
	float gravity = 0.03;
};

MainMenu mainMenu;
GameLevel gameLevel;


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

	const GLuint playerTex = LoadTexture(RESOURCE_FOLDER"playerShip1_green.png"); // 0
	const GLuint meteorB1 = LoadTexture(RESOURCE_FOLDER"meteorBrown_big1.png"); // 1
	const GLuint meteorB2 = LoadTexture(RESOURCE_FOLDER"meteorBrown_big2.png");
	const GLuint meteorB3 = LoadTexture(RESOURCE_FOLDER"meteorBrown_big3.png");
	const GLuint meteorB4 = LoadTexture(RESOURCE_FOLDER"meteorBrown_big4.png");
	const GLuint meteorG1 = LoadTexture(RESOURCE_FOLDER"meteorGrey_big1.png");
	const GLuint meteorG2 = LoadTexture(RESOURCE_FOLDER"meteorGrey_big2.png");
	const GLuint meteorG3 = LoadTexture(RESOURCE_FOLDER"meteorGrey_big3.png");
	const GLuint meteorG4 = LoadTexture(RESOURCE_FOLDER"meteorGrey_big4.png"); // 8


	gameLevel.addTexture(playerTex);
	gameLevel.addTexture(meteorB1);
	gameLevel.addTexture(meteorB2);
	gameLevel.addTexture(meteorB3);
	gameLevel.addTexture(meteorB4);
	gameLevel.addTexture(meteorG1);
	gameLevel.addTexture(meteorG2);
	gameLevel.addTexture(meteorG3);
	gameLevel.addTexture(meteorG4);

	gameLevel.addPlayer();
	gameLevel.addMeteor();
}

void processInput(SDL_Event& event, bool& done) {
	switch (mode) {
	case STATE_MAIN_MENU:
		mainMenu.processInput(event, done);
		break;
	case STATE_GAME_LEVEL:
		gameLevel.processInput(event, done);
		break;
	}
}

void Update(float elapsed) {
	switch (mode) {
	case STATE_MAIN_MENU:
		mainMenu.Update(elapsed);
		break;
	case STATE_GAME_LEVEL:
		gameLevel.Update(elapsed);
		break;
	}
}

void Render() {
	switch (mode) {
	case STATE_MAIN_MENU:
		mainMenu.Render();
		break;
	case STATE_GAME_LEVEL:
		gameLevel.Render();
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
		Update(elapsed);
		Render();
	}
	

	SDL_Quit();
	return 0;
}
