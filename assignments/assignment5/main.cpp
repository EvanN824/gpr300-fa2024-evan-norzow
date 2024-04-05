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

#include "TransformHierarchy.h"

ew::Camera camera;
ew::CameraController cameraController;
const unsigned int NUM_MONKIES = 7;

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();

struct Material {
	float Ka = 1.0;
	float Kd = 0.5;
	float Ks = 0.5;
	float Shininess = 128;
}material;

//Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

int main() {
	GLFWwindow* window = initWindow("Assignment 5", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	GLuint rockTexture = ew::loadTexture("assets/Rock051_2K-JPG_Color.jpg");
	GLuint rockNormMap = ew::loadTexture("assets/Rock051_2K-JPG_NormalGL.jpg");

	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f); //Look at the center of the scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f; //Vertical field of view, in degrees

	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Model monkeyModel = ew::Model("assets/suzanne.fbx");

	//Bind brick texture to texture unit 0 
	glBindTextureUnit(0, rockTexture);
	glBindTextureUnit(1, rockNormMap);
	//Make "_MainTex" sampler2D sample from the 2D texture bound to unit 0
	shader.use();
	shader.setInt("_MainTex", 0);
	shader.setInt("_NormalMap", 1);
	shader.setVec3("_EyePos", camera.position);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); //Back face culling
	glEnable(GL_DEPTH_TEST); //Depth testing

	EN::TransformHierarchy hierarchy = EN::TransformHierarchy(NUM_MONKIES);
	hierarchy.nodes[1].setValue(glm::vec3(-4.0f, 0.0f, 0.0f), glm::vec3(0.5f), 0);
	hierarchy.nodes[2].setValue(glm::vec3(4.0f, 0.0f, 0.0f), glm::vec3(0.5f), 0);
	hierarchy.nodes[3].setValue(glm::vec3(0.0f, -4.0f, 0.0f), glm::vec3(1), 1);
	hierarchy.nodes[4].setValue(glm::vec3(0.0f, -4.0f, 0.0f), glm::vec3(1), 2);
	hierarchy.nodes[5].setValue(glm::vec3(0.0f, -4.0f, 0.0f), glm::vec3(1), 3);
	hierarchy.nodes[6].setValue(glm::vec3(0.0f, -4.0f, 0.0f), glm::vec3(1), 4);

	double sinValue = 0;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		cameraController.move(window, &camera, deltaTime);

		//All animation here
		if (sinValue < 10.0)
		{
			sinValue += deltaTime;
		}
		else
		{
			sinValue -= deltaTime;
		}

		hierarchy.nodes[0].localRotation = glm::rotate(hierarchy.nodes[0].localRotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));
		hierarchy.nodes[1].localRotation = glm::rotate(hierarchy.nodes[1].localRotation, deltaTime, glm::vec3(1.0, 0.0, 0.0));
		hierarchy.nodes[2].localRotation = glm::rotate(hierarchy.nodes[2].localRotation, deltaTime, glm::vec3(-1.0, 0.0, 0.0));
		hierarchy.nodes[5].localPosition = glm::vec3(glm::sin(sinValue) * 3, -4.0, 0.0);
		hierarchy.nodes[6].localPosition = glm::vec3(glm::sin(sinValue) * 3, -4.0, 0.0);

		hierarchy.SolveFK();

		shader.setFloat("_Material.Ka", material.Ka);
		shader.setFloat("_Material.Kd", material.Kd);
		shader.setFloat("_Material.Ks", material.Ks);
		shader.setFloat("_Material.Shininess", material.Shininess);

		//RENDER
		//Clears backbuffer color & depth values
		glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shader.use();
		//shader.setMat4("_Model", glm::mat4(1.0f));

		for (int i = 0; i < NUM_MONKIES; i++)
		{
			shader.setMat4("_Model", hierarchy.nodes[i].globalTransform);
			shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
			monkeyModel.draw(); //Draws monkey model using current shader
		}

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
