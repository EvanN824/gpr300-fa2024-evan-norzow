#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/cameraController.h>
#include <ew/texture.h>
#include <ew/procGen.h>


ew::Camera camera;
ew::Camera lightCam;
ew::CameraController cameraController;
ew::Transform monkeyTransform;
ew::Transform planeTransform;

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();
void makeShadowBuffer();

struct Material {
	float Ka = 1.0;
	float Kd = 0.5;
	float Ks = 0.5;
	float Shininess = 128;
}material;

float _gamma = 2.2f;

//Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

//FrameBuffer stuffs
unsigned int fbo, colorBuffer, depthBuffer, dummyVAO;

unsigned int shadowfbo, shadowMap;

int main() {
	GLFWwindow* window = initWindow("Assignment 2", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	GLuint rockTexture = ew::loadTexture("assets/Rock051_2K-JPG_Color.jpg");
	GLuint rockNormMap = ew::loadTexture("assets/Rock051_2K-JPG_NormalGL.jpg");

	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f); //Look at the center of the scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f; //Vertical field of view, in degrees

	lightCam.position = glm::vec3(0.0f, 5.0f, 5.0f); //Possibly change later
	lightCam.target = glm::vec3(0.0f, 0.0f, 0.0f);
	lightCam.aspectRatio = 1;
	lightCam.orthographic = true;
	lightCam.orthoHeight = 10.0f;

	//lightcam.viewmatrix
	glm::mat4 lightMatrix = lightCam.projectionMatrix() * lightCam.viewMatrix();

	ew::Shader depthShader = ew::Shader("assets/depthOnly.vert", "assets/depthOnly.frag");
	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader postprocess = ew::Shader("assets/postprocess.vert", "assets/postprocess.frag");
	ew::Model monkeyModel = ew::Model("assets/suzanne.fbx");
	ew::Mesh planeMesh = ew::Mesh(ew::createPlane(10, 10, 5));

	//creates your shadow buffer yay
	makeShadowBuffer();

	//FrameBuffer object creation
	glCreateFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	//8 bit RGBA color buffer
	glGenTextures(1, &colorBuffer);
	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, screenWidth, screenHeight);

	//16 bit Depth buffer
	glGenTextures(1, &depthBuffer);
	//Swap with a renderbuffer object if you don't wanna sample
	glBindTexture(GL_TEXTURE_2D, depthBuffer);
	//MAKE SURE COLOR AND DEPTH BUFFERS ARE OF IDENTICAL DIMENSIONS
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, screenWidth, screenHeight);

	//attach colorbuffer and depthbuffer to framebuffer
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorBuffer, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffer, 0);

	//Requires a depth buffer to work.
	glEnable(GL_DEPTH_TEST);

	//For dummy VAO
	glCreateVertexArrays(1, &dummyVAO);

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer incomplete: %d", fboStatus);
		return 0;
	}


	//Bind brick texture to texture unit 0 
	glBindTextureUnit(1, rockNormMap);
	//Make "_MainTex" sampler2D sample from the 2D texture bound to unit 0
	shader.use();
	shader.setInt("_MainTex", 0);
	shader.setInt("_NormalMap", 1);
	shader.setVec3("_EyePos", camera.position);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); //Back face culling
	glEnable(GL_DEPTH_TEST); //Depth testing

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		cameraController.move(window, &camera, deltaTime);

		//Rotate model around Y axis
		monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

		//Scoot plane transform down
		planeTransform.position = glm::vec3(0.0, -3.0, 0.0);

		//transform.modelMatrix() combines translation, rotation, and scale into a 4x4 model matrix

		shader.setFloat("_Material.Ka", material.Ka);
		shader.setFloat("_Material.Kd", material.Kd);
		shader.setFloat("_Material.Ks", material.Ks);
		shader.setFloat("_Material.Shininess", material.Shininess);
		postprocess.setFloat("_Gamma", _gamma);

		//Draw Shadow
		glBindFramebuffer(GL_FRAMEBUFFER, shadowfbo);
		glViewport(0, 0, screenWidth, screenHeight);
		glClear(GL_DEPTH_BUFFER_BIT);

		depthShader.use();
		depthShader.setMat4("_Model", monkeyTransform.modelMatrix());
		depthShader.setMat4("_ViewProjection", lightMatrix);
		monkeyModel.draw(); //Draws monkey model using current shader
		depthShader.setMat4("_Model", planeTransform.modelMatrix());
		planeMesh.draw();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//BIND FBO / FRAMEBUFFER
		//Clears framebuffer color & depth values
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, screenWidth, screenHeight);
		glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//DRAW SCENE
		glBindTextureUnit(0, rockTexture);
		shader.use();
		shader.setMat4("_Model", monkeyTransform.modelMatrix());
		shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		monkeyModel.draw(); //Draws monkey model using current shader
		shader.setMat4("_Model", planeTransform.modelMatrix());
		planeMesh.draw();

		//UNBIND FBO
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//POSTPROCESS SHADER (Sample from colorbuffer)
		postprocess.use();

		//DRAW FULLSCREEN TRIANGLE
		glBindTextureUnit(0, colorBuffer);
		glBindVertexArray(dummyVAO);
		//3 vertices because triangle
		glDrawArrays(GL_TRIANGLES, 0, 3);


		drawUI();

		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}

void resetCamera(ew::Camera* camera, ew::CameraController* controller) {
	camera->position = glm::vec3(0, 0, 5.0f);
	camera->target = glm::vec3(0);
	controller->yaw = controller->pitch = 0;
}


void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");
	if (ImGui::Button("Reset Camera")) {
		resetCamera(&camera, &cameraController);
	}
	//Others
	if (ImGui::CollapsingHeader("Material")) {
		ImGui::SliderFloat("AmbientK", &material.Ka, 0.0f, 1.0f);
		ImGui::SliderFloat("DiffuseK", &material.Kd, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularK", &material.Ks, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);
	}
	//Gamma
	if (ImGui::CollapsingHeader("Gamma_Settings"))
	{
		ImGui::SliderFloat("Gamma_Value", &_gamma, 2.0f, 2.5f);
	} 
	ImGui::End();

	ImGui::Begin("Shadow Map");
	//Using a Child allow to fill all the space of the window.
	ImGui::BeginChild("Shadow Map");
	//Stretch image to be window size
	ImVec2 windowSize = ImGui::GetWindowSize();
	//Invert 0-1 V to flip vertically for ImGui display
	//shadowMap is the texture2D handle
	ImGui::Image((ImTextureID)shadowMap, windowSize, ImVec2(0, 1), ImVec2(1, 0));
	ImGui::EndChild();
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;
}

/// <summary>
/// Initializes GLFW, GLAD, and IMGUI
/// </summary>
/// <param name="title">Window title</param>
/// <param name="width">Window width</param>
/// <param name="height">Window height</param>
/// <returns>Returns window handle on success or null on fail</returns>
GLFWwindow* initWindow(const char* title, int width, int height) {
	printf("Initializing...");
	if (!glfwInit()) {
		printf("GLFW failed to init!");
		return nullptr;
	}

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		printf("GLFW failed to create window");
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		printf("GLAD Failed to load GL headers");
		return nullptr;
	}

	//Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}

void makeShadowBuffer()
{
	//ShadowMap depth buffer creation
	glCreateFramebuffers(1, &shadowfbo);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowfbo);
	glGenTextures(1, &shadowMap);
	glBindTexture(GL_TEXTURE_2D, shadowMap);
	//16 bit depth values, 2k res.
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, 2048, 2048);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//pixels outside frustum should be white (max dist)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
}

