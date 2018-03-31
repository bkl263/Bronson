#ifndef ENTITY_H
#define ENTITY_H
#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include "Matrix.h"
#include <SDL.h>
#include "ShaderProgram.h"

class Entity {
public:
	Entity();
	Entity(float _x, float _y, float tileSize);
	Entity(float _x, float _y, float tileSize, int t_Ind, int SPRITE_COUNT_X, int SPRITE_COUNT_Y);
	void setTex(const GLuint& tex);
	float getBot() const;
	float getTop() const;
	float getCenterX() const;
	float getCenterY() const;
	float getLeft() const;
	float getRight() const;
	void setOrigin(float x, float y);
	void collisionBot(int tile_y);
	void collisionTop(int tile_y);
	bool OutofBounds(int mode = 0);
	GLuint getTex() const;
	void draw(ShaderProgram& shader, Matrix& projectionMatrix) const;
	void printModel() const;
	void flipTexture();
	Matrix& getModel();
	void update(float elapsed, float gravity);
protected:
	float u, v, x, y, t_width, t_height;
	int t_Ind, SPRITE_COUNT_X, SPRITE_COUNT_Y;

	float spriteHeight = 1 / (float)SPRITE_COUNT_Y;
	float spriteWidth = 1 / (float)SPRITE_COUNT_X;

	float tileSize;

	float top = y;
	float bot = y - tileSize;
	float left = x;
	float right = x + tileSize;

	float y_vel = 0;
	float x_vel = 0;

	float timeElapsed;

	Matrix modelMatrix;
	Matrix viewMatrix;

	GLuint texture;
};
#endif
