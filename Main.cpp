/*Autor: Eduardo de Jesús Gasca Laguna
Verano de la ciencia, Universidad de Guanajuato 2021
División de Ingenierías Campus Irapuato-Salamanca
Asesora de proyecto: Dra. Dora Luz Almanza Ojeda */

//Librerias para OpenGL
#include <Windows.h>
#include <Ole2.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <gl/glut.h>

//Libreria kinect
#include <NuiApi.h>

//Estándar C++
#include <iostream>
using namespace std;

//-------------------------------------//
//Resolucion
#define ancho 640
#define alto 480

//Variables OpenGL
GLuint texturaID;
GLubyte datos[ancho * alto * 4];

long mapeo_ProfaRGB[ancho * alto * 2];
float RGBArray[ancho * alto * 3];
float ProfArray[ancho * alto * 3];

//Kinect
HANDLE camaraRGB;
HANDLE sensorDeProfundidad;
INuiSensor* Kinect360;

void dibujar(void);

bool iniciarOpenGL(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(ancho,alto);
    glutCreateWindow("Verano de investigacion UG 2021");
    glutDisplayFunc(dibujar);
    glutIdleFunc(dibujar);
    return true;
}

bool iniciarKinect()
{
    int Sensores;

    if (NuiGetSensorCount(&Sensores) < 0 || Sensores < 1) 
        return false;
    if (NuiCreateSensorByIndex(0, &Kinect360) < 0) 
        return false;

    // Sensor de Profundida
    Kinect360->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_COLOR);
    Kinect360->NuiImageStreamOpen(
        NUI_IMAGE_TYPE_DEPTH,
        NUI_IMAGE_RESOLUTION_640x480,             
        0,        
        2,       
        NULL,    
        &sensorDeProfundidad);
    //Camara RGB
    Kinect360->NuiImageStreamOpen(
        NUI_IMAGE_TYPE_COLOR, 
        NUI_IMAGE_RESOLUTION_640x480, 
        0,     
        2,      
        NULL, 
        &camaraRGB);

    return Kinect360;
}

void obtenerProfundidad(GLubyte* matriz) 
{
    float* fmatriz = (float*)matriz;
    long* profARGB = (long*)mapeo_ProfaRGB;
    NUI_IMAGE_FRAME imageFrame;
    NUI_LOCKED_RECT LockedRect;

    if (Kinect360->NuiImageStreamGetNextFrame(sensorDeProfundidad, 0, &imageFrame) < 0) 
        return;

    INuiFrameTexture* textura = imageFrame.pFrameTexture;
    textura->LockRect(0, &LockedRect, NULL, 0);

    if (LockedRect.Pitch != 0) 
    {
        const USHORT* buffer = (const USHORT*)LockedRect.pBits;

        for (int j = 0; j < alto; ++j) 
        {
            for (int i = 0; i < ancho; ++i) 
            {
                USHORT profundidad = NuiDepthPixelToDepth(*buffer++);
                
                Vector4 pos = NuiTransformDepthImageToSkeleton(i, j, profundidad << 3, NUI_IMAGE_RESOLUTION_640x480);
                *fmatriz++ = pos.x / pos.w;
                *fmatriz++ = pos.y / pos.w;
                *fmatriz++ = pos.z / pos.w;

                
                NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
                    NUI_IMAGE_RESOLUTION_640x480, 
                    NUI_IMAGE_RESOLUTION_640x480, 
                    NULL,
                    i, j, profundidad << 3, 
                    profARGB, profARGB + 1);

                profARGB += 2;
            }
        }
    }
    textura->UnlockRect(0);
    Kinect360->NuiImageStreamReleaseFrame(sensorDeProfundidad, &imageFrame);
}

void obtenerRGB(GLubyte* matriz) 
{
    float* fmatriz = (float*)matriz;
    long* profARGB = (long*)mapeo_ProfaRGB;
    NUI_IMAGE_FRAME imageFrame;
    NUI_LOCKED_RECT LockedRect;

    if (Kinect360->NuiImageStreamGetNextFrame(camaraRGB, 0, &imageFrame) < 0)
        return;
    
    INuiFrameTexture* textura = imageFrame.pFrameTexture;
    textura->LockRect(0, &LockedRect, NULL, 0);

    if (LockedRect.Pitch != 0) 
    {
        const BYTE* start = (const BYTE*)LockedRect.pBits;

        for (int j = 0; j < alto; ++j) 
        {
            for (int i = 0; i < ancho; ++i) 
            {
                
                long x = *profARGB++;
                long y = *profARGB++;
                
                if (x < 0 || y < 0 || x > ancho || y > alto) 
                {
                    for (int k = 0; k < 3; ++k) *(fmatriz++) = 0.0f;
                }
                else 
                {
                    const BYTE* buffer = start + (x + ancho * y) * 4;

                    for (int n = 0; n < 3; ++n) 
                        *(fmatriz++) = buffer[2 - n] / 255.0f;
                }

            }
        }
    }
    textura->UnlockRect(0);
    Kinect360->NuiImageStreamReleaseFrame(camaraRGB, &imageFrame);
}

void obtenerDatosKinect() 
{
    obtenerProfundidad((GLubyte*)ProfArray);
    obtenerRGB((GLubyte*)RGBArray);
}

void rotar() 
{
    static double angulo = 0.;
    static double radio = 3.;
    double x = radio * sin(angulo);
    double z = radio * (1 - cos(angulo)) - radio / 2;
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(x, 0, z, 0, 0, radio / 2, 0, 1, 0);
    angulo += 0.05;
}

void mostrarDatosKinect() 
{
    
    obtenerDatosKinect();
    rotar();
   
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glBegin(GL_POINTS);
    for (int i = 0; i < ancho * alto; ++i) 
    {
        glColor3f(RGBArray[i * 3], RGBArray[i * 3 + 1], RGBArray[i * 3 + 2]);
        glVertex3f(ProfArray[i * 3], ProfArray[i * 3 + 1], ProfArray[i * 3 + 2]);
    }
    glEnd();
}

void dibujar()
{
    mostrarDatosKinect();
    glutSwapBuffers();
}

void lanzar()
{
    glutMainLoop();
}

//MAIN
int main(int argc, char* argv[])
{
    if (!iniciarOpenGL(argc,argv))
    {
        cout << "Falla al iniciar OpenGL" << endl;
        return 1;
    }
    cout << "OpenGL iniciado correctamente" << endl;

    if (!iniciarKinect())
    {
        cout << "No se puede iniciar Kinect360" << endl;
        return 1;
    }

    glClearColor(0, 0, 0, 0);
    glClearDepth(1.0f);

    glViewport(0, 0, ancho, alto);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, ancho / (GLdouble)alto, 0.1, 1000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 0, 0, 0, 0, 1, 0, 1, 0);

    lanzar();
	
    return 0;
}
