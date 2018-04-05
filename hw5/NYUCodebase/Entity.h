#ifndef ENTITY_H
#define ENTITY_H
#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include "Matrix.h"
#include <SDL.h>
#include "ShaderProgram.h"
#include <vector>
#include "SatCollision.h"

struct Vector {
	Vector() {}
	Vector(const float x, const float y, const float z) : x(x), y(y), z(z) {}
	Vector operator*(Matrix& matrix) {
		return Vector(matrix.m[0][0] * x + matrix.m[1][0] * y + matrix.m[2][0] * z + matrix.m[3][0],
			matrix.m[0][1] * x + matrix.m[1][1] * y + matrix.m[2][1] * z + matrix.m[3][1],
			matrix.m[0][2] * x + matrix.m[1][2] * y + matrix.m[2][2] * z + matrix.m[3][2]);
	}

	float x, y, z;
};

class Entity {
public:
	Entity();
	Entity(float _x, float _y, float tileSize);
	Entity(float _x, float _y, float tileSize, int t_Ind, int SPRITE_COUNT_X, int SPRITE_COUNT_Y);
	void setTex(const GLuint& tex);
	void centerModel();
	float getCenterX() const;
	float getCenterY() const;
	void setOrigin(float x, float y);
	void collisionBot(int tile_y);
	void collisionTop(int tile_y);
	GLuint getTex() const;
	void draw(ShaderProgram& shader, Matrix& projectionMatrix) const;
	void printModel() const;
	void flipTexture();
	Matrix& getModel();
	void update(float elapsed);
	void checkCollision(Entity* rhs);
protected:
	float u, v, x, y, t_width, t_height;
	int t_Ind, SPRITE_COUNT_X, SPRITE_COUNT_Y;

	float spriteHeight = 1 / (float)SPRITE_COUNT_Y;
	float spriteWidth = 1 / (float)SPRITE_COUNT_X;

	float tileSize;

	std::vector<Vector> points;

	float y_vel = 0;
	float x_vel = 0;

	float timeElapsed;

	Matrix modelMatrix;
	Matrix viewMatrix;

	GLuint texture;
};
#endif
