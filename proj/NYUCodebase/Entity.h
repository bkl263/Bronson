#ifndef ENTITY_H
#define ENTITY_H
#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include "Matrix.h"
#include <SDL.h>
#include "ShaderProgram.h"
#include <vector>
struct Tile { bool solid; };

class Entity {
public:
	Entity();
	Entity(float _x, float _y, float tileSize);
	Entity(float _x, float _y, float tileSize, int t_Ind, int SPRITE_COUNT_X, int SPRITE_COUNT_Y, float img_width, float img_height);
	void setTex(const GLuint& tex);
	void setOrigin(float x, float y);
	GLuint getTex() const;
	void draw(ShaderProgram& shader, Matrix& projectionMatrix) const;
	void printModel() const;
	void flipTexture();
	Matrix& getModel();
	float getCenterX() const;
	float getCenterY() const;

	void collisionBot(int tile);
	void collisionTop(int tile);
	void collisionRight(int tile);
	void collisionLeft(int tile);
	void update(float elapsed, float gravity);

	void checkTileCollision(int LEVEL_WIDTH, int LEVEL_HEIGHT, std::vector<std::vector<Tile*>> tileCollisions);
protected:
	
	bool hasGravity = true;

	int SPRITE_COUNT_X, SPRITE_COUNT_Y;

	float spriteHeight = 20 / (float)692;
	float spriteWidth = 20 / (float)692;

	float tileSize;

	float y_vel = 0;
	float x_vel = 0;

	float timeElapsed;

	//--------------------------TEXTURES-----------------------------
	int t_Ind;
	float u, v, x, y, t_width, t_height;
	float SPRITESHEET_WIDTH;
	float SPRITESHEET_HEIGHT;
	bool mirrorTEX = false;
	GLuint texture;
	//----------------------------MATRIX-----------------------------
	Matrix modelMatrix;
	Matrix viewMatrix;	
};
#endif
