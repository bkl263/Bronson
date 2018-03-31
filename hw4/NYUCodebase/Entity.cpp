#include "Entity.h"

Entity::Entity() {}
Entity::Entity(float _x, float _y, float tileSize) : x(_x), y(_y), tileSize(tileSize) {}
Entity::Entity(float _x, float _y, float tileSize, int t_Ind, int SPRITE_COUNT_X, int SPRITE_COUNT_Y) : x(_x), y(_y), tileSize(tileSize), SPRITE_COUNT_X(SPRITE_COUNT_X), SPRITE_COUNT_Y(SPRITE_COUNT_Y), t_Ind(t_Ind) {
	u = (float)(((int)t_Ind) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
	v = (float)(((int)t_Ind) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
}
void Entity::setTex(const GLuint& tex) { texture = tex; }
void Entity::collisionBot(int tile_y) {
	y_vel = 0;
	y = tileSize * tile_y - 0.0001;
}
void Entity::collisionTop(int tile_y) {
	y_vel = 0;
	y = tileSize * (tile_y+2) + 0.0001;
}
bool Entity::OutofBounds(int mode) {
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
	t_Ind = mirrorInd;
	int temp = u;
	u = (float)(((int)t_Ind) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
	u += (1+temp);
}

float Entity::getBot() const { return bot; }
float Entity::getTop() const { return top; }
float Entity::getLeft() const { return left; }
float Entity::getRight() const { return right; }
float Entity::getCenterX() const { return x + (tileSize / 2); }
float Entity::getCenterY() const { return y - (tileSize / 2); }

void Entity::update(float elapsed, float gravity) {
	y_vel += (gravity*elapsed);

	y += y_vel;
	x += x_vel;

	timeElapsed += elapsed;

	top = y - tileSize;
	bot = y;
	left = x;
	right = x + tileSize;

}