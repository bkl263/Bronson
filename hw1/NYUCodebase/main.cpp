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
#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;


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

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif
	float lastFrameTicks = 0.0f;
	glViewport(0, 0, 1280, 720);

	ShaderProgram untextured;
	ShaderProgram textured;
	textured.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	untextured.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	
	Matrix projectionMatrix;
	projectionMatrix.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);

	GLuint marioTexture = LoadTexture(RESOURCE_FOLDER"mario.png");
	GLuint xqc = LoadTexture(RESOURCE_FOLDER"xqc0.png");
	GLuint harambeTexture = LoadTexture(RESOURCE_FOLDER"harambe.png");

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SDL_Event event;
	bool done = false;
	while (!done) {

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		
		Matrix modelMatrix;
		Matrix viewMatrix;
		untextured.SetModelMatrix(modelMatrix);
		untextured.SetProjectionMatrix(projectionMatrix);
		untextured.SetViewMatrix(viewMatrix);
		float triangle1[] = { 0.5f, -0.5f, 0.0f, 0.5f, -0.5f, -0.5f };
		glVertexAttribPointer(untextured.positionAttribute, 2, GL_FLOAT, false, 0, triangle1);
		glEnableVertexAttribArray(untextured.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		Matrix modelHarambe;
		Matrix viewHarambe;
		modelHarambe.Translate(2, 0, 0);
		modelHarambe.Scale(3, 2, 1);
		textured.SetModelMatrix(viewHarambe);
		textured.SetViewMatrix(modelHarambe);
		textured.SetProjectionMatrix(projectionMatrix);
		float harambe[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(textured.positionAttribute, 2, GL_FLOAT, false, 0, harambe);
		glEnableVertexAttribArray(textured.positionAttribute);
		float hartexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(textured.texCoordAttribute, 2, GL_FLOAT, false, 0, hartexCoords);
		glEnableVertexAttribArray(textured.texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, harambeTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		Matrix modelMario;
		Matrix viewMario;
		modelMario.Translate(-2, 0, 0);
		modelMario.Scale(1, 2, 1);
		textured.SetModelMatrix(modelMario);
		textured.SetViewMatrix(viewMario);
		float mario[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(textured.positionAttribute, 2, GL_FLOAT, false, 0, mario);
		glEnableVertexAttribArray(textured.positionAttribute);
		float mariotexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(textured.texCoordAttribute, 2, GL_FLOAT, false, 0, mariotexCoords);
		glEnableVertexAttribArray(textured.texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, marioTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		Matrix model0;
		Matrix view0;
		model0.Translate(1, 1.5, 0);
		model0.Rotate(elapsed-ticks * 2);
		textured.SetModelMatrix(model0);
		textured.SetViewMatrix(view0);
		float xqc0[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(textured.positionAttribute, 2, GL_FLOAT, false, 0, xqc0);
		glEnableVertexAttribArray(textured.positionAttribute);
		float xqctexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(textured.texCoordAttribute, 2, GL_FLOAT, false, 0, xqctexCoords);
		glEnableVertexAttribArray(textured.texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, xqc);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(untextured.positionAttribute);
		glDisableVertexAttribArray(textured.positionAttribute);
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
