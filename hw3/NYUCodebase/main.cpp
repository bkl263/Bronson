#define NDEBUG
#define STB_IMAGE_IMPLEMENTATION
#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#include "stb_image.h"
#include <assert.h>
#include <vector>
#include "GameState.h"
#include <deque>
#include <algorithm> 

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
Matrix projectionMatrix;
ShaderProgram untextured;
ShaderProgram textured;
enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER };
GameMode mode;
int WINDOW_W = 224 * 3.5;
int WINDOW_H = 256 * 3.5;
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

class Entity {
public:
	Entity() {}
	Entity(float _x, float _y, float width, float height) : x(_x), y(_y), height(height), width(width) {}
	Entity(float _x, float _y, float width, float height, float u, float v, float t_width, float t_height) : u(u), v(v), t_width(t_width), t_height(t_height), x(_x), y(_y), height(height), width(width) {}
	void setTex(GLuint& tex) { texture = tex; }
	bool collision(Entity* rhs) {
		if (bot > rhs->top | top < rhs->bot | left > rhs->right | right < rhs->left) { return false; }
		return true;
	}
	bool OutofBounds(int mode = 0) {
		switch (mode) {
		case 0:
			if (left < -1.75 || right > 1.75 || top > 2.0 || bot < -2.0) { return true; }
			return false;
		case 1:
			if (left < -1.75) { return true; }
			return false;
		case 2:
			if (right > 1.75) { return true; }
			return false;
		case 3:
			if (top > 2.0) { return true; }
			return false;
		case 4:
			if (bot < -2.0) { return true; }
			return false;
		}
	}
	GLuint getTex() const { return texture; }
	void draw(ShaderProgram& shader) const {
		glUseProgram(shader.programID);

		shader.SetModelMatrix(modelMatrix);
		shader.SetProjectionMatrix(projectionMatrix);
		shader.SetViewMatrix(viewMatrix);

		glBindTexture(GL_TEXTURE_2D, texture);

		float position[] = {
			x - (width / 2), y - (height / 2),
			x + (width / 2), y - (height / 2),
			x + (width / 2), y + (height / 2),

			x - (width / 2), y - (height / 2),
			x + (width / 2), y + (height / 2),
			x - (width / 2), y + (height / 2) };

		float textureCord[] = {
			u - (t_width / 2), v + (t_height / 2),
			u + (t_width / 2), v + (t_height / 2),
			u + (t_width / 2), v - (t_height / 2),

			u - (t_width / 2), v + (t_height / 2),
			u + (t_width / 2), v - (t_height / 2),
			u - (t_width / 2), v - (t_height / 2) };
		glVertexAttribPointer(shader.positionAttribute, 2, GL_FLOAT, false, 0, position);
		glEnableVertexAttribArray(shader.positionAttribute);

		glVertexAttribPointer(shader.texCoordAttribute, 2, GL_FLOAT, false, 0, textureCord);
		glEnableVertexAttribArray(shader.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	void setX(float newX) { 
		x = newX; 
		left = x - (width / 2);
		right = x + (width / 2);
	}
protected:
	float u, v, x, y, width, height, t_width, t_height;

	float top = y + (height / 2);
	float bot = y - (height / 2);
	float left = x - (width / 2);
	float right = x + (width / 2);

	Matrix modelMatrix;
	Matrix viewMatrix;

	GLuint texture;
};

class Bullet : public Entity {
public:
	Bullet(float _x, float _y, float width, float height) : Entity(_x, _y, width, height) {}
	Bullet(float _x, float _y, float width, float height, float u, float v, float t_width, float t_height) : Entity(_x, _y, width, height, u, v, t_width, t_height) {}
	void update(float elapsed) {
		y += (speed * elapsed);
		top = y + (height / 2);
		bot = y - (height / 2);
	}
private:
	float speed = 4;
};

class Player : public Entity {
public:
	Player() : Entity() {}
	Player(float _x, float _y, float width, float height) : Entity(_x, _y, width, height) {}
	Player(float _x, float _y, float width, float height, float u, float v, float t_width, float t_height) : Entity(_x, _y, width, height, u, v, t_width, t_height) {}
	void draw(ShaderProgram& shader) const {
		Entity::draw(shader);
	}
	void move(float elapsed) {
		x += speed*elapsed;

		left = x - (width / 2);
		right = x + (width / 2);
	}
	void shoot(std::deque<Bullet*>& bullets) {
		if (bullets.size() == 0) {
			Bullet* newBullet = new Bullet(x, y + height / 2 + bulletHeight / 2, bulletWidth, bulletHeight, 0.32, 0.93, 0.1, 0.01);
			newBullet->setTex(texture);
			bullets.push_back(newBullet);
			lastBulletElapsed = 0;
		}
	}
	void update(float elapsed) {
		lastBulletElapsed += elapsed;
	}
private:
	float bulletDelay = 0.5;
	float lastBulletElapsed = bulletDelay + 1;
	float speed = 2.5;
	float bulletWidth = 0.009;
	float bulletHeight = 0.2;
};

class Enemy : public Entity {
public:
	Enemy() {}
	Enemy(float _x, float _y, float width, float height) : Entity(_x, _y, width, height) {}
	Enemy(float _x, float _y, float width, float height, float u, float v, float t_width, float t_height) : Entity(_x, _y, width, height, u, v, t_width, t_height) {}
	bool update(float elapsed, float step, bool y_dir) {
		timeElapsed += elapsed;
		/*
		if (timeElapsed > moveDelay) { 
			if (y_dir) { y -= 0.25; }
			else { x += step; }
			timeElapsed = 0;
			return true;
		}
		*/
		top = y + (height / 2);
		bot = y - (height / 2);
		left = x - (width / 2);
		right = x + (width / 2);
		return false;
	}
private:
	float moveDelay = 1;
	float timeElapsed = moveDelay + 1;
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
				mode = STATE_GAME_LEVEL;
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
		DrawText(&textured, textTex, "SPACE INVADERS", 0.4, -0.2, modelMatrix, viewMatrix);
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

class GameLevel : public GameState {
public:
	GameLevel() : ship(0, -1.5, 0.25, 0.13, 0.32, 0.93, 0.12, 0.075) {}
	~GameLevel() {}
	void setShipTex(GLuint& texture) { spritesheet = texture; ship.setTex(spritesheet);  }
	void makeEnemies(int row, int col) {
		float startY = 1.75;
		for (int i = 0; i < row; i++) {
			float startX = -1.6;
			std::deque<Enemy*> temp;
			for (int j = 0; j < col; j++) {
				Enemy* newEnemy = new Enemy(startX, startY, 0.25, 0.15, 0.12, 0.245, 0.24, 0.115);
				newEnemy->setTex(spritesheet);
				temp.push_back(newEnemy);
				startX += 0.25;
			}
			startY -= 0.2;
			enemies.push_back(temp);
		}
	}
	void checkHit() {
		int eROW = 0;
		for (std::deque<Enemy*> row : enemies) {
			int eCOL = 0;
			for (Enemy* enemy : row) {
				int pos = 0;
				for (Bullet* bullet : bullets) {
					if (bullet->collision(enemy)) {
						delete bullet;
						bullets.erase(bullets.begin() + pos);
						delete enemy;
						row.erase(row.begin() + eCOL);
					}
					++pos;
				}
				if (row.size() == 0) { enemies.erase(enemies.begin() + eROW); }
				++eCOL;
			}
			++eROW;
		}
	}
	void bulletOutofBounds(float elapsed) {
		int pos = 0;
		for (Bullet* bullet : bullets) {
			bullet->update(elapsed);
			if (bullet->OutofBounds(3)) { delete bullet; bullets.erase(bullets.begin() + pos); }
			++pos;
		}
	}
	bool enemyOutOfBound() {
		bool ret = false;
		for (std::deque<Enemy*> row : enemies) {
			for (Enemy* enemy : row) {
				ret |= enemy->OutofBounds();
			}
		}
		return ret;
	}
	void processInput(SDL_Event& event, bool& done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		if (keys[SDL_SCANCODE_SPACE]) {
			ship.shoot(bullets);
		}
	}
	void Render() {
		glClear(GL_COLOR_BUFFER_BIT);
		ship.draw(textured);
		for (std::deque<Enemy*> row : enemies) {
			for (Enemy* enemy : row) {
				enemy->draw(textured);
			}
		}

		for (Bullet* bullet : bullets) {
			bullet->draw(textured);
		}

		SDL_GL_SwapWindow(displayWindow);
	}
	void Update(float elapsed) {
		if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
			if (!ship.OutofBounds(2)) { ship.move(elapsed); }
		}
		if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
			if (!ship.OutofBounds(1)) { ship.move(-elapsed); }
		}
		ship.update(elapsed);
		bulletOutofBounds(elapsed);
		checkHit();

		for (std::deque<Enemy*> row : enemies) {
			for (Enemy* enemy : row) {
				enemy->update(elapsed, enemy_step, false);
			}
		}
	}
private:
	float enemy_step = 0.25;
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	std::deque<std::deque<Enemy*>> enemies;
	std::deque<Bullet*> bullets;
	GLuint spritesheet;
	Player ship;
};

MainMenu mainMenu;
GameLevel gameLevel;

void setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("SPACE INVADERS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif
	glViewport(0, 0, WINDOW_W, WINDOW_H);
	untextured.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	textured.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	projectionMatrix.SetOrthoProjection(-1.75, 1.75, -2.0f, 2.0f, -1.0f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLuint font = LoadTexture(RESOURCE_FOLDER"font1.png");
	GLuint shipTex = LoadTexture(RESOURCE_FOLDER"spaceinvadors.png");
	mainMenu.setFont(font);
	gameLevel.setShipTex(shipTex);
	gameLevel.makeEnemies(5, 11);
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
