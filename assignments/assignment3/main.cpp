#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>

#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
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

struct FrameBuffer {
	unsigned int fbo;
	unsigned int colorBuffer[8];
	unsigned int depthBuffer;
	unsigned int width;
	unsigned int height;
};

FrameBuffer createGBuffer(unsigned int width, unsigned int height);

float _gamma = 2.2f;
glm::vec3 lightDir = glm::vec3(0,-1,0);
const float lightDist = 7;

//Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

//FrameBuffer stuffs
unsigned int postProcessVAO, deferredLitVAO, colorBuffer;

unsigned int shadowfbo, shadowMap;

FrameBuffer gBuffer;

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
	glm::mat4 lightMatrix;

	ew::Shader depthShader = ew::Shader("assets/depthOnly.vert", "assets/depthOnly.frag");
	ew::Shader geometryPass = ew::Shader("assets/geoPass.vert", "assets/geoPass.frag");
	ew::Shader litPass = ew::Shader("assets/deferredLit.vert", "assets/deferredLit.frag");
	ew::Shader postprocess = ew::Shader("assets/postprocess.vert", "assets/postprocess.frag");

	ew::Model monkeyModel = ew::Model("assets/suzanne.fbx");
	ew::Mesh planeMesh = ew::Mesh(ew::createPlane(10, 10, 5));

	//creates your shadow buffer yay
	makeShadowBuffer();

	gBuffer = createGBuffer(screenWidth, screenHeight);

	//Requires a depth buffer to work.
	glEnable(GL_DEPTH_TEST);

	//For dummy VAO
	//glCreateVertexArrays(1, &dummyVAO);

	glEnable(GL_DEPTH_TEST); //Depth testing

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		cameraController.move(window, &camera, deltaTime);

		lightCam.position = (lightDir * -1.0f) * lightDist;
	    lightMatrix = lightCam.projectionMatrix() * lightCam.viewMatrix();

		//Rotate model around Y axis
		monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

		//Scoot plane transform down
		planeTransform.position = glm::vec3(0.0, -3.0, 0.0);


		//Draw Shadow
		glBindFramebuffer(GL_FRAMEBUFFER, shadowfbo);
		//Don't use screenwidth and height, use the same as the other stuff
		glViewport(0, 0, 2048, 2048);
		glClear(GL_DEPTH_BUFFER_BIT);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);

		depthShader.use();
		depthShader.setMat4("_Model", monkeyTransform.modelMatrix());
		depthShader.setMat4("_ViewProjection", lightMatrix);
		monkeyModel.draw(); //Draws monkey model using current shader
		depthShader.setMat4("_Model", planeTransform.modelMatrix());
		planeMesh.draw();

		//glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//BIND FBO / FRAMEBUFFER
		//Clears framebuffer color & depth values
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.fbo);
		glViewport(0, 0, gBuffer.width, gBuffer.height);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//DRAW SCENE
		glBindTextureUnit(0, rockTexture);
		glBindTextureUnit(1, rockNormMap);
		//Bind brick texture to texture unit 0 
		//Make "_MainTex" sampler2D sample from the 2D texture bound to unit 0
		geometryPass.setInt("_MainTex", 0);
		geometryPass.setInt("_NormalMap", 1);
		geometryPass.setVec3("_EyePos", camera.position);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		geometryPass.use();
		geometryPass.setMat4("_Model", monkeyTransform.modelMatrix());
		geometryPass.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		monkeyModel.draw(); //Draws monkey model using current shader
		geometryPass.setMat4("_Model", planeTransform.modelMatrix());
		planeMesh.draw();

		//drawing to colorbuffer because it be gamer time.
		glBindFramebuffer(GL_FRAMEBUFFER, colorBuffer);
		glViewport(0, 0, gBuffer.width, gBuffer.height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		litPass.use();
		litPass.setMat4("_LightViewProj", lightMatrix);
		litPass.setVec3("_LightDirection", lightDir);
		litPass.setFloat("_Material.Ka", material.Ka);
		litPass.setFloat("_Material.Kd", material.Kd);
		litPass.setFloat("_Material.Ks", material.Ks);
		litPass.setFloat("_Material.Shininess", material.Shininess);

		//Bind g-buffer textures
		glBindTextureUnit(0, gBuffer.colorBuffer[0]);
		glBindTextureUnit(1, gBuffer.colorBuffer[1]);
		glBindTextureUnit(2, gBuffer.colorBuffer[2]);
		glBindTextureUnit(3, shadowMap); //For shadow mapping
		//dunno if that's necessary.
		litPass.setInt("_gPositions", 0);
		litPass.setInt("_gNormals", 1);
		litPass.setInt("_gAlbedo", 2);
		litPass.setInt("_ShadowMap", 3);


		//Dummy VAO is a vector array object that we write to, to create the fullscreen triangle
		glBindVertexArray(deferredLitVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//POSTPROCESS SHADER (Sample from colorbuffer)
		postprocess.use();
		postprocess.setFloat("_Gamma", _gamma);

		//DRAW FULLSCREEN TRIANGLE
		glBindTextureUnit(0, colorBuffer);
		postprocess.setInt("_ColorBuffer", 0);
		glBindVertexArray(postProcessVAO);
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
	//LightDir
	if (ImGui::SliderFloat3("Light Direction", glm::value_ptr(lightDir), -1.0f, 1.0f))
	{
		if (glm::length(lightDir) > 0)
		{
			glm::normalize(lightDir);
		}
	}
	ImGui::End();

	ImGui::Begin("GBuffers");
		ImVec2 texSize = ImVec2(gBuffer.width / 4, gBuffer.height / 4);
		for (size_t i = 0; i < 3; i++)
		{
			ImGui::Image((ImTextureID)gBuffer.colorBuffer[i], texSize, ImVec2(0, 1), ImVec2(1, 0));
		}
	ImGui::End();


	//Expects the start of a float array
	//Use ImGui::SliderFloat3("name", &lightdirection (it's a glm vec3), -1.0f, 1.0f;
	//If on imgui returns true if something has changed, so do if (above), normalize the vector.
	//if glm::length(lightdir) > 0 then do it

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
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
}

FrameBuffer createGBuffer(unsigned int width, unsigned int height)
{
	FrameBuffer f_buffer;
	f_buffer.width = width;
	f_buffer.height = height;

	glCreateFramebuffers(1, &f_buffer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, f_buffer.fbo);

	int formats[3] = {
		GL_RGB32F, //0 = World Position
		GL_RGB16F, //1 = World Normal
		GL_RGB16F //2 = Alebdo
	};

	//Create 3 color textures
	for (size_t i = 0; i < 3; i++)
	{
		glGenTextures(1, &f_buffer.colorBuffer[i]);
		glBindTexture(GL_TEXTURE_2D, f_buffer.colorBuffer[i]);
		glTexStorage2D(GL_TEXTURE_2D, 1, formats[i], width, height);
		//Clamp to border, no wrapping.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//Attach each texture to a different slot.
		//GL_COLOR_ATTACHMENT0 + 1 = GL_COLOR_ATTACHMENT1, etc
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, f_buffer.colorBuffer[i], 0);
	}
	//Tell opengl which color attachments we're drawing to
	const GLenum drawBuffers[3] = {
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
	};
	glDrawBuffers(3, drawBuffers);

	//TODO: Add texture2D depth buffer.
	//16 bit Depth buffer
	glGenTextures(1, &f_buffer.depthBuffer);
	//Swap with a renderbuffer object if you don't wanna sample
	glBindTexture(GL_TEXTURE_2D, f_buffer.depthBuffer);
	//MAKE SURE COLOR AND DEPTH BUFFERS ARE OF IDENTICAL DIMENSIONS
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, width, height);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, f_buffer.depthBuffer, 0);


	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer incomplete: %d", fboStatus);
		exit(0);
	}

	//Clean up global state
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return f_buffer;
}
