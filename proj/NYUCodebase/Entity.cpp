#include "Entity.h"

Entity::Entity() {}
Entity::Entity(float _x, float _y, float tileSize) : x(_x), y(_y), tileSize(tileSize) {}
Entity::Entity(float _x, float _y, float tileSize, int t_Ind, int SPRITE_COUNT_X, int SPRITE_COUNT_Y, float img_width, float img_height) : x(_x), y(_y), tileSize(tileSize), SPRITE_COUNT_X(SPRITE_COUNT_X), SPRITE_COUNT_Y(SPRITE_COUNT_Y), t_Ind(t_Ind), SPRITESHEET_WIDTH(img_width), SPRITESHEET_HEIGHT(img_height) {}
void Entity::setTex(const GLuint& tex) { texture = tex; }
GLuint Entity::getTex() const { return texture; }
void Entity::draw(ShaderProgram& shader, Matrix& projectionMatrix) const {
	glUseProgram(shader.programID);

	shader.SetModelMatrix(modelMatrix);
	shader.SetProjectionMatrix(projectionMatrix);
	shader.SetViewMatrix(viewMatrix);

	float position[] = {
		tileSize * x, -tileSize * y,
		tileSize * x, (-tileSize * y) - tileSize,
		(tileSize * x) + tileSize, (-tileSize * y) - tileSize,
		tileSize * x, -tileSize * y,
		(tileSize * x) + tileSize, (-tileSize * y) - tileSize,
		(tileSize * x) + tileSize, -tileSize * y };

	float textureCord[] = {
		u, v,
		u, v + (spriteHeight),
		u + spriteWidth, v + (spriteHeight),
		u, v,
		u + spriteWidth, v + (spriteHeight),
		u + spriteWidth, v };
	glVertexAttribPointer(shader.positionAttribute, 2, GL_FLOAT, false, 0, position);
	glEnableVertexAttribArray(shader.positionAttribute);

	glVertexAttribPointer(shader.texCoordAttribute, 2, GL_FLOAT, false, 0, textureCord);
	glEnableVertexAttribArray(shader.texCoordAttribute);
	
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Entity::setOrigin(float x, float y) {
	y += tileSize;
	modelMatrix.Translate(x, y, 0);
}

void Entity::printModel() const {
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			std::cout << modelMatrix.m[i][j] << " ";
		}
		std::cout << std::endl;
	}
}

Matrix& Entity::getModel() {
	return modelMatrix;
}

void Entity::flipTexture() {
	int mirrorInd = (-(t_Ind % SPRITE_COUNT_X) + SPRITE_COUNT_X - 1) + ((t_Ind / SPRITE_COUNT_X) * SPRITE_COUNT_X);
	int temp = u;
	u = (float)(((int)mirrorInd) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
	if (u > 1) { u -= (1 + temp); }
	else { u += (1 + temp); }
}

float Entity::getCenterX() const { return x + 0.5; }
float Entity::getCenterY() const { return y - 0.5; }

void Entity::checkTileCollision(int LEVEL_WIDTH, int LEVEL_HEIGHT, std::vector<std::vector<Tile*>> tileCollisions) {
	//FLOOR COLLISIONS
	int tileX = getCenterX() - 0.2;
	int tileY = getCenterY() + 0.5;
	if (tileX > -1 && tileX < LEVEL_WIDTH && tileY > -1 && tileY < LEVEL_HEIGHT) {
		if (tileCollisions[tileY][tileX]->solid) { collisionBot(tileY); }
	}

	tileX = getCenterX() + 0.01;
	tileY = getCenterY() + 0.5;
	if (tileX > -1 && tileX < LEVEL_WIDTH && tileY > -1 && tileY < LEVEL_HEIGHT) {
		if (tileCollisions[tileY][tileX]->solid) { collisionBot(tileY); }
	}
	//CEILING COLLISIONS
	tileX = getCenterX() - 0.2;
	tileY = getCenterY() - 0.5;
	if (tileX > -1 && tileX < LEVEL_WIDTH && tileY > -1 && tileY < LEVEL_HEIGHT) {
		if (tileCollisions[tileY][tileX]->solid) { collisionTop(tileY); }
	}

	tileX = getCenterX() + 0.01;
	tileY = getCenterY() - 0.5;
	if (tileX > -1 && tileX < LEVEL_WIDTH && tileY > -1 && tileY < LEVEL_HEIGHT) {
		if (tileCollisions[tileY][tileX]->solid) { collisionTop(tileY); }
	}

	//RIGHT COLLISIONS
	tileX = getCenterX() + 0.5;
	tileY = getCenterY();
	if (tileX > -1 && tileX < LEVEL_WIDTH && tileY > -1 && tileY < LEVEL_HEIGHT) {
		if (tileCollisions[tileY][tileX]->solid) { collisionRight(tileX); }
	}

	tileX = getCenterX() - 0.5;
	//LEFT COLLISIONS
	if (tileX > -1 && tileX < LEVEL_WIDTH && tileY > -1 && tileY < LEVEL_HEIGHT) {
		if (tileCollisions[tileY][tileX]->solid) { collisionLeft(tileX); }
	}
}


void Entity::collisionBot(int tile) {
	y_vel = 0;
	y = tile;
}

void Entity::collisionTop(int tile) {
	y_vel = 0.1;
	y = tile + 2;
}

void Entity::collisionRight(int tile) {
	x_vel = 0;
	x = tile - 1;
}

void Entity::collisionLeft(int tile) {
	x_vel = 0;
	x = tile + 1;
}

void Entity::update(float elapsed, float gravity) {
	if (hasGravity) { y_vel += (gravity*elapsed); }

	y += y_vel;
	x += x_vel;

	timeElapsed += elapsed;


	u = (2 / SPRITESHEET_WIDTH) + (float)(((int)t_Ind) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
	v = (2 / SPRITESHEET_HEIGHT) + (float)(((int)t_Ind) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
	if (mirrorTEX) {
		flipTexture(); 
	}
}