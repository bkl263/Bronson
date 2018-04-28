#include "Entity.h"


Entity::Entity() {}
Entity::Entity(float _x, float _y, float tileSize) : x(_x), y(_y), tileSize(tileSize) {}
Entity::Entity(float _x, float _y, float tileSize, int t_Ind, int SPRITE_COUNT_X, int SPRITE_COUNT_Y) : x(_x), y(_y), tileSize(tileSize), SPRITE_COUNT_X(SPRITE_COUNT_X), SPRITE_COUNT_Y(SPRITE_COUNT_Y), t_Ind(t_Ind) {
	u = (float)(((int)t_Ind) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
	v = (float)(((int)t_Ind) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;


	Vector p1 = Vector(tileSize * x, -tileSize * y, 0);
	Vector p2 = Vector(tileSize * x, (-tileSize * y) - tileSize, 0);
	Vector p3 = Vector((tileSize * x) + tileSize, (-tileSize * y) - tileSize, 0);
	Vector p4 = Vector((tileSize * x) + tileSize, -tileSize * y, 0);

	points.push_back(p1);
	points.push_back(p2);
	points.push_back(p3);
	points.push_back(p4);

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
float Entity::getCenterX() const { return x + (tileSize / 2); }
float Entity::getCenterY() const { return y - (tileSize / 2); }

void Entity::update(float elapsed) {

	modelMatrix.Translate((x_vel * elapsed), (y_vel * elapsed), 0);
	timeElapsed += elapsed;
}

void Entity::centerModel() {
	float prev_x = x;
	float prev_y = y;

	x = -0.5;
	y = -0.5;

	Vector p1 = Vector(tileSize * x, -tileSize * y, 0);
	Vector p2 = Vector(tileSize * x, (-tileSize * y) - tileSize, 0);
	Vector p3 = Vector((tileSize * x) + tileSize, (-tileSize * y) - tileSize, 0);
	Vector p4 = Vector((tileSize * x) + tileSize, -tileSize * y, 0);

	points.clear();
	points.push_back(p1);
	points.push_back(p2);
	points.push_back(p3);
	points.push_back(p4);

	modelMatrix.Translate(prev_x, prev_y, 0);
}

void Entity::checkCollision(Entity* rhs) {
	std::pair<float, float> penetration;
	std::vector<std::pair<float, float>> e1Points;
	std::vector<std::pair<float, float>> e2Points;
	

	for (int i = 0; i < this->points.size(); i++) {
		Vector point = this->points[i] * this->modelMatrix;
		e1Points.push_back(std::make_pair(point.x, point.y));
	}

	for (int i = 0; i < rhs->points.size(); i++) {
		Vector point = rhs->points[i] * rhs->modelMatrix;
		e2Points.push_back(std::make_pair(point.x, point.y));
	}
	
	bool collided = CheckSATCollision(e1Points, e2Points, penetration);

	if (collided) {
		this->modelMatrix.Translate(penetration.first * 0.5f, penetration.second * 0.5f, 0);
		rhs->modelMatrix.Translate(-penetration.first * 0.5f, -penetration.second * 0.5f, 0);
	}
}