//Membros: Guilherme Barth, Robson Chiarello, Willian Werlang

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <GL\glew.h>
#include <GLM\glm.hpp>
#include <GLM\gtc\matrix_transform.hpp>
#include <GLM\gtc\type_ptr.hpp>
#include <GLM\vec2.hpp>
#include <GLFW\glfw3.h>

#define PI  3.14159265359
#define HALF_PI PI/2.0

using namespace std;

const GLint WIDTH = 1200, HEIGHT = 900;
vector<glm::vec3*>* selectedPoints = new vector<glm::vec3*>();
vector<glm::vec3*>* originalCurve = new vector<glm::vec3*>();
vector<glm::vec3*>* externalCurve = new vector<glm::vec3*>();
vector<glm::vec3*>* internalCurve = new vector<glm::vec3*>();
vector<glm::vec3*>* finalPoints = new vector<glm::vec3*>();
vector<GLfloat>* finalPointsFloat = new vector<GLfloat>();

//variables to write indexes of faces on the obj
int internalCurveSize = 0;
int externalCurveSize = 0;

int faces = 0;

GLuint vao, vbo;
bool draw = false;

void mouseCallback(GLFWwindow* window, int button, int action, int mods);
void convertCoordinates(double &x, double &y);
int getZone(float x, float y);
vector<glm::vec3*>* generateCurve(vector<glm::vec3*>* points);
vector<glm::vec3*>* generateExternalCurve(vector<glm::vec3*>* points, bool external);
vector<glm::vec3*>* generateFinalVector(vector<glm::vec3*>* internalCurve, vector<glm::vec3*>* externalCurve);
vector<GLfloat>* convertToFloat(std::vector<glm::vec3*>* points);
void saveTextureValuesToOBJ();

int main() {

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Hotwheels", nullptr, nullptr);

	int screenWidth, screenHeight;
	glfwGetFramebufferSize(window, &screenWidth, &screenHeight);

	if (window == nullptr) {
		std::cout << "Failed to create GLFW Window" << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK) {
		std::cout << "Failed no init GLEW." << std::endl;
		return EXIT_FAILURE;
	}

	glViewport(0, 0, screenWidth, screenHeight);

	const char* fragment_shader =
		"#version 410\n"
		"in vec3 color;"
		"out vec4 frag_color;"
		"void main () {"
		" frag_color = vec4 (color, 1.0);"
		"}";

	const char* vertex_shader =
		"#version 410\n"
		"layout(location=0) in vec3 vp;"
		"layout(location=1) in vec3 vc;"
		"out vec3 color;"
		"void main () {"
		" color = vc;"
		" gl_Position = vec4 (vp, 1.0);"
		"}";

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertex_shader, NULL);
	glCompileShader(vs);

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fragment_shader, NULL);
	glCompileShader(fs);

	GLuint shader_programme = glCreateProgram();
	glAttachShader(shader_programme, fs);
	glAttachShader(shader_programme, vs);
	glLinkProgram(shader_programme);

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glUseProgram(shader_programme);

	glfwMakeContextCurrent(window);
	glfwSetMouseButtonCallback(window, mouseCallback);

	ofstream arc;
	arc.open("curve.obj"); //if no mode is specified, the file is open on the beggining
	//arc << "mtllib " << '"' << "C:/Users/I852885/Desktop/UNISINOS/7 semestre/CG/projetos/curva/Sabertooth/Sabertooth/curve.mtl" << '"' << "\n" << endl;
	arc << "mtllib " << "curve.mtl" << "\n" << endl;
	arc << "g " << "road" << "\n" << endl;
	arc << "usemtl road\n" << endl;
	arc.close();

	ofstream arc2;
	arc2.open("curve.mtl"); //if no mode is specified, the file is open on the beggining
	arc2 << "newmtl " << "road\n" << endl;
	arc2 << "Kd " << 0.50 << " " << 0.50 << " " << 0.50 << endl;
	arc2 << "Ka " << 0.15 << " " << 0.15 << " " << 0.15 << endl;
	arc2 << "Tf " << 1.0 << " " << 1.0 << " " << 1.0 << endl;
	arc2 << "map_Kd " << "road.png" << endl;
	arc2 << "Ni " << 1.0 << endl;
	arc2 << "Ns " << 1.0 << endl;
	arc2 << "Ks " << 1.0 << " " << 1.0 << " " << 1.0 << endl;
	arc2.close();

	while (!glfwWindowShouldClose(window)) {

		glClear(1);
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (draw == true) {
			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLES, 0, finalPointsFloat->size());
		}

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return EXIT_SUCCESS;
}

void mouseCallback(GLFWwindow* window, int button, int action, int mods) {

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		convertCoordinates(xpos, ypos);

		glm::vec3* point = new glm::vec3(xpos, ypos, 0.0);
		selectedPoints->push_back(point);
		cout << "Point recorded:" << endl;
		cout << xpos << endl;
		cout << ypos << endl;

		int zone = getZone(xpos, ypos);
		if (zone == 1) {
			xpos += 0.5;
			ypos += 0.5;
		}
		else if (zone == 2) {
			xpos -= 0.5;
			ypos += 0.5;
		}
		else if (zone == 3) {
			xpos -= 0.5;
			ypos -= 0.5;
		}
		else if (zone == 4) {
			xpos += 0.5;
			ypos -= 0.5;
		}
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		draw = true;

		originalCurve = generateCurve(selectedPoints);
		externalCurve = generateExternalCurve(originalCurve, true);
		internalCurve = generateExternalCurve(originalCurve, false);

		externalCurveSize = externalCurve->size()/2.0;
		internalCurveSize = internalCurve->size()/2.0;

		//writes the texture points to the obj
		saveTextureValuesToOBJ();

		finalPoints = generateFinalVector(internalCurve, externalCurve);

		//after the faces are calculated, generate the normals and write them on the OBJ
		//saveNormalValuesToOBJ(finalPoints);

		finalPointsFloat= convertToFloat(finalPoints);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*finalPointsFloat->size(), &finalPointsFloat->at(0), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	}
}

void convertCoordinates(double &x, double &y) {
	// Convert resolution coordinates to graph coordinates
	if (x > (WIDTH / 2)) {
		x = x - (WIDTH / 2);
		x = x / (WIDTH / 2);
	}
	else if (x == (WIDTH / 2)) {
		x = 0;
	}
	else {
		x = -(((WIDTH / 2) - x) / (WIDTH / 2));
	}

	if (y > (HEIGHT / 2)) {
		y = y - (HEIGHT / 2);
		y = y / (HEIGHT / 2);
		y = y * (-1); // Necessary since the resolution coordinates start on the top left corner
	}
	else if (y == (HEIGHT / 2)) {
		y = 0;
	}
	else {
		y = -(((HEIGHT / 2) - y) / (HEIGHT / 2));
		y = y * (-1);
	}
}

int getZone(float x, float y) {
	if (x > 0.0 && y > 0.0) {
		return 1;
	}
	else if (x > 0.0 && y < 0.0) {
		return 4;
	}
	else if (x < 0.0 && y < 0.0) {
		return 3;
	}
	else {
		return 2;
	}
}

vector<glm::vec3*>* generateCurve(vector<glm::vec3*>* points) {	
	//save values od the curve to follow on the rendering program
	ofstream arq("originalCurve.txt");

	vector<glm::vec3*>* calculatedCurve = new vector<glm::vec3*>();
	vector<glm::vec3*>* temp = new vector<glm::vec3*>();
	vector<glm::vec3*>* temp2 = new vector<glm::vec3*>();

	for (int i = 0; i < points->size(); i++) {
		temp->push_back(new glm::vec3(points->at(i)->x, points->at(i)->y, 0));
	}

	temp->push_back(points->at(0));
	temp->push_back(points->at(1));
	temp->push_back(points->at(2));

	for (int i = 0; i < (temp->size() - 3); i++) {

		for (int j = 0; j<1000; ++j) {

			float t = static_cast<float>(j) / 999.0;

			GLfloat x = (((-1 * pow(t, 3) + 3 * pow(t, 2) - 3 * t + 1)*temp->at(i)->x +
				(3 * pow(t, 3) - 6 * pow(t, 2) + 0 * t + 4)*temp->at(i + 1)->x +
				(-3 * pow(t, 3) + 3 * pow(t, 2) + 3 * t + 1)*temp->at(i + 2)->x +
				(1 * pow(t, 3) + 0 * pow(t, 2) + 0 * t + 0)*temp->at(i + 3)->x) / 6);

			GLfloat y = (((-1 * pow(t, 3) + 3 * pow(t, 2) - 3 * t + 1)*temp->at(i)->y +
				(3 * pow(t, 3) - 6 * pow(t, 2) + 0 * t + 4)*temp->at(i + 1)->y +
				(-3 * pow(t, 3) + 3 * pow(t, 2) + 3 * t + 1)*temp->at(i + 2)->y +
				(1 * pow(t, 3) + 0 * pow(t, 2) + 0 * t + 0)*temp->at(i + 3)->y) / 6);

			glm::vec3* point = new glm::vec3(x, y, 0.0);
			calculatedCurve->push_back(point);
			//saving values of the curve to the txt file (color is not needed)
			//invert y and z to invert the axis
			arq << "v " << point->x << " " << point->z << " "<< point->y << endl;
			calculatedCurve->push_back(new glm::vec3(1.0, 1.0, 1.0));
			cout << "Point added" << endl;
		}
	}
	arq.close();
	cout << "Done" << endl;
	return calculatedCurve;
}

vector<glm::vec3*>* generateExternalCurve(vector<glm::vec3*>* points, bool external) {
	//save the points on obj file
	ofstream arq;
	arq.open("curve.obj", ios::app); //ios:app opens the file and edits at the end
	
	vector<glm::vec3*>* calculatedCurve = new vector<glm::vec3*>();

	for (int j = 0; j < points->size() - 1; j += 2) {

		glm::vec3* a = points->at(j);
		glm::vec3* b;

		if (j == points->size() - 2) {
			b = points->at(0);
		}
		else {
			b = points->at(j+2);
		}

		GLfloat dx = b->x - a->x;
		GLfloat dy = b->y - a->y;
	
		if (dx == 0 || dy == 0) {
			dx = b->x - points->at(j - 2)->x;
			dy = b->y - points->at(j - 2)->y;
		}

		GLfloat angle = glm::atan(dy, dx);

		if (external) {
			angle += HALF_PI;
		}
		else {
			angle -= HALF_PI;
		}

		GLfloat offsetX = glm::cos(angle) * 0.09;
		GLfloat offsetY = glm::sin(angle) * 0.09;

		glm::vec3* pointGenerated = new glm::vec3(a->x + offsetX, a->y + offsetY, 0.0);

		calculatedCurve->push_back(pointGenerated);

		//add curve points to obj file as vertexes (color not needed)
		//invert y and z to invert axis
		arq << "v " <<pointGenerated->x << " " << pointGenerated->z << " " << pointGenerated->y << endl;

		calculatedCurve->push_back(new glm::vec3(1.0, 1.0, 1.0));
	}
	arq.close();

	return calculatedCurve;
}

vector<glm::vec3*>* generateFinalVector(vector<glm::vec3*>* internalCurve, vector<glm::vec3*>* externalCurve) {
	ofstream arq;
	arq.open("curve.obj", ios::app); //ios:app opens the file and edits at the end
	
	int i = 0;
	int index = 1;

	for (; i < internalCurve->size() - 2; i += 2) {
		// Ponto Interno 1
		finalPoints->push_back(internalCurve->at(i));
		finalPoints->push_back(internalCurve->at(i + 1));

		glm::vec3* a_int = internalCurve->at(i);

		// Ponto Interno 2
		finalPoints->push_back(internalCurve->at(i + 2));
		finalPoints->push_back(internalCurve->at(i + 3));

		glm::vec3* b_int = internalCurve->at(i+2);

		// Ponto Externo 1
		finalPoints->push_back(externalCurve->at(i));
		finalPoints->push_back(externalCurve->at(i + 1));

		glm::vec3* c_ext = externalCurve->at(i);

		faces++;
		arq << "f " << index + externalCurveSize << "/" << 1 << "/" << faces << " " <<
			           index + 1 + externalCurveSize << "/" << 2 << "/" << faces << " " <<
					   index << "/" << 3 << "/" << faces << endl;
		
		// Ponto Interno 2
		finalPoints->push_back(internalCurve->at(i + 2));
		finalPoints->push_back(internalCurve->at(i + 3));

		// Ponto Externo 2
		finalPoints->push_back(externalCurve->at(i + 2));
		finalPoints->push_back(externalCurve->at(i + 3));

		glm::vec3* d_ext = externalCurve->at(i+2);

		// Ponto Externo 1
		finalPoints->push_back(externalCurve->at(i));
		finalPoints->push_back(externalCurve->at(i + 1));

		faces++;
		arq << "f " << index + 1 + externalCurveSize << "/" << 2 << "/" << faces << " " <<
					   index + 1  << "/" << 4 << "/" << faces << " " <<
					   index << "/" << 3 << "/" << faces << endl;
		
		//get vectors for the normals
		//y and z are inversed to modify axis
		glm::vec3 ab = glm::vec3(b_int->x - a_int->x, b_int->z - a_int->z, b_int->y - a_int->y);
		glm::vec3 ac = glm::vec3(c_ext->x - a_int->x, c_ext->z - a_int->z, c_ext->y - a_int->y);
		glm::vec3 dc = glm::vec3(c_ext->x - d_ext->x, c_ext->z - d_ext->z, c_ext->y - d_ext->y);
		glm::vec3 db = glm::vec3(b_int->x - d_ext->x, b_int->z - d_ext->z, b_int->y - d_ext->y);

		glm::vec3 normal_vec_abac = glm::cross(ac, ab);
		glm::vec3 normal_vec_dbdc = glm::cross(db, dc);

		arq << "vn " << normal_vec_abac[0] << " " << normal_vec_abac[1] << " " << normal_vec_abac[2] << endl;
		arq << "vn " << normal_vec_dbdc[0] << " " << normal_vec_dbdc[1] << " " << normal_vec_dbdc[2] << endl;

		index++;
	}
	cout << i << " , " << index << endl;
	// O trecho abaixo liga os últimos pontos com primeiro os primeiros
	// Ponto Interno 1
	finalPoints->push_back(internalCurve->at(i));
	finalPoints->push_back(internalCurve->at(i + 1));

	glm::vec3* a_int = internalCurve->at(i);

	// Ponto Interno 2
	finalPoints->push_back(internalCurve->at(0));
	finalPoints->push_back(internalCurve->at(1));

	glm::vec3* b_int = internalCurve->at(0);

	// Ponto Externo 1
	finalPoints->push_back(externalCurve->at(i));
	finalPoints->push_back(externalCurve->at(i + 1));

	glm::vec3* c_ext = externalCurve->at(i);

	faces++;
	arq << "f " << index + externalCurveSize << "/" << 1 << "/" << faces << " " <<
				   externalCurveSize + 1 << "/" << 2 << "/" << faces << " " <<
				   index << "/" << 3 << "/" << faces << endl;
	
	// Ponto Interno 2
	finalPoints->push_back(internalCurve->at(0));
	finalPoints->push_back(internalCurve->at(1));

	// Ponto Externo 2
	finalPoints->push_back(externalCurve->at(0));
	finalPoints->push_back(externalCurve->at(1));

	glm::vec3* d_ext = externalCurve->at(0);

	// Ponto Externo 1
	finalPoints->push_back(externalCurve->at(i));
	finalPoints->push_back(externalCurve->at(i + 1));
	
	faces++;
	arq << "f " << 1 + externalCurveSize << "/" << 2 << "/" << faces << " " <<
		           1 << "/" << 4 << "/" << faces << " " <<
				   index << "/" << 3 << "/" << faces << endl;
	

	//get vectors for the normals
	//y and z are inversed to modify axis
	glm::vec3 ab = glm::vec3(a_int->x - b_int->x, a_int->z - b_int->z, a_int->y - b_int->y);
	glm::vec3 ac = glm::vec3(a_int->x - c_ext->x, a_int->z - c_ext->z, a_int->y - c_ext->y);
	glm::vec3 dc = glm::vec3(d_ext->x - c_ext->x, d_ext->z - c_ext->z, d_ext->y - c_ext->y);
	glm::vec3 db = glm::vec3(d_ext->x - b_int->x, d_ext->z - b_int->z, d_ext->y - b_int->y);

	glm::vec3 normal_vec_abac = glm::cross(ab, ac);
	glm::vec3 normal_vec_dbdc = glm::cross(db, dc);

	arq << "vn " << normal_vec_abac[0] << " " << normal_vec_abac[1] << " " << normal_vec_abac[2] << endl;
	arq << "vn " << normal_vec_dbdc[0] << " " << normal_vec_dbdc[1] << " " << normal_vec_dbdc[2] << endl;


	return finalPoints;
}

vector<GLfloat>* convertToFloat(vector<glm::vec3*>* points) {
std:vector<GLfloat>* temp = new std::vector<GLfloat>();

	for (int i = 0; i < points->size(); i++) {
		temp->push_back(points->at(i)->x);
		temp->push_back(points->at(i)->y);
		temp->push_back(points->at(i)->z);
	}

	return temp;
}

void saveNormalValuesToOBJ(vector<glm::vec3*>* points) {
	ofstream arq;
	arq.open("curve.obj", ios::app); //ios:app opens the file and edits at the end
	
	for (int i = 0; i < points->size() - 6; i+=2) {
		
		glm::vec3* a_int = points->at(i);
		glm::vec3* b_int = points->at(i+2);
		glm::vec3* c_ext = points->at(i+4);
		glm::vec3* d_ext = points->at(i+6);

		//get vectors
		//y and z are inversed to modify axis
		glm::vec3 ab = glm::vec3(a_int->x - b_int->x, a_int->z - b_int->z, a_int->y - b_int->y);
		glm::vec3 ac = glm::vec3(a_int->x - c_ext->x, a_int->z - c_ext->z, a_int->y - c_ext->y);
		glm::vec3 dc = glm::vec3(d_ext->x - c_ext->x, d_ext->z - c_ext->z, d_ext->y - c_ext->y);
		glm::vec3 db = glm::vec3(d_ext->x - b_int->x, d_ext->z - b_int->z, d_ext->y - b_int->y);

		glm::vec3 normal_vec_abac = glm::cross(ab, ac);
		glm::vec3 normal_vec_dbdc = glm::cross(db, dc);
		
		arq << "vn " << normal_vec_abac[0] << " " << normal_vec_abac[1] << " " << normal_vec_abac[2] << endl;
		arq << "vn " << normal_vec_dbdc[0] << " " << normal_vec_dbdc[1] << " " << normal_vec_dbdc[2] << endl;
	}
	arq.close();
}

void saveTextureValuesToOBJ() {
	//save texture points on obj
	ofstream arq;
	arq.open("curve.obj", ios::app); //ios:app opens the file and edits at the end
	arq << endl;
	arq << "vt " << 1.0 << " " << 1.0 << endl; //-1.0 is to treat the 1 - v on the objreader
	arq << "vt " << 1.0 << " " << 0.0 << endl; //+1.0 is to treat the 1 - v on the objreader
	arq << "vt " << 0.0 << " " << 1.0 << endl;
	arq << "vt " << 0.0 << " " << 0.0 << endl;
	arq << endl;
	arq.close();
}

/*
original texture values

arq << "vt " << 1.0 << " " << 0.0 << endl;
arq << "vt " << 1.0 << " " << 1.0 << endl;
arq << "vt " << 0.0 << " " << 0.0 << endl;
arq << "vt " << 0.0 << " " << 1.0 << endl;


*/