#include "BezierCurve.h"
#include <vector>
#include <glm\gtx\transform.hpp>

#include <cstdlib>
#include <time.h>


BezierCurve::BezierCurve(int steps, float curv){
	nSteps = steps;
	t = curv;

	srand(time(NULL));
	createPoints();
	createCurve();

	//Init color
	color = glm::vec3(0.7f, 0.33f, 0.1f);

	/* Init shaders */
	initValues();
	generateVectors();
	initShaders();
	generateBuffers();
}

void BezierCurve::createPoints(){
	/* Create some initial points */
	glm::vec4 initialPoint = glm::vec4(-4, 5, 0, 0);
	controlPointsList.push_back(initialPoint);
	////2
	controlPointsList.push_back(glm::vec4(-2,-5,0,0));
	////3
	controlPointsList.push_back(glm::vec4(0,0,0,0));
	////4
	controlPointsList.push_back(glm::vec4(2,-5,0,0));
	////5
	controlPointsList.push_back(glm::vec4(4,5,0,0));
}

void BezierCurve::createCurve(){
	
	glm::mat4x4 mMat = glm::mat4x4(0,   1,     0,  0,
								  -t,   0,     t,  0,
							     2*t, t-3, 3-2*t, -t,
								  -t, 2-t,   t-2,  t);

	for (int i = 2; i< controlPointsList.size()+2; ++i){
		for (int step = 0; step< nSteps; step++){
			float u = ((float)step) / ((float)nSteps);

			glm::vec4   uVec = glm::vec4(1, u, u*u, u*u*u);
			glm::mat4x4 pVec = glm::mat4x4(controlPointsList[(i-2) %controlPointsList.size()],
										   controlPointsList[(i-1) %controlPointsList.size()],
										   controlPointsList[(i)   %controlPointsList.size()],
										   controlPointsList[(i+1) %controlPointsList.size()]);

			glm::vec4 point = pVec*(mMat*uVec);

			//Adding vertex of interpolated point
			pointList.push_back(new PV3D(point.x, point.y, point.z));
		}
	}

	/* Calculate tangent, normal and binormal at the first point */
	PV3D* tangent = (pointList[1]->subtraction(pointList[pointList.size()-1]))->factor(t); tangent->normalize();
	tangentList.push_back(tangent);
	PV3D* normal = new PV3D(0.0f,0.0f,1.0f);
	normalList.push_back(normal);
	PV3D* binormal = normal->crossProduct(tangent);  binormal->normalize();
	binormalList.push_back(binormal);

	/* Calculate tangents, normals and binormals */
	for (int i = 1; i < pointList.size() + 1; ++i){
		/* Tangent = t*([pi+1] - [pi-1]) */
		tangent = (pointList[(i+1) %pointList.size()]->subtraction(pointList[(i-1) %pointList.size()]))->factor(t); tangent->normalize();
		tangentList.push_back(tangent);

		/* Binormal = Normal x Tangent */
		normal = new PV3D(0.0f,0.0f,1.0f);
		//normal->setZ(1.0f); //Set normal vector as z-axis
		//normal = binormalList.at(i-1)->crossProduct(tangent); normal->normalize();
		//normal = tangent->crossProduct(new PV3D(0, 1, 0)); normal->normalize();
		normalList.push_back(normal);

		/* Binormal = Normal x Tangent */
		binormal = normal->crossProduct(tangent);  binormal->normalize();
		binormalList.push_back(binormal);
	}
}

float BezierCurve::numRandom(){
	return ((float)rand() / (RAND_MAX + 1)) * 10;
}

/* GETTERS */
std::vector<glm::vec4> BezierCurve::getControlPointsList(){
	return controlPointsList;
}
std::vector<PV3D*> BezierCurve::getPointList(){
	return pointList;
}
std::vector<PV3D*> BezierCurve::getTangentList(){
	return tangentList;
}
std::vector<PV3D*> BezierCurve::getNormalList(){
	return normalList;
}
std::vector<PV3D*> BezierCurve::getBinormalList(){
	return binormalList;
}
int BezierCurve::nPoints(){
	return pointList.size();
}

/* Draw Curve*/
// Old method with static opengl and cpu compute
void BezierCurve::draw(int modo){
	if (modo == 1){
		glBegin(GL_LINES);
	}else
		glBegin(GL_LINE_STRIP);
	// Blue colour
	glColor3f(0.0f, 0.0f, 1.0f);
		for (PV3D* punto : pointList){
			glVertex3f(punto->getX(), punto->getY(), punto->getZ());
		}
		glVertex3f(pointList.at(0)->getX(), pointList.at(0)->getY(), pointList.at(0)->getZ()); // El primero tambi�n se dibuja el ultimo
	glEnd();
}

//OpenGL 3.0 - gpu compute
void BezierCurve::draw(Camara* camera){

	glUseProgram(program);

	glm::mat4 modelMatrix = glm::translate(glm::vec3(0, 0, 0));

	glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &camera->getModelViewProjection(modelMatrix)[0][0]);
	glUniform3f(inColor, color.x, color.y, color.z);

	//Drawing   
	glBindVertexArray(vao);

	glLineWidth(5);
	glDrawElements(GL_LINE_LOOP, pointVector.size(), GL_UNSIGNED_INT, 0);
	glLineWidth(1);

	glUseProgram(NULL);

}

void BezierCurve::initValues(){

	program = 0;

	inVertex = -1;
	inColor = -1;
	
	mvpMatrixID = -1;
	
}

void BezierCurve::generateVectors(){

	for (int i = 0; i < nPoints(); i++){
		PV3D* punto = pointList.at(i)->clone();
		vertexVector.push_back(punto->getX());
		vertexVector.push_back(punto->getY());
		vertexVector.push_back(punto->getZ());
	}

	for (unsigned int i = 0; i<vertexVector.size() / 3; ++i)
		pointVector.push_back(i);

}

void BezierCurve::initShaders(){
	//Compile shaders
	vShader.compileShader();
	fShader.compileShader();

	//Link
	program = glCreateProgram();
	glAttachShader(program, vShader.getShaderID());
	glAttachShader(program, fShader.getShaderID());
	glBindAttribLocation(program, 0, "inVertex");
	glBindAttribLocation(program, 1, "inColor");

	glLinkProgram(program);

	int linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked){
		//Calculate error string
		GLint logLen;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);

		char *logString = new char[logLen];
		glGetProgramInfoLog(program, logLen, NULL, logString);
		std::cout << "Error: " << logString << std::endl;
		delete logString;

		glDeleteProgram(program);
		program = 0;
		return;
	}

	//Uniform variable
	mvpMatrixID = glGetUniformLocation(program, "modelViewProjection");

	//Attributes
	inColor = glGetAttribLocation(program, "inColor");
	inVertex = glGetAttribLocation(program, "inVertex");

}

//Generate VBO && VAO
void BezierCurve::generateBuffers(){
	
	glFrontFace(GL_CW);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(2, buffer);

	//Vertex
	glBindBuffer(GL_ARRAY_BUFFER, buffer[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vertexVector.size(), &(vertexVector.front()), GL_STATIC_DRAW);
	glVertexAttribPointer(inVertex, 3, GL_FLOAT, GL_FALSE, 0, 0);  //Shader input
	glEnableVertexAttribArray(inVertex);

	//Points
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*pointVector.size(), &(pointVector.front()), GL_STATIC_DRAW);

	glBindVertexArray(0);

}
