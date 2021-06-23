// this program simply demonstrates only a few of the
// functions available in the Easy Accelerated Image Lib 0.2
// A list of available functions is available in graphics2d.h
// Also be sure to read the README.TXT file
//
// (See LICENSE.TXT for license agreement.)

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <string.h>
#include "callbacks.h"

#include "graphics2d.h"

#define PI (3.14159) // close enough
#define DEG2RAD(a) (a*PI/180.0f)

PSP_MODULE_INFO("HMP2D_IMGLIB",0,1,1);

int main()
{
	float windmillAngle = 0.0f;
	const char msg1[] = "Hazed Mirror Productions presents";
	const char msg2[] = "Easy Accelerated Image lib 0.2";
	Image* backgroundImg;
	Image* spinningImg;
	Image* millImg;
	Image* textImg;
	
	pspDebugScreenInit();
	setupCallbacks();
	
	// *** Call this to init the graphics functions ***
	//
	initGraphics();
	
	// *** load images. PNG files are supported ***
	//
	backgroundImg = loadImage("gfx/ground.png");
	spinningImg = loadImage("gfx/wind1.png");
	millImg = loadImage("gfx/wind2.png");
	textImg = newImage((strlen(msg2)+1)*8,12,0);
	if( backgroundImg == NULL || spinningImg == NULL || millImg==NULL || textImg == NULL )
	{
		sceKernelExitGame();
		return -1;
	}
	
	// *** scale the background to be twice its size
	// (this is necessary because the image dimensions are half
	// the screen width and height.)
	//
	stretchImage(backgroundImg,2.0f,2.0f);
	
	// *** we can set the Z location of the backgroundImg to
	//	   make sure it is drawn at the bottom no matter what order
	//		the images are drawn. (All Z values must be a floating
	//		point number between -1 and 1.)
	// 	
	setImageZ(backgroundImg, -0.5f);
	setImageZ(millImg,  -0.2f);
	setImageZ(spinningImg,  -0.1f);
	
	// *** images can also be scaled down.
	//
	stretchImage(millImg,0.5f,0.5f);

	// *** printTextImage is now implemented
	//
	printTextImage(msg2,0,0, 0xff000000, textImg);
	printTextImage(msg2,1,0, 0xff000000, textImg);
	stretchImage(textImg,1.8f,2.0f);
	
	// *** this has to be called right before the game loop starts
	//
	sceKernelDcacheWritebackAll();
	
	// *** while the user hasn't exited with the home button...
	//
	while( running() )
	{
		// *** begin drawing sequence. All calls to drawImage go 
		//		between startDrawing() and endDrawing()
		//
		startDrawing();

		// *** render the background image at 0,0--the top left corner.
		// the last parameter is the angle in radians at which to draw.
		// specify 0 for no rotation, of course.
		//
		drawImage(backgroundImg,0,0,0.0f);
		
		// *** draw part of wind mill at a moving angle
		//
		drawImage(spinningImg,40,80,DEG2RAD(windmillAngle));
		
		// *** draw other part of windmill
		//
		drawImage(millImg,56,104,0.0f);

		// *** draw text image
		//
		drawImage(textImg, 25,200, 0);

		// *** draw bolded messages
		//		
		printTextScreen(msg1,25,191,0xff800000);
		printTextScreen(msg1,26,191,0xff800000);
		
		// *** end the drawing sequence.
		//
		endDrawing();

		// *** helps prevent flicker:
		//
		sceDisplayWaitVblankStart();

		// *** flip the screen--this MUST be done AFTER endDrawing.
		//
		flipScreen();
		
		windmillAngle += 0.2f; // rotate windmill
	}
	
	// *** call this to end the graphics functions lib***
	//
	endGraphics();
	
	// *** free images from memory
	//
	freeImage(backgroundImg);
	freeImage(spinningImg);
	freeImage(millImg);
	freeImage(textImg);
	
	sceKernelExitGame();
	return 0;
}
