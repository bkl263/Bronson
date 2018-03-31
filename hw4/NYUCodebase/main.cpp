#define NDEBUG
#define STB_IMAGE_IMPLEMENTATION
#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "Entity.h"
#include "ShaderProgram.h"
#include "stb_image.h"
#include <assert.h>
#include <vector>
#include "GameState.h"
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include "FlareMap.h"

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
void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	*gridX = (int)(worldX / (2 * TILE_SIZE));
	*gridY = (int)(worldY / (2 * TILE_SIZE));
}

class Player : public Entity {
public:
	Player() {}
	Player(float _x, float _y, float tileSize, int t_ind, int SPRITE_COUNT_X, int SPRITE_COUNT_Y) : Entity(_x, _y, tileSize, t_ind, SPRITE_COUNT_X, SPRITE_COUNT_Y) {}
	void move(const std::string& dir) {
		if (dir == "right") { 
			if (!facingRight) { flipTexture(); facingRight = true; }
			x_vel = speed; }
		if (dir == "left") {
			if (facingRight) { flipTexture(); facingRight = false; }
			x_vel = -speed;
		}
		if (dir == "stop") {
			x_vel = 0;
		}
	}
	void jump() {
		if (onFloor) {
			y_vel = -0.02;
			onFloor = false;
		}
	}

	void landed() { onFloor = true; }
private:
	bool alive;
	bool facingRight = true;
	float speed = 0.01;
	
	bool onFloor = false;
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
	GameLevel() { map.Load("map.txt");}

	void addPlayer() {
		for (const FlareMapEntity& e : map.entities) {
			if (e.type == "Start") {
				player = Player(e.x / 2, e.y / 2, TILE_SIZE * 2, 8, 8, 4);
				player.setTex(textures[1]);
				player.setOrigin(-3.55, 2);
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
		if (!keys[SDL_SCANCODE_D] && !keys[SDL_SCANCODE_A]) { player.move("stop"); }
		if (keys[SDL_SCANCODE_D]) {
			player.move("right");
		}
		if (keys[SDL_SCANCODE_A]) {
			player.move("left");
		}
		if (keys[SDL_SCANCODE_W]) {
			player.jump();
		}
		if (keys[SDL_SCANCODE_SPACE]) {
			int x = 0;
			int y = 0;
			worldToTileCoordinates(player.getCenterX(), player.getTop(), &x, &y);
			std::cout << x << " " << y << std::endl;

		}
	}
	void Render() {
		glClearColor(0, 140.0f/255.0f, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		drawLevel();
		player.draw(textured, projectionMatrix);
		SDL_GL_SwapWindow(displayWindow);
	}
	void Update(float elapsed) {
		player.update(elapsed, gravity);
		int x = 0;
		int y = 0;
		worldToTileCoordinates(player.getCenterX(), player.getBot(), &x, &y);
		if (x > -1 && x < LEVEL_WIDTH && y > -1 && y < LEVEL_HEIGHT) {
			if (map.mapData[y][x] != 0) { player.collisionBot(y); player.landed(); }
		}

		worldToTileCoordinates(player.getCenterX(), player.getTop(), &x, &y);
		if (x > -1 && x < LEVEL_WIDTH && y > -1 && y < LEVEL_HEIGHT) {
			if (map.mapData[y][x] != 0) { player.collisionTop(y);}
		}

		worldToTileCoordinates(player.getRight(), player.getCenterY(), &x, &y);
		if (x > -1 && x < LEVEL_WIDTH && y > -1 && y < LEVEL_HEIGHT) {
			if (map.mapData[y][x] != 0) { player.collisionTop(y); }
		}
	}

	void drawLevel() const {
		int SPRITE_COUNT_X = 16;
		int SPRITE_COUNT_Y = 8;
		std::vector<float> vertexData;
		std::vector<float> texCoordData;
		for (int y = 0; y < LEVEL_HEIGHT; y++) {
			for (int x = 0; x < LEVEL_WIDTH; x++) {
				if (map.mapData[y][x] != 0) {
					// add vertices
					float u = (float)(((int)map.mapData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
					float v = (float)(((int)map.mapData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
					float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
					float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
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
		}
		Matrix modelMatrix;
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
			glBindTexture(GL_TEXTURE_2D, textures[0]);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}
private:
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	std::vector<GLuint> textures;

	Player player;
	std::vector<Entity> world;

	float gravity = 0.03;
	FlareMap map;
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

	const GLuint font = LoadTexture(RESOURCE_FOLDER"font1.png");
	const GLuint playerTex = LoadTexture(RESOURCE_FOLDER"characters_3.png");
	const GLuint tiles = LoadTexture(RESOURCE_FOLDER"arne_sprites.png");

	gameLevel.addTexture(tiles);
	gameLevel.addTexture(playerTex);
	gameLevel.addPlayer();

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
