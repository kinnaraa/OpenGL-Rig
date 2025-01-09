// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <array>
#include <stack>   
#include <sstream>
// Include GLEW
#include <GL/glew.h>
// Include GLFW
#include <GLFW/glfw3.h>
// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

const int window_width = 1024, window_height = 768;

typedef struct Vertex {
	float Position[4];
	float Color[4];
	float Normal[3];
	void SetPosition(float* coords) {
		Position[0] = coords[0];
		Position[1] = coords[1];
		Position[2] = coords[2];
		Position[3] = 1.0;
	}
	void SetColor(float* color) {
		Color[0] = color[0];
		Color[1] = color[1];
		Color[2] = color[2];
		Color[3] = color[3];
	}
	void SetNormal(float* coords) {
		Normal[0] = coords[0];
		Normal[1] = coords[1];
		Normal[2] = coords[2];
	}
};

struct Node {
	glm::mat4 localTransform;
	glm::mat4 globalTransform;
	GLuint VAO;
	GLsizei numIndices;
	std::vector<Node*> children;
	bool isSelected = false;

	void addChild(Node* child) {
		children.push_back(child);
	}
};

void updateTransforms(Node* node, const glm::mat4& parentTransform) {
	node->globalTransform = parentTransform * node->localTransform;

	for (Node* child : node->children) {
		updateTransforms(child, node->globalTransform);
	}
}

// function prototypes
int initWindow(void);
void initOpenGL(void);
void createVAOs(Vertex[], GLushort[], int);
void loadObject(char*, glm::vec4, Vertex*&, GLushort*&, int);
void createObjects(void);
void pickObject(void);
void renderScene(void);
void cleanup(void);
static void keyCallback(GLFWwindow*, int, int, int, int);
static void mouseCallback(GLFWwindow*, int, int, int);

// GLOBAL VARIABLES
GLFWwindow* window;

glm::mat4 gProjectionMatrix;
glm::mat4 gViewMatrix;

GLuint gPickedIndex = -1;
std::string gMessage;

GLuint programID;
GLuint pickingProgramID;

bool cameraSelected = false;
bool penSelected = false;

float penLongitudeAngle = 0.0f;  // J4
float penLatitudeAngle = 0.0f;   // J5
float penTwistAngle = 0.0f;      // J6

bool baseSelected = false;
float baseMovementSpeed = 0.1f;

bool arm1Selected = false;
float arm1RotationAngle = 0.0f;
float arm1RotationSpeed = 5.0f;

glm::vec3 projectilePosition;
std::vector<glm::vec3> bezierControlPoints;
bool projectileLaunched = false;
float t = 0.0f;
float stylusLength = 1.2f;
glm::vec3 gravity(0.0f, -9.81f, 0.0f);

glm::vec3 lightPos1 = glm::vec3(-2.0f, 5.0f, 5.0f);
glm::vec3 lightDiffuseColor1 = glm::vec3(1.0f, 0.6f, 0.6f);
glm::vec3 lightAmbientColor1 = glm::vec3(0.2f, 0.2f, 0.2f);
glm::vec3 lightSpecularColor1 = glm::vec3(0.8f, 0.8f, 0.8f);

glm::vec3 lightPos2 = glm::vec3(2.0f, 5.0f, -5.0f);
glm::vec3 lightDiffuseColor2 = glm::vec3(0.2f, 0.8f, 1.0f);
glm::vec3 lightAmbientColor2 = glm::vec3(0.2f, 0.2f, 0.2f);
glm::vec3 lightSpecularColor2 = glm::vec3(0.8f, 0.8f, 0.8f);

glm::vec3 objectColor = glm::vec3(1.0f, 0.5f, 0.5f);

glm::vec3 materialDiffuse = objectColor;
glm::vec3 materialAmbient = objectColor;
glm::vec3 materialSpecular = materialDiffuse * 0.1f;
float materialShininess = 20.0f;

const GLuint NumObjects = 11;	// CHANGE AS YOU ADD NEW OBJECTS
GLuint VertexArrayId[NumObjects];
GLuint VertexBufferId[NumObjects];
GLuint IndexBufferId[NumObjects];
GLuint GridArrayId[NumObjects];
GLuint GridBufferId[NumObjects];

// TL
size_t VertexBufferSize[NumObjects];
size_t IndexBufferSize[NumObjects];
size_t NumIdcs[NumObjects];
size_t NumVerts[NumObjects];

GLuint MatrixID;
GLuint ModelMatrixID;
GLuint ViewMatrixID;
GLuint ProjMatrixID;
GLuint PickingMatrixID;
GLuint pickingColorID;
GLuint LightID;

GLuint baseObjectID = 2;
GLuint topObjectID = 3;
GLuint arm1ObjectID = 4;
GLuint jointObjectID = 5;
GLuint arm2ObjectID = 6;
GLuint penObjectID = 7;
GLuint projectileObjectID = 8;

// Declare global objects
const size_t CoordVertsCount = 6;
Vertex CoordVerts[CoordVertsCount];

const size_t gridSize = 44;
Vertex* gridVerts = new Vertex[gridSize];

float horizAngle = 3.14f / 2.0f;
float vertAngle = 0.0f;
float radius = 10.0f;

float cameraSpeed = 0.05f;
glm::vec3 cameraPosition;
glm::vec3 upVector(0.0f, 1.0f, 0.0f);

bool topSelected = false;
float topRotationAngle = 0.0f;
float topRotationSpeed = 5.0f;

bool arm2Selected = false;
float arm2RotationAngle = 0.0f;

// make graph hierarchy
Node* baseNode = new Node();
Node* topNode = new Node();
Node* arm1Node = new Node();
Node* jointNode = new Node();
Node* arm2Node = new Node();
Node* penNode = new Node();
Node* projectileNode = new Node();

glm::vec3 basePosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 topOffset = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 arm1Offset = glm::vec3(0.0f, 0.2f, 0.0f);
glm::vec3 jointOffset = glm::vec3(0.0f, 0.0f, -1.2f);
glm::vec3 arm2Offset = glm::vec3(0.0f, 0.0f, -0.03f);
glm::vec3 penOffset = glm::vec3(0.0f, -0.7f, 0.71f);
glm::vec3 projectileOffset = glm::vec3(0.0f, 0.0f, 0.9f);

int initWindow(void) {
	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(window_width, window_height, "Bosworth,Kinnara (71760772)", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Set up inputs
	glfwSetCursorPos(window, window_width / 2, window_height / 2);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);

	return 0;
}

void initOpenGL(void) {
	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// Projection matrix : 45Â° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	gProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);


	// Camera matrix
	gViewMatrix = glm::lookAt(glm::vec3(10.0, 10.0, 10.0f),	// eye
		glm::vec3(0.0, 0.0, 0.0),	// center
		glm::vec3(0.0, 1.0, 0.0));	// up

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("StandardShading.vertexshader", "StandardShading.fragmentshader");
	pickingProgramID = LoadShaders("Picking.vertexshader", "Picking.fragmentshader");

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");
	ModelMatrixID = glGetUniformLocation(programID, "M");
	ViewMatrixID = glGetUniformLocation(programID, "V");
	ProjMatrixID = glGetUniformLocation(programID, "P");

	PickingMatrixID = glGetUniformLocation(pickingProgramID, "MVP");
	// Get a handle for our "pickingColorID" uniform
	pickingColorID = glGetUniformLocation(pickingProgramID, "PickingColor");
	// Get a handle for our "LightPosition" uniform
	LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

	// Define objects
	createObjects();

	// ATTN: create VAOs for each of the newly created objects here:
	VertexBufferSize[0] = sizeof(CoordVerts);
	NumVerts[0] = CoordVertsCount;

	createVAOs(CoordVerts, NULL, 0);
}

void createVAOs(Vertex Vertices[], unsigned short Indices[], int ObjectId) {
	GLenum ErrorCheckValue = glGetError();
	const size_t VertexSize = sizeof(Vertices[0]);
	const size_t RgbOffset = sizeof(Vertices[0].Position);
	const size_t Normaloffset = sizeof(Vertices[0].Color) + RgbOffset;

	// Create Vertex Array Object
	glGenVertexArrays(1, &VertexArrayId[ObjectId]);
	glBindVertexArray(VertexArrayId[ObjectId]);

	// Create Buffer for vertex data
	glGenBuffers(1, &VertexBufferId[ObjectId]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[ObjectId]);
	glBufferData(GL_ARRAY_BUFFER, VertexBufferSize[ObjectId], Vertices, GL_STATIC_DRAW);

	// Create Buffer for indices
	if (Indices != NULL) {
		glGenBuffers(1, &IndexBufferId[ObjectId]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId[ObjectId]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexBufferSize[ObjectId], Indices, GL_STATIC_DRAW);
	}

	// Assign vertex attributes
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, VertexSize, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)RgbOffset);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)Normaloffset);	// TL

	glEnableVertexAttribArray(0);	// position
	glEnableVertexAttribArray(1);	// color
	glEnableVertexAttribArray(2);	// normal

	// Disable Vertex Buffer Object 
	glBindVertexArray(0);

	ErrorCheckValue = glGetError();
	if (ErrorCheckValue != GL_NO_ERROR)
	{
		fprintf(
			stderr,
			"ERROR: Could not create a VBO: %s \n",
			gluErrorString(ErrorCheckValue)
		);
	}
}


void loadObject(char* file, glm::vec4 color, Vertex*& out_Vertices, GLushort*& out_Indices, int ObjectId) {
	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ(file, vertices, normals);

	std::vector<GLushort> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, normals, indices, indexed_vertices, indexed_normals);

	const size_t vertCount = indexed_vertices.size();
	const size_t idxCount = indices.size();

	// populate output arrays
	out_Vertices = new Vertex[vertCount];
	for (int i = 0; i < vertCount; i++) {
		out_Vertices[i].SetPosition(&indexed_vertices[i].x);
		out_Vertices[i].SetNormal(&indexed_normals[i].x);
		out_Vertices[i].SetColor(&color[0]);
	}
	out_Indices = new GLushort[idxCount];
	for (int i = 0; i < idxCount; i++) {
		out_Indices[i] = indices[i];
	}

	// set global variables!!
	NumIdcs[ObjectId] = idxCount;
	VertexBufferSize[ObjectId] = sizeof(out_Vertices[0]) * vertCount;
	IndexBufferSize[ObjectId] = sizeof(GLushort) * idxCount;
}

void createObjects(void) {
	//-- COORDINATE AXES --//
	CoordVerts[0] = { { 0.0, 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[1] = { { 5.0, 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[2] = { { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[3] = { { 0.0, 5.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[4] = { { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[5] = { { 0.0, 0.0, 5.0, 1.0 }, { 0.0, 0.0, 1.0, 1.0 }, { 0.0, 0.0, 1.0 } };

	//-- GRID --//
	int index = 0;
	for (int i = -5; i <= 5; ++i) {
		gridVerts[index++] = { { (float)i, 0.0, -5.0, 1.0 }, { 0.8, 0.8, 0.8, 1.0 }, { 0.0, 1.0, 0.0 } };
		gridVerts[index++] = { { (float)i, 0.0, 5.0, 1.0 }, { 0.8, 0.8, 0.8, 1.0 }, { 0.0, 1.0, 0.0 } };

		gridVerts[index++] = { { -5.0, 0.0, (float)i, 1.0 }, { 0.8, 0.8, 0.8, 1.0 }, { 0.0, 1.0, 0.0 } };
		gridVerts[index++] = { { 5.0, 0.0, (float)i, 1.0 }, { 0.8, 0.8, 0.8, 1.0 }, { 0.0, 1.0, 0.0 } };
	}

	VertexBufferSize[1] = sizeof(Vertex) * gridSize;
	NumVerts[1] = gridSize;
	createVAOs(gridVerts, nullptr, 1);

	//-- .OBJs --//

	Vertex* baseVerts;
	GLushort* baseIndices;
	loadObject("../common/Base2.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), baseVerts, baseIndices, baseObjectID);
	createVAOs(baseVerts, baseIndices, baseObjectID);
	baseNode->VAO = VertexArrayId[baseObjectID];
	baseNode->numIndices = NumIdcs[baseObjectID];
	baseNode->localTransform = glm::translate(glm::mat4(1.0f), basePosition);

	Vertex* topVerts;
	GLushort* topIndices;
	loadObject("../common/Top.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), topVerts, topIndices, topObjectID);
	createVAOs(topVerts, topIndices, topObjectID);
	topNode->VAO = VertexArrayId[topObjectID];
	topNode->numIndices = NumIdcs[topObjectID];
	topNode->localTransform = glm::translate(glm::mat4(1.0f), topOffset);

	Vertex* arm1Verts;
	GLushort* arm1Indices;
	loadObject("../common/Arm1.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), arm1Verts, arm1Indices, arm1ObjectID);
	createVAOs(arm1Verts, arm1Indices, arm1ObjectID);
	arm1Node->VAO = VertexArrayId[arm1ObjectID];
	arm1Node->numIndices = NumIdcs[arm1ObjectID];
	arm1Node->localTransform = glm::translate(glm::mat4(1.0f), arm1Offset);

	Vertex* jointVerts;
	GLushort* jointIndices;
	loadObject("../common/Joint.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), jointVerts, jointIndices, jointObjectID);
	createVAOs(jointVerts, jointIndices, jointObjectID);
	jointNode->VAO = VertexArrayId[jointObjectID];
	jointNode->numIndices = NumIdcs[jointObjectID];
	jointNode->localTransform = glm::translate(glm::mat4(1.0f), jointOffset);

	Vertex* arm2Verts;
	GLushort* arm2Indices;
	loadObject("../common/Arm2.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), arm2Verts, arm2Indices, arm2ObjectID);
	createVAOs(arm2Verts, arm2Indices, arm2ObjectID);
	arm2Node->VAO = VertexArrayId[arm2ObjectID];
	arm2Node->numIndices = NumIdcs[arm2ObjectID];
	arm2Node->localTransform = glm::translate(glm::mat4(1.0f), arm2Offset);

	Vertex* penVerts;
	GLushort* penIndices;
	loadObject("../common/Pen.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), penVerts, penIndices, penObjectID);
	createVAOs(penVerts, penIndices, penObjectID);
	penNode->VAO = VertexArrayId[penObjectID];
	penNode->numIndices = NumIdcs[penObjectID];
	penNode->localTransform = glm::translate(glm::mat4(1.0f), penOffset);

	Vertex* projectileVerts;
	GLushort* projectileIndices;
	loadObject("../common/Object.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), projectileVerts, projectileIndices, projectileObjectID);
	createVAOs(projectileVerts, projectileIndices, projectileObjectID);
	projectileNode->VAO = VertexArrayId[projectileObjectID];
	projectileNode->numIndices = NumIdcs[projectileObjectID];
	projectileNode->localTransform = glm::translate(glm::mat4(1.0f), projectileOffset);

	baseNode->addChild(topNode);
	topNode->addChild(arm1Node);
	arm1Node->addChild(jointNode);
	jointNode->addChild(arm2Node);
	arm2Node->addChild(penNode);
}

void pickObject(void) {
	// Clear the screen in white
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(pickingProgramID);
	{
		glm::mat4 ModelMatrix = glm::mat4(1.0); // TranslationMatrix * RotationMatrix;
		glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader, in the "MVP" uniform
		glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);

		glBindVertexArray(0);
	}
	glUseProgram(0);


	glFlush();
	glFinish();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	unsigned char data[4];
	glReadPixels(xpos, window_height - ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data); // OpenGL renders with (0,0) on bottom, mouse reports with (0,0) on top

	// Convert the color back to an integer ID
	gPickedIndex = int(data[0]);

	if (gPickedIndex == 255) { // Full white, must be the background!
		gMessage = "background";
	}
	else {
		std::ostringstream oss;
		oss << "point " << gPickedIndex;
		gMessage = oss.str();
	}
}

// Change camera position
void updateCamera() {
	cameraPosition = glm::vec3(
		radius * cos(vertAngle) * sin(horizAngle),
		radius * sin(vertAngle),
		radius * cos(vertAngle) * cos(horizAngle)
	);

	gViewMatrix = glm::lookAt(
		cameraPosition,
		glm::vec3(0.0f, 0.0f, 0.0f),
		upVector
	);
}

// Render each node in the rig heirarchy
void renderNode(Node* node) {
	glm::mat4 mvp = gProjectionMatrix * gViewMatrix * node->globalTransform;
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &node->globalTransform[0][0]);

	glUniform1i(glGetUniformLocation(programID, "isSelected"), node->isSelected ? 1 : 0);

	glBindVertexArray(node->VAO);
	glDrawElements(GL_TRIANGLES, node->numIndices, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);

	for (Node* child : node->children) {
		if (child == projectileNode && !projectileLaunched) {
			continue;
		}
		renderNode(child);
	}
}

// Move arm to impact point of the projectile
void adjustArmToTarget(const glm::vec3& impactPoint) {
	glm::vec3 penTipPos = glm::vec3(penNode->globalTransform[3]);

	float tolerance = 0.01f;
	for (int i = 0; i < 10; ++i) {
		penTipPos = glm::vec3(penNode->globalTransform[3]);
		if (glm::length(impactPoint - penTipPos) < tolerance) break;

		glm::vec3 toTarget = impactPoint - penTipPos;
		glm::vec3 toPen = penTipPos - glm::vec3(arm2Node->globalTransform[3]);
		glm::vec3 axis = glm::cross(toPen, toTarget);
		float angle = glm::acos(glm::dot(glm::normalize(toPen), glm::normalize(toTarget)));

		if (glm::length(axis) > 0.001f) {
			arm2Node->localTransform = glm::rotate(glm::mat4(1.0f), angle, axis) * arm2Node->localTransform;
			updateTransforms(baseNode, glm::mat4(1.0f));
		}

		penTipPos = glm::vec3(penNode->globalTransform[3]);
		toTarget = impactPoint - penTipPos;
		toPen = penTipPos - glm::vec3(arm1Node->globalTransform[3]);
		axis = glm::cross(toPen, toTarget);
		angle = glm::acos(glm::dot(glm::normalize(toPen), glm::normalize(toTarget)));

		if (glm::length(axis) > 0.001f) {
			arm1Node->localTransform = glm::rotate(glm::mat4(1.0f), angle, axis) * arm1Node->localTransform;
			updateTransforms(baseNode, glm::mat4(1.0f));
		}
	}
}

// Update position of projectile over time
void updateProjectile(float deltaTime) {
	if (projectileLaunched) {
		t = std::min(t + deltaTime * 1.5f, 1.0f);

		if (t >= 1.0f) {
			t = 1.0f;
			projectileLaunched = false;
			projectilePosition = bezierControlPoints[2];
			adjustArmToTarget(projectilePosition);
		}
		else {
			projectilePosition = (1 - t) * (1 - t) * bezierControlPoints[0] +
				2 * (1 - t) * t * bezierControlPoints[1] +
				t * t * bezierControlPoints[2];
		}

		projectileNode->globalTransform = glm::translate(glm::mat4(1.0f), projectilePosition);
	}
}


void launchProjectile() {
	if (!projectileLaunched) {
		glm::vec4 localTipPosition(0.0f, 0.0f, stylusLength - 0.2f, 1.0f);
		glm::vec4 worldTipPosition = penNode->globalTransform * localTipPosition;
		projectilePosition = glm::vec3(worldTipPosition);

		glm::vec3 localStylusAxis(0.0f, 1.0f, 0.0f);
		glm::vec3 worldStylusAxis = glm::vec3(penNode->globalTransform * glm::vec4(localStylusAxis, 0.0f));
		glm::vec3 stylusDirection = glm::normalize(worldStylusAxis);

		glm::vec3 C0 = projectilePosition;
		glm::vec3 C1 = C0 + stylusDirection * stylusLength * 0.5f + glm::vec3(0.0f, 2.0f, 0.0f);
		glm::vec3 C2 = glm::vec3(C0.x, 0.0f, C0.z - 1.5f);

		bezierControlPoints = { C0, C1, C2 };
		projectileLaunched = true;
		t = 0.0f;
	}
}


// Render projectile after launch
void renderProjectile() {
	if (projectileLaunched) {
		glm::mat4 projectileTransform = glm::translate(glm::mat4(1.0f), projectilePosition);
		glm::mat4 mvp = gProjectionMatrix * gViewMatrix * projectileTransform;

		glUseProgram(programID);
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &projectileTransform[0][0]);

		glBindVertexArray(projectileNode->VAO);
		glDrawElements(GL_TRIANGLES, projectileNode->numIndices, GL_UNSIGNED_SHORT, 0);
		glBindVertexArray(0);
	}
}


void renderScene(void) {
	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.2f, 0.0f);
	// Re-clear the screen for real rendering
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(programID);
	{
		glm::vec3 lightPos = glm::vec3(4, 4, 4);
		glm::mat4x4 ModelMatrix = glm::mat4(1.0);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

		// light 1
		glUniform3f(glGetUniformLocation(programID, "lightPos1"), lightPos1.x, lightPos1.y, lightPos1.z);
		glUniform3f(glGetUniformLocation(programID, "lightDiffuse1"), lightDiffuseColor1.x, lightDiffuseColor1.y, lightDiffuseColor1.z);
		glUniform3f(glGetUniformLocation(programID, "lightAmbient1"), lightAmbientColor1.x, lightAmbientColor1.y, lightAmbientColor1.z);
		glUniform3f(glGetUniformLocation(programID, "lightSpecular1"), lightSpecularColor1.x, lightSpecularColor1.y, lightSpecularColor1.z);

		// light 2
		glUniform3f(glGetUniformLocation(programID, "lightPos2"), lightPos2.x, lightPos2.y, lightPos2.z);
		glUniform3f(glGetUniformLocation(programID, "lightDiffuse2"), lightDiffuseColor2.x, lightDiffuseColor2.y, lightDiffuseColor2.z);
		glUniform3f(glGetUniformLocation(programID, "lightAmbient2"), lightAmbientColor2.x, lightAmbientColor2.y, lightAmbientColor2.z);
		glUniform3f(glGetUniformLocation(programID, "lightSpecular2"), lightSpecularColor2.x, lightSpecularColor2.y, lightSpecularColor2.z);

		// material
		glUniform3f(glGetUniformLocation(programID, "materialDiffuse"), materialDiffuse.x, materialDiffuse.y, materialDiffuse.z);
		glUniform3f(glGetUniformLocation(programID, "materialAmbient"), materialAmbient.x, materialAmbient.y, materialAmbient.z);
		glUniform3f(glGetUniformLocation(programID, "materialSpecular"), materialSpecular.x, materialSpecular.y, materialSpecular.z);
		glUniform1f(glGetUniformLocation(programID, "materialShininess"), materialShininess);

		glUniform3f(glGetUniformLocation(programID, "viewPosition"), cameraPosition.x, cameraPosition.y, cameraPosition.z);

		glBindVertexArray(VertexArrayId[0]);
		glUniform1i(glGetUniformLocation(programID, "useLighting"), false);
		glDrawArrays(GL_LINES, 0, NumVerts[0]);

		glBindVertexArray(0);

		// draw grid
		glBindVertexArray(VertexArrayId[1]);
		glUniform1i(glGetUniformLocation(programID, "useLighting"), false);
		glDrawArrays(GL_LINES, 0, NumVerts[1]);

		glBindVertexArray(0);

		// render nodes
		updateTransforms(baseNode, glm::mat4(1.0f));
		glUniform1i(glGetUniformLocation(programID, "useLighting"), true);
		renderNode(baseNode);

		// render projectile
		renderProjectile();

		glUseProgram(0);
	}
	glUseProgram(0);

	// Swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void cleanup(void) {
	// Cleanup VBO and shader
	for (int i = 0; i < NumObjects; i++) {
		glDeleteBuffers(1, &VertexBufferId[i]);
		glDeleteBuffers(1, &IndexBufferId[i]);
		glDeleteVertexArrays(1, &VertexArrayId[i]);
	}
	glDeleteProgram(programID);
	glDeleteProgram(pickingProgramID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();
}

void deselectAllParts() {
	cameraSelected = false;
	baseSelected = false;
	topSelected = false;
	arm1Selected = false;
	arm2Selected = false;
	penSelected = false;

	baseNode->isSelected = false;
	topNode->isSelected = false;
	arm1Node->isSelected = false;
	arm2Node->isSelected = false;
	penNode->isSelected = false;
}

// Keyboard events
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
		case GLFW_KEY_C:
			deselectAllParts();
			cameraSelected = true;
			printf("Camera selected\n");
			break;

		case GLFW_KEY_B:
			deselectAllParts();
			baseSelected = true;
			baseNode->isSelected = true;
			printf("Base selected\n");
			break;

		case GLFW_KEY_T:
			deselectAllParts();
			topSelected = true;
			topNode->isSelected = true;
			printf("Top selected\n");
			break;

		case GLFW_KEY_1:
			deselectAllParts();
			arm1Selected = true;
			arm1Node->isSelected = true;
			printf("Arm1 selected\n");
			break;

		case GLFW_KEY_2:
			deselectAllParts();
			arm2Selected = true;
			arm2Node->isSelected = true;
			printf("Arm2 selected\n");
			break;

		case GLFW_KEY_P:
			deselectAllParts();
			penSelected = true;
			penNode->isSelected = true;
			printf("Pen selected\n");
			break;

		// Transformations for each node
		case GLFW_KEY_LEFT:
			if (cameraSelected) horizAngle -= cameraSpeed;

			if (baseSelected) baseNode->localTransform = glm::translate(baseNode->localTransform, glm::vec3(-baseMovementSpeed, 0.0f, 0.0f));

			if (topSelected) topNode->localTransform = glm::rotate(topNode->localTransform, glm::radians(-topRotationSpeed), glm::vec3(0.0f, 1.0f, 0.0f));

			if (penSelected) {
				if (mods & GLFW_MOD_SHIFT) {
					penNode->localTransform = glm::rotate(penNode->localTransform, glm::radians(-5.0f), glm::vec3(0.0f, 0.0f, 1.0f));  // J6 twist
				}
				else {
					penNode->localTransform = glm::rotate(penNode->localTransform, glm::radians(-5.0f), glm::vec3(0.0f, 1.0f, 0.0f));  // J4 longitude
				}
			}
			break;

		case GLFW_KEY_RIGHT:
			if (cameraSelected) horizAngle += cameraSpeed;

			if (baseSelected) baseNode->localTransform = glm::translate(baseNode->localTransform, glm::vec3(baseMovementSpeed, 0.0f, 0.0f));

			if (topSelected) topNode->localTransform = glm::rotate(topNode->localTransform, glm::radians(topRotationSpeed), glm::vec3(0.0f, 1.0f, 0.0f));

			if (penSelected) {
				if (mods & GLFW_MOD_SHIFT) {
					penNode->localTransform = glm::rotate(penNode->localTransform, glm::radians(5.0f), glm::vec3(0.0f, 0.0f, 1.0f));  // J6 twist
				}
				else {
					penNode->localTransform = glm::rotate(penNode->localTransform, glm::radians(5.0f), glm::vec3(0.0f, 1.0f, 0.0f));  // J4 longitude
				}
			}
			break;

		case GLFW_KEY_UP:
			if (cameraSelected && vertAngle < glm::radians(89.0f)) vertAngle += cameraSpeed;

			if (penSelected) penNode->localTransform = glm::rotate(penNode->localTransform, glm::radians(5.0f), glm::vec3(1.0f, 0.0f, 0.0f));  // J5 latitude

			if (arm1Selected) arm1Node->localTransform = glm::rotate(arm1Node->localTransform, glm::radians(5.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // J2 rotation

			if (arm2Selected) arm2Node->localTransform = glm::rotate(arm2Node->localTransform, glm::radians(5.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // J3 rotation

			break;

		case GLFW_KEY_DOWN:
			if (cameraSelected && vertAngle > glm::radians(-89.0f)) vertAngle -= cameraSpeed;

			if (penSelected) penNode->localTransform = glm::rotate(penNode->localTransform, glm::radians(-5.0f), glm::vec3(1.0f, 0.0f, 0.0f));  // J5 latitude

			if (arm1Selected) arm1Node->localTransform = glm::rotate(arm1Node->localTransform, glm::radians(-5.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // J2 rotation

			if (arm2Selected) arm2Node->localTransform = glm::rotate(arm2Node->localTransform, glm::radians(-5.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // J3 rotation

			break;

		case GLFW_KEY_S:
			if (!projectileLaunched) {
				launchProjectile();
			}
			break;


		default:
			break;
		}

		if (cameraSelected) updateCamera();
	}
}


// Not used
static void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		pickObject();
	}
}

int main(void) {
	// Refer to https://learnopengl.com/Getting-started/Transformations, https://learnopengl.com/Getting-started/Coordinate-Systems,
	// and https://learnopengl.com/Getting-started/Camera to familiarize yourself with implementing the camera movement

	// Refer to https://learnopengl.com/Getting-started/Textures to familiarize yourself with mapping a texture
	// to a given mesh

	// Initialize window
	int errorCode = initWindow();
	if (errorCode != 0)
		return errorCode;

	// Initialize OpenGL pipeline
	initOpenGL();

	// For speed computation
	double lastTime = glfwGetTime();
	double lastFPSUpdateTime = lastTime;
	int nbFrames = 0;
	do {
		// Measure speed
		double currentTime = glfwGetTime();
		float deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		nbFrames++;
		if (currentTime - lastFPSUpdateTime >= 1.0) {
			printf("%f ms/frame\n", 1000.0 / double(nbFrames));
			nbFrames = 0;
			lastFPSUpdateTime += 1.0;
		}

		updateProjectile(deltaTime);

		// DRAWING POINTS
		renderScene();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	cleanup();

	return 0;
}
