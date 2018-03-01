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

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif


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

SDL_Window* displayWindow;
Matrix projectionMatrix;
ShaderProgram untextured;
ShaderProgram textured;
float lastFrameTicks = 0.0;

class Entity {
public:
	virtual void reset() = 0;
	Entity(float _x, float _y, float width, float height) : x(_x), y(_y), height(height), width(width) {}
	Entity(float _x, float _y, float width, float height, GLuint& _texture) : x(_x), y(_y), height(height), width(width), texture(_texture) {}

	bool collision(const Entity& rhs) {
		if (bot > rhs.top | top < rhs.bot | left > rhs.right | right < rhs.left) { return false; }
		return true;
	}

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
		glVertexAttribPointer(shader.positionAttribute, 2, GL_FLOAT, false, 0, position);
		glEnableVertexAttribArray(shader.positionAttribute);

		glVertexAttribPointer(shader.texCoordAttribute, 2, GL_FLOAT, false, 0, textureCord);
		glEnableVertexAttribArray(shader.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
protected:
	float x, y, width, height;

	float top = y + (height / 2);
	float bot = y - (height / 2);
	float left = x - (width / 2);
	float right = x + (width / 2);

	float textureCord[12] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

	Matrix modelMatrix;
	Matrix viewMatrix;

	GLuint texture;
};

class Paddle : public Entity {
public:
	Paddle(float _x, float _y, float width, float height) : Entity(_x, _y, width, height) {}
	Paddle(float _x, float _y, float width, float height, GLuint& _texture) : Entity(_x, _y, width, height, _texture) {}
	
	void move(float time) {
		y += speed*time; 
		top = y + (height / 2);
		bot = y - (height / 2);
	}
	int getScore() const { return score; }
	void reset() {
		y = 0;
		top = y + (height / 2);
		bot = y - (height / 2);
	}
	void wins() { score++; }
private:
	int score = 0;
	float speed = 5.0;
};

class Ball : public Entity {
public:
	Ball(float width, float height) : Entity(0, 0, width, height) {}
	Ball(float width, float height, GLuint& _texture) : Entity(0, 0, width, height, _texture) {}
	
	void move(float time) {
		x += x_vel;
		y += y_vel;

		top = y + (height / 2);
		bot = y - (height / 2);
		left = x - (width / 2);
		right = x + (width / 2);
	}

	void bounce(bool vertical, bool horizonal) {
		if (vertical) {
			if (y_vel < max_vel) { y_vel *= -(1.1); }
			else { y_vel *= -(1); }
		}
		if (horizonal) {
			if (x_vel < max_vel) { x_vel *= -(1.2); }
			else { x_vel *= -(1); }
		}
	}

	bool outOfBounds() {
		if (right < -3.55 | left > 3.55) { return true; }
		return false;
	}

	bool getWinner() {
		if (right < -3.55) { return true; } //if true if right wins
		return false;
	}

	void start(bool left) {
		if (!left) { x_vel *= -1; }
	}

	void reset() { 
		x = 0.0, 
		y = 0.0, 
		top = y + (height / 2);
		bot = y - (height / 2);
		left = x - (width / 2);
		right = x + (width / 2);
		x_vel = 0.002;
		y_vel = 0.001;
	}

private:
	float x_vel = 0.002;
	float y_vel = 0.001;
	float max_vel = 0.01;
};

class Boundary : public Entity {
public:
	Boundary(float _x, float _y, float width, float height) : Entity(_x, _y, width, height) {}
	Boundary(float _x, float _y, float width, float height, GLuint& _texture) : Entity(_x, _y, width, height, _texture) {}
	void reset() {}
};

void reset(std::vector<Entity*>& world) {
	for (Entity* obj : world) { obj->reset(); }
}

void setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("PONG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif
	glViewport(0, 0, 1280, 720);
	untextured.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	textured.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	projectionMatrix.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void processEvents(SDL_Event& event, bool& done) {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
	}
	
}

void update(std::vector<Entity*> world, Ball& ball, Paddle& lp, Paddle& rp, Boundary& tp, Boundary& bt, float elapsed) {
	//ball 0, LP 1, RP 2, TOP 3 , BOT 4
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_W]) {
		if (!lp.collision(tp)) { lp.move(elapsed); }
	}
	if (keys[SDL_SCANCODE_S]) {
		if (!lp.collision(bt)) { lp.move(-elapsed); }
	}

	if (keys[SDL_SCANCODE_UP]) {
		if (!rp.collision(tp)) { rp.move(elapsed); }
	}
	if (keys[SDL_SCANCODE_DOWN]) {
		if (!rp.collision(bt)) { rp.move(-elapsed); }
	}

	if (ball.collision(tp) | ball.collision(bt)) { ball.bounce(true, false); }
	if (ball.collision(rp) | ball.collision(lp)) { ball.bounce(false, true); }
	if (ball.outOfBounds()) { 
		bool winner = ball.getWinner();
		reset(world);
		ball.start(winner);
		if (winner) { rp.wins(); }
		else { lp.wins(); }
	}
	ball.move(elapsed);
}

void render(std::vector<Entity*> world, ShaderProgram& shader) {
	for (Entity* obj : world) { obj->draw(shader); }
	glDisableVertexAttribArray(untextured.positionAttribute);
	glDisableVertexAttribArray(textured.positionAttribute);
	SDL_GL_SwapWindow(displayWindow);
}
int main(int argc, char *argv[])
{
	setup();
	std::vector<Entity*> world;
	GLuint leftPaddleTex = LoadTexture(RESOURCE_FOLDER"RED.png");
	GLuint rightPaddleTex = LoadTexture(RESOURCE_FOLDER"BLUE.png");

	Paddle leftPaddle(-3.35, 0, 0.15, 1, leftPaddleTex);
	Paddle rightPaddle(3.35, 0, 0.15, 1, rightPaddleTex);
	Boundary topBarrior(0, 1.90, 8, 0.15);
	Boundary botBarrior(0, -1.90, 8, 0.15);
	Ball ball(0.15, 0.15);

	world.push_back(&ball);
	world.push_back(&leftPaddle);
	world.push_back(&rightPaddle);
	world.push_back(&topBarrior);
	world.push_back(&botBarrior);
	
	SDL_Event event;
	bool done = false;
	while (!done) {
		glClear(GL_COLOR_BUFFER_BIT);
		
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		processEvents(event, done);
		update(world, ball, leftPaddle, rightPaddle, topBarrior, botBarrior, elapsed); 
		render(world, untextured);
	}

	SDL_Quit();
	return 0;
}
