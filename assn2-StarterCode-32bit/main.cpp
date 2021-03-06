/*
	Simulacion del Torrente Sanguineo
	RA - Raquel Peces & Julio Martin
*/

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "pic.h"

#ifdef WIN32

#include <windows.h>
#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glu.h"
#include "GL/glut.h"
#pragma comment(lib, "win32/libtiff_i.lib")
#pragma comment(lib, "win32/jpeg.lib")
#pragma comment(lib, "win32/libtiff.lib")
#pragma comment(lib, "win32/libpicio.lib")
#pragma comment(lib, "libs/glut32.lib")
#pragma comment(lib, "libs/glew32.lib")
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include "BezierCurve.h"
#include "Perlin_noise/PerlinGenerator.h"
#include "Vein.h"
#include "Camara.h"
#include "Blood.h"
#include "DrawCurve.h"
#include <vector>
#include <iostream>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

int old_mouse_x = WINDOW_WIDTH / 2, old_mouse_y = WINDOW_HEIGHT / 2;

/* - Menu State Identifier - */
int g_iMenuId;

/* - Mouse State Variables - */
int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

/* - BezierCurve Variable - */
const int curveSteps = 256;
const float curveT = 0.7f;
BezierCurve* curve;
BezierCurve* curve2;
DrawCurve* drawCurve;

/* - Vein Variable - */
const int veinSides = 256;
const float veinRadius = 2.0f;
Vein* vein;
int showTexture;
int showTexture2;

/* - Blood Variable - */
 const int numRedCorpuscles = 300;
 const int numWhiteCorpuscles = 150;
 Blood* blood;

/* - Perlin Noise - */
const int sideVertex = 256;
PerlinGenerator perlinNoise(6, sideVertex);

/* Cam Variables*/
const int INITPOINT = 0;
Camara* camara;
PV3D *eye, *look, *up;
GLdouble N=0.01, F=1000.0; //near and fare planes
//float angleYaw=0, angleRoll=0, anglePitch=0;
float fovy = 45.0, aspect = WINDOW_WIDTH / WINDOW_HEIGHT, zoom = 0.1;
bool automatic = true;    // Move Automatic
bool heartBeat = false;    // Move Heart Beat
int acceleration = 1;
int modo = 3;			   // Default Mode lines
float displaced = 70.0;    // Init Distance from a vein in camera out
int mutation = 0;
int mutation2 = 0;
int countPoint0 = 0;
int countPoint1 = 0;

/* Control del numero de captura */
int captura = 0;

/* Muesra los FPS en pantalla*/
int cycle, fps;

/*	saveScreenshot - Writes a screenshot to the specified filename in JPEG */
void saveScreenshot (char *filename){
	Pic *in = NULL;

	if (filename == NULL)
		return;

	/* Allocate a picture buffer */
	in = pic_alloc (WINDOW_WIDTH, WINDOW_HEIGHT, 3, NULL);

	printf("File to save to: %s\n", filename);
	/* Loop over each row of the image and copy into the image */
	for (int i = WINDOW_HEIGHT - 1; i >= 0; i--){
		glReadPixels(0, WINDOW_HEIGHT - 1 - i, WINDOW_WIDTH, 1, GL_RGB,
						GL_UNSIGNED_BYTE, &in->pix[i*in->nx*in->bpp]);
	}

	/* Output the file */
	if (jpeg_write(filename, in)){
		printf("File saved Successfully\n");
	}else{
		printf("Error in Saving\n");
	}
	
	pic_free(in);
}

/*	loadTexture - Loads a texture from a JPEG file to memory and returns the handle	*/
//GLuint loadTexture (char *filename, int *pWidth = NULL, int *pHeight = NULL){
//	GLuint texIndex;
//
//	if (filename == NULL)
//		return 0;
//
//	Pic *texture = jpeg_read (filename, NULL);
//	if (texture == NULL)
//		return 0;
//
//	glGenTextures (1, &texIndex);
//	glBindTexture (GL_TEXTURE_2D, texIndex);
//
//	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
//
//	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
//
//	if (pWidth != NULL)
//		*pWidth = texture->nx;
//	if (pHeight != NULL)
//		*pHeight = texture->ny;
//
//	switch (texture->bpp){
//		case 1:
//		default:
//			gluBuild2DMipmaps (GL_TEXTURE_2D, 1, texture->nx, texture->ny,
//						GL_R3_G3_B2, GL_UNSIGNED_BYTE, texture->pix);
//			break;
//		case 3:
//			gluBuild2DMipmaps (GL_TEXTURE_2D, 3, texture->nx, texture->ny,
//						GL_RGB, GL_UNSIGNED_BYTE, texture->pix);
//			break;
//		case 4:
//			gluBuild2DMipmaps (GL_TEXTURE_2D, 4, texture->nx, texture->ny,
//						GL_RGBA8, GL_UNSIGNED_BYTE, texture->pix);
//			break;
//	}
//
//	pic_free(texture);
//
//	return texIndex;
//}

/*	doIdle - The idle-function that can be used to update the screen */
void doIdle(){
	if (automatic)
		camara->followCurve(true); // true indica ir hacia adelante
	if (heartBeat){
		camara->simulateHeartBeat(acceleration); // true indica ir hacia adelante
		acceleration=(acceleration+1)%25;
	}
	glutPostRedisplay();
}

/*	mousedrag - converts mouse drags into information about rotation/translation/scaling */
void mousedrag(int x, int y){
	int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
	g_vMousePos[0] = x;
	g_vMousePos[1] = y;

	//camara->move((x - WINDOW_WIDTH / 2) / WINDOW_WIDTH, (y - WINDOW_HEIGHT / 2) / WINDOW_HEIGHT);
}

/*	mouseidle - Idle mouse movement callback function */
void mouseidle(int x, int y){
	g_vMousePos[0]= x;
	g_vMousePos[1]= y;
}

/* 	mousebutton - Sets the global mouse states according to the actions */
void mousebutton(int button, int state, int x, int y){
	switch (button){
		case GLUT_LEFT_BUTTON:
			g_iLeftMouseButton = (state==GLUT_DOWN);
			break;
		case GLUT_MIDDLE_BUTTON:
			g_iMiddleMouseButton = (state==GLUT_DOWN);
			break;
		case GLUT_RIGHT_BUTTON:
			g_iRightMouseButton = (state==GLUT_DOWN);
			break;
	}
	g_vMousePos[0] = x;
	g_vMousePos[1] = y;
}
//Destroy and free memory
void unlinkAndFree(){
	vein->freeMemory();
	drawCurve->freeMemory();
	blood->freeMemory();
}

void key(unsigned char key, int x, int y){
	bool need_redisplay = true;
	switch (key) {
	//Escape key
	case 27:  
		unlinkAndFree();
		exit(0);
		break;
	// Teclas de Avanzar por vena
	case 'a':
		camara->followCurve(true);
		break;
	case 'z':
		camara->followCurve(false);
		break;
	// Vena desde fuera y zoom
	case '8':
		camara->followCurveOut(0, displaced);
		break;
	case char('9'):
		camara->followCurveOut(1,displaced);
		break;
	case '6':
		displaced+= 4.0;
		camara->followCurveOut(2, displaced);
		break;
	case char('7') :
		displaced -= 4.0;
		camara->followCurveOut(2, displaced);
		break;

	// Teclas para zoom
	case '+':
		camara->addZoom(zoom);
		camara->reDisplay();
		break;
	case '-':
		camara->deductZoom(zoom);
		camara->reDisplay();
		break;

	//Teclas para moverse
	case 'j':
		camara->move(-0.01f, 0.0f);
		camara->reDisplay();
		break;
	case 'k':
		camara->move(0.0f, -0.01f);
		camara->reDisplay();
		break;
	case 'l':
		camara->move(0.01f, 0.0f);
		camara->reDisplay();
		break;
	case 'i':
		camara->move(0.0f, 0.01f);
		camara->reDisplay();
		break;
	//Teclas para rotacion
	case 'u':
		camara->rotate(0.1f, 0.00f, 0.0f);
		camara->reDisplay();
		break;
	case 'o':
		camara->rotate(0.0f, 0.1f, 0.0f);
		camara->reDisplay();
		break;
	case 'p':
		camara->rotate(0.0f, 0.0f, 1.0f);
		camara->reDisplay();
		break;

	// Cambio de Modo entre puntos, aristas o poligonos
	case '1':
			modo = 1;
		break;
	case '2':
			modo = 2;
		break;
	case '3':
			modo = 3;
		break;
	case '4':
			automatic = !automatic;
			heartBeat = false;
		break;
	case '5':
			heartBeat = !heartBeat;
			automatic = false;
		break;
	// Mostrar textura
	case 'c':
		showTexture = 1 - showTexture;
		vein->setShowTexture(showTexture);
		break;
	case 'v':
		showTexture2 = 1 - showTexture2;
		blood->setShowTexture(showTexture2);
		break;
	// Capturas de pantalla
	case '0':
		string folder = "./Capturas/screenShot" + to_string(captura) + ".jpg";
		char *fileName = new char[folder.length() + 1]; strcpy(fileName, folder.c_str());
		saveScreenshot(fileName);
		delete[] fileName;
		captura++;
		break;
	}
}


void flopsForSecond(){

	// Informacion de FPS
	char scene_info[32];
	//void *font_type = GLUT_BITMAP_9_BY_15;
	void *font_type = GLUT_BITMAP_HELVETICA_18;

	double x = 1.0 * WINDOW_WIDTH / 100.0;
	double y = 97.0 * WINDOW_HEIGHT / 100.0;
	//set and save projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	
	glOrtho(0.0, WINDOW_WIDTH, 0.0, WINDOW_HEIGHT, -1.0f, 1.0f);
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor3f(1.0, 1.0, 1.0);

	sprintf(scene_info, "FPS: %d", fps);
	glRasterPos2d(x - 3 + 1, y - 1);
	for (char *p = scene_info; *p; p++)
		glutBitmapCharacter(font_type, *p);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glPopAttrib();
}

/*	display - Function to modify with your heightfield rendering code (Currently displays a simple cube) */
void display(){

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (camara->getCurrentPoint() == (curve->nPoints()-1)) ++countPoint0;
	if (camara->getCurrentPoint() == (curve->nPoints()*2/3)) ++countPoint1;
	if (countPoint1 % 2 == 1) mutation2 = 1; else  mutation2 = 0;
	if (countPoint0 % 2 == 1){ mutation = 1; mutation2 = 0; }else{ mutation = 0; }
	
	camara->setMutation(mutation);

	drawCurve->draw(camara, modo, mutation);
	vein->draw(camara, modo, mutation);
	blood->draw(camara, modo, mutation2);

	flopsForSecond();

	glutSwapBuffers(); 
	cycle++;
}

void onSecond(int x){
	fps = cycle;
	cycle = 0;
	glutTimerFunc(1000, onSecond, 0);
}

/*	menufunc - Menu Event-handler */
void menufunc(int value){
	switch (value){
	case 0:
		exit(0);
		break;
	case 1:
		modo = (modo% 3) + 1;
		break;
	}
}

void startGlew(){
	GLenum err = glewInit();
	if (GLEW_OK != err){
		printf("Error: %s\n", glewGetErrorString(err));
	}
	const GLubyte *oglVersion = glGetString(GL_VERSION);
	printf("This system supports OpenGL Version %s.\n", oglVersion);
}

/*	myinit - Function to add your initialization code */
void startGlut(int argc, char** argv){
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("Torrente Sanguineo - Julio y Raquel");
	
	glutDisplayFunc(display);			/* tells glut to use a particular display function to redraw */
	glutKeyboardFunc(key);				/* callback for keyboard */
	glutIdleFunc(doIdle);				/* replace with any animate code */
	glutMotionFunc(mousedrag);			/* callback for mouse drags */
	glutPassiveMotionFunc(mouseidle);	/* callback for idle mouse movement */
	glutMouseFunc(mousebutton);			/* callback for mouse button changes */
	glutTimerFunc(1000, onSecond, 0);   /* callback for calculate FPS */

	/* allow the user to quit using the right mouse button menu */
	g_iMenuId = glutCreateMenu(menufunc);
	glutSetMenu(g_iMenuId);
	glutAddMenuEntry("Quit", 0);
	glutAddMenuEntry("ModoVisualiz", 1);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void startCam(){	
	glClearColor(0.0, 0.0, 0.0, 0.0);

	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_BACK, GL_LINE);
	//glEnable(GL_CULL_FACE);
	//glEnable(GL_NORMALIZE);
	//glEnable(GL_COLOR_MATERIAL);
	//glShadeModel(GL_SMOOTH);

	camara->reDisplay();

	//glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);  //// Viewport set up
}

/*	main - The Main Function */
int main (int argc, char ** argv){

	/* do initialization */
	startGlut(argc, argv);
	startGlew();

	/* Creamos el ruido de Perlin*/
	std::cout << "Generating perlin noise..." << std::endl;
	perlinNoise.generate();

	/* Creamos la curva y la vena asociada*/
	std::cout << "Generating bezier curve..." << std::endl;
	curve = new BezierCurve(curveSteps, curveT);
	drawCurve = new DrawCurve(curve);
	curve2 = new BezierCurve(40, 10);

	std::cout << "Generating vein..." << std::endl;
	//GLuint textureID = loadTexture("./Textures/veinTexture.jpg");
	vein = new Vein(veinSides, veinRadius, curve, -1);
	vein->addPerlinNoise(perlinNoise.getNoiseImage());
	vein->generateShader();

	/* Creamos los globulos dentro de la vena */
	std::cout << "Generating blood..." << std::endl;
	blood = new Blood(numRedCorpuscles, numWhiteCorpuscles, curve, veinRadius);
	
	/* Camera parameters */
	std::cout << "Colocando la camara..." << std::endl;
	eye = curve->getPointList().at(INITPOINT);
	look = eye->addition(curve->getTangentList().at(INITPOINT));
	up = curve->getBinormalList().at(INITPOINT);

	camara = new Camara(*eye, *look, *up, N, F, fovy, aspect, curve,veinRadius);
	startCam();
	
	glutMainLoop();
	/* Liberando recursos */
	unlinkAndFree(); 
	return 0;
}
