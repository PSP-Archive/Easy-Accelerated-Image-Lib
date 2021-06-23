#include "graphics2d.h"

#include <string.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <png.h>
#include <stdlib.h>

//unsigned int __attribute__((aligned(16))) g_dlist[262144];
void* g_dlist;

extern u8 msx[]; // change to extern "C" u8 msx[]; for use with C++

#define FRAMEBUFFER_SIZE (PSP_LINE_SIZE*SCR_HEIGHT*4)

int g_dispNumber;
u32 *g_vram_base = (u32*) (0x40000000 | 0x04000000);

void *fbp0 = NULL;
void *fbp1;
void *zbp;

Vertex getVertex(float u,float v,u32 color,float x, float y,float z)
{
	Vertex vert;
	vert.u = u;
	vert.v = v;
	vert.color = color;
	vert.x = x;
	vert.y = y;
	vert.z = z;
	return vert;
}

Image* newImage(int width, int height, unsigned int color)
{
	Image* img = (Image*)malloc(sizeof(Image));
	
	if( img==NULL || width < 1 || height < 1 )
		return NULL;
		
	img->width = width;
	img->height = height;
	img->origWidth = width;
	img->origHeight = height;
	img->texWidth = nextPowOf2(width);
	img->texHeight = nextPowOf2(height);
	
	img->data = (unsigned char*)malloc(img->texWidth*img->height*sizeof(u32));
	if( img->data == NULL )
	{
		free(img);
		return NULL;
	}
	
	int ix, iy;
	
	u32* data = (u32*)(img->data);
	
	for(ix = 0; ix < width; ix++)
	{
		for(iy=0; iy < img->height; iy++)
		{
			data[iy * img->texWidth + ix] = color;
		}
	}
	
	if( (height % 8) == 0 )
		swizzleImage(img);
	else
		img->swizzled = 0;
		
	createImageVerts(img);
	
	return img;
}

Image* loadImage(const char fileName[])
{
	Image* img = NULL;
	
	png_structp pngPtr;
	png_infop infoPtr;
	unsigned int sigRead = 0;
	png_uint_32 width, height;
	int bitDepth, colorType, interlaceType, x, y;
	u32* line;
	FILE *fileIn;

	fileIn = fopen(fileName, "rb");
	if (fileIn == NULL) 
		return NULL;

	pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (pngPtr == NULL) 
	{
		fclose(fileIn);
		return NULL;
	}

	png_set_error_fn(pngPtr, (png_voidp) NULL, (png_error_ptr) NULL, NULL);
	infoPtr = png_create_info_struct(pngPtr);
	if (infoPtr == NULL) 
	{
		fclose(fileIn);
		png_destroy_read_struct(&pngPtr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}

	png_init_io(pngPtr, fileIn);
	png_set_sig_bytes(pngPtr, sigRead);
	png_read_info(pngPtr, infoPtr);
	png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth, &colorType, 
					&interlaceType, int_p_NULL, int_p_NULL);

	// can't allow images with width or height greater than 512 or it will crash
	// the system.
	//
	if (width > 512 || height > 512) 
	{
		fclose(fileIn);
		png_destroy_read_struct(&pngPtr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}

	// attempt to allocate memory for new image
	//
	img = (Image*)malloc(sizeof(Image));
	if( img == NULL )
		return NULL;

	img->swizzled = 0;
	
	img->width = width;
	img->height = height;
	img->origWidth = width;
	img->origHeight = height;
	img->texWidth = nextPowOf2(width);
	img->texHeight = nextPowOf2(height);
	png_set_strip_16(pngPtr);
	png_set_packing(pngPtr);

	if (colorType == PNG_COLOR_TYPE_PALETTE) 
		png_set_palette_to_rgb(pngPtr);

	if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) 
		png_set_gray_1_2_4_to_8(pngPtr);

	if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)) 
		png_set_tRNS_to_alpha(pngPtr);

	png_set_filler(pngPtr, 0xff, PNG_FILLER_AFTER);

	//img->data = malloc(width * height * sizeof(u32));
	img->data = (unsigned char*)malloc( img->texWidth * img->height * sizeof(u32));
	
	if (!img->data) 
	{
		fclose(fileIn);
		png_destroy_read_struct(&pngPtr, png_infopp_NULL, png_infopp_NULL);
		free(img);
		return NULL;
	}

	line = (u32*) malloc(width * 4);

	if (!line) 
	{
		fclose(fileIn);
		png_destroy_read_struct(&pngPtr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}

	u32* data = (u32*)img->data;

	for (y = 0; y < height; y++) 
	{
		png_read_row(pngPtr, (u8*) line, png_bytep_NULL);

		for (x = 0; x < width; x++) 
		{
			u32 color = line[x];
			data[x + y * img->texWidth] =  color;
		}
	}

	// Png's are 32-bit color? so...
	//
	img->colorMode = 32 / 8; // i'm not sure about this

	free(line);
	png_read_end(pngPtr, infoPtr);
	png_destroy_read_struct(&pngPtr, &infoPtr, png_infopp_NULL);
	fclose(fileIn);

	if( (img->height % 8) == 0 )
		swizzleImage(img);
	createImageVerts(img);
	return img;

}

// swizzle code from
// http://www.psp-programming.com/forums/index.php?topic=1043.0
// Thanks to Raphael
//
void swizzleImage( Image* img )
{
	if( img->swizzled != 0 || (img->origHeight % 8) != 0 )
	{
		return;
	}

	unsigned int pitch = (img->texWidth*sizeof(u32));
	unsigned int i,j;
	unsigned int rowblocks = (pitch / 16);
	long size = pitch  * img->height;
      
	unsigned char* out = (unsigned char*)malloc( size*sizeof(unsigned char) );
    
	for (j = 0; j < img->origHeight; ++j)
	{
		for (i = 0; i < pitch; ++i)
		{
			unsigned int blockx = i / 16;
            unsigned int blocky = j / 8;
   
            unsigned int x = (i - blockx*16);
            unsigned int y = (j - blocky*8);
            unsigned int block_index = blockx + ((blocky) * rowblocks);
            unsigned int block_address = block_index * 16 * 8;
   
            out[block_address + x + y * 16] = img->data[i+j*pitch];
         }
	}
	free( img->data );
	img->data = out;
	img->swizzled = 1;
}

void unswizzleImage(Image* img)
{
	if( img->swizzled != 1 || (img->origHeight % 8) != 0 )
	{
		return;
	}

	unsigned int pitch = (img->texWidth*sizeof(u32));
	unsigned int i,j;
	unsigned int rowblocks = (pitch / 16);
	long size = img->texWidth * img->texHeight * sizeof(u32);
      
	unsigned char* out = (unsigned char*)malloc( size );
    
	for (j = 0; j < img->origHeight; ++j)
	{
		for (i = 0; i < pitch; ++i)
		{
			unsigned int blockx = i / 16;
            unsigned int blocky = j / 8;
   
            unsigned int x = (i - blockx*16);
            unsigned int y = (j - blocky*8);
            unsigned int block_index = blockx + ((blocky) * rowblocks);
            unsigned int block_address = block_index * 16 * 8;
   
            out[i+j*pitch] = img->data[block_address + x + y * 16];
         }
	}
	free( img->data );
	img->data = out;
	img->swizzled = 0;
}

void createImageVerts(Image* img)
{
	img->vertices = (Vertex*)malloc( 4 * sizeof(Vertex) );
	
	float hPercent = (float)img->height / (float)img->texHeight;
	float wPercent = (float)img->width / (float)img->texWidth;
	
	if( img->vertices )
	{
		img->vertices[0] = getVertex(0.0f,0.0f,0xffFFFFFF, // top-left
			-img->width/2,-img->height/2,0.0f);
		img->vertices[1] = getVertex(0.0f,hPercent,0xffFFFFFF, // bl
			-img->width/2, img->height/2,0.0f);
		img->vertices[2] = getVertex(wPercent,0.0f,0xffFFFFFF, // tr
			 img->width/2,-img->height/2,0.0f);
		img->vertices[3] = getVertex(wPercent,hPercent,0xffFFFFFF,// br
			 img->width/2, img->height/2,0.0f);
	}
}

void freeImage(Image* img)
{
	free(img->data);
	img->data = NULL;
	free(img);
	img = NULL;
}

void drawImage(Image* img, int x, int y, float radians)
{
	sceGumPushMatrix();
	
	ScePspFVector3 loc = {x+img->width/2,y+img->height/2,0.0f};
	sceGumTranslate(&loc);
	sceGumRotateZ(radians);
	
	sceGuTexMode(GU_PSM_8888,0,0,img->swizzled);
	sceGuTexImage(0,img->texWidth,img->texHeight,img->texWidth,
		img->data);
	sceGumDrawArray(GU_TRIANGLE_STRIP,GU_COLOR_8888 | 
		GU_TEXTURE_32BITF |	GU_VERTEX_32BITF | GU_TRANSFORM_3D, 
		4, 0, img->vertices
		);
	
	sceGumPopMatrix();
}

void drawImagePart(Image* img, int dx, int dy, int srcX,
	int srcY, int srcW, int srcH, float rads)
{
	const unsigned int WHITE=0xffFFFFFF;
	Vertex *verts = (Vertex*)sceGuGetMemory(sizeof(Vertex) * 4);
	
	float scaleX = (float)img->width/(float)img->origWidth;
	float scaleY = (float)img->height/(float)img->origHeight;
	
	float u_Left= ((float)(srcX)/(float)(img->texWidth)) / scaleX;
	float v_Top = ((float)(srcY)/(float)(img->texHeight))/ scaleY;
	float u_Right=((float)(srcX+srcW)/(float)(img->texWidth))/ scaleX;
	float v_Bottom=((float)(srcY+srcH)/(float)(img->texHeight))/scaleY;
	
	int width = srcW;
	int height = srcH;
	float z = img->vertices[0].z;
	
	// -- top left --
	verts[0]=getVertex(u_Left,v_Top,WHITE,-width/2,-height/2,z);
	// -- bottom left --
	verts[1]=getVertex(u_Left,v_Bottom,WHITE,-width/2,height/2,z);
	// -- top right --
	verts[2]=getVertex(u_Right,v_Top,WHITE,width/2,-height/2,z);
	// -- bottom right -- 
	verts[3]=getVertex(u_Right,v_Bottom,WHITE,width/2,height/2,z);
	
	// render the image
	sceGumPushMatrix();
	{
	ScePspFVector3 pos={dx+width/2,dy+height/2,0};
	sceGumTranslate(&pos);
	sceGumRotateZ(rads);
	sceGuTexMode(GU_PSM_8888,0,0,img->swizzled);
	sceGuTexImage(0,img->texWidth,img->texHeight,
		img->texWidth,img->data);
	sceGumDrawArray(GU_TRIANGLE_STRIP,GU_COLOR_8888|GU_TEXTURE_32BITF 
		 |GU_VERTEX_32BITF|GU_TRANSFORM_3D,4, 0, verts);
	}
	sceGumPopMatrix();
		
}

// borrowed from graphics.c. i will replace it with something
// that uses memcpy.
//
void drawImageToImage(Image* srcImg, Image* dstImg, int dx, int dy,
					int srcX, int srcY, int srcW, int srcH)
{
	u32* dst = (u32*)dstImg->data;
	u32* src = (u32*)srcImg->data;
	int x,y;
	
	unswizzleImage(dstImg);
	unswizzleImage(srcImg);
	
	for(y = 0; y < srcH; y++)
	{
		for(x = 0; x < srcW; x++)
		{
			dst[(x+dx) + dstImg->texWidth * (y+dy)] =
				src[(x+srcX) + srcImg->texWidth * (y+srcY)]; 
		}
	}
	
	swizzleImage(srcImg);
	swizzleImage(dstImg);
}

void setImageZ(Image* img, float z)
{
	if(img==NULL) 
		return;
		
	img->vertices[0].z = z;
	img->vertices[1].z = z;
	img->vertices[2].z = z;
	img->vertices[3].z = z;
}

void stretchImage(Image* img, float xScale, float yScale)
{
	int i;
	
	if(img==NULL)
		return;
		
	for(i=0; i<4; i++)
	{
		img->vertices[i].x = img->vertices[i].x * xScale;
		img->vertices[i].y = img->vertices[i].y * yScale;
	}
	
	// as long as all tex operations deal with texWidth/Height then
	// this should be ok to do
	//
	img->width = img->width * xScale;
	img->height = img->height * yScale;
}

void restoreImage(Image* img)
{
	if( img == NULL )
		return;
	img->width = img->origWidth;
	img->height = img->origHeight;
	
	// restore the vertices
	
	img->vertices[0].x = -img->width/2; // top left
	img->vertices[0].y = -img->height/2; //
	
	img->vertices[1].x = -img->width/2; // bottom left
	img->vertices[1].y = img->height/2; //
	
	img->vertices[2].x = img->width/2; // top right
	img->vertices[2].y = -img->height/2; //
	
	img->vertices[3].x = img->width/2; // bottom right
	img->vertices[3].y = img->height/2; //
	
	
}

u32 getImagePixelAt(Image* img, int x, int y)
{
	u32 pixel=0;
	u32 *data;
	unswizzleImage(img);
	data = (u32*)(img->data);
	pixel = data[y*img->texWidth + x];
	swizzleImage(img);
	return pixel;
}

void setImagePixelAt(Image* img, int x, int y, u32 color)
{
	u32* data = NULL;
	
	unswizzleImage(img);
	
	data = (u32*)(img->data);
	data[(y * (img->texWidth)) + x] = color;
	
	swizzleImage(img);
}

// taken from graphics.c
//
int nextPowOf2(int dimension)
{
	int b = dimension;
	int n;
	for (n = 0; b != 0; n++) b >>= 1;
	b = 1 << n;
	if (b == 2 * dimension) b >>= 1;
	return b;
}

void initGraphics()
{	
	g_dlist = malloc(262144);
	g_dispNumber = 0;
	g_vram_base = (u32*) (0x40000000 | 0x04000000);
	fbp0 = NULL;
	fbp1 = (void*)0x88000;
	
	sceGuInit();
 
	sceGuStart(GU_DIRECT,g_dlist);
	sceGuDrawBuffer( GU_PSM_8888, fbp0, BUF_WIDTH );
	sceGuDispBuffer( SCR_WIDTH, SCR_HEIGHT, fbp1, BUF_WIDTH);
	sceGuDepthBuffer( (void*)0x110000, BUF_WIDTH );
	
	sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
	sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);
	sceGuDepthRange(65535,0);
	sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuFrontFace(GU_CCW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuAlphaFunc(GU_GREATER,0,0xff);
	sceGuEnable(GU_CULL_FACE); 
	sceGuEnable(GU_DEPTH_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_ALPHA_TEST);
	sceGuEnable(GU_TEXTURE_2D);
	
	sceGuTexMode(GU_PSM_8888,0,0,1);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	sceGuTexFilter(GU_NEAREST,GU_NEAREST);
	sceGuTexWrap(GU_CLAMP,GU_CLAMP);
	sceGuTexScale(1,1);
	sceGuTexOffset(0,0);
	sceGuAmbientColor(0xffffffff);
	
	sceGuFinish();
	sceGuSync(0,0);
	sceDisplayWaitVblankStart();
	sceGuDisplay(1);
}

void endGraphics()
{
	sceGuTerm();
}

void startDrawing()
{
	sceGuStart(GU_DIRECT,g_dlist);
	
	sceGuClearColor(0);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
	
	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumOrtho(0,480,272,0,-1,1);
	
	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();

	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
}

void endDrawing()
{
	sceGuFinish();
	sceGuSync(0,0);
}

void clearScreen()
{
	sceGuClearColor(0);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
}

void flipScreen()
{
	fbp0 = sceGuSwapBuffers();
	g_dispNumber ^= 1;
}

u32* getVramDrawBuffer()
{
	u32* vram = (u32*) g_vram_base;
	if (g_dispNumber== 0) 
		vram += FRAMEBUFFER_SIZE / sizeof(u32);
	return vram;
}

// adapted from graphics.c
//
void printTextScreen(const char* text, int x, int y, u32 color)
{
	int c, i, j, l;
	u8 *font;
	u32 *vram_ptr;
	u32 *vram;

	sceGuFinish();
	sceGuSync(0,0);

	sceGuStart(GU_DIRECT,g_dlist);

	for (c = 0; (unsigned)c < strlen(text); c++) 
	{
		if (x < 0 || x + 8 > SCR_WIDTH || y < 0 || y + 8 > SCR_HEIGHT) 
			break;
		char ch = text[c];
		
		//vram = getVramDrawBuffer() //+(unsigned int)(fbp0) 
			//	+ x + y * PSP_LINE_SIZE;
		if( g_dispNumber == 0 )
			vram = g_vram_base + (unsigned long)(fbp0)
				+ x + (y * PSP_LINE_SIZE);
		else
			vram = g_vram_base + FRAMEBUFFER_SIZE / sizeof(u32)
				+ x + (y * PSP_LINE_SIZE);
		
		font = &msx[ (int)ch * 8];
		for (i = l = 0; i < 8; i++, l += 8, font++) 
		{
			vram_ptr  = vram;
			for (j = 0; j < 8; j++) 
			{
				if ((*font & (128 >> j))) *vram_ptr = color;
				vram_ptr++;
			}
			vram += PSP_LINE_SIZE;
		}
		x += 8;
	}

}

// also borrowed from graphics.c
//
void printTextImage(const char* text, int x, int y, u32 color, 
					Image* img)
{
	int c, i, j, l;
	u8 *font;
	u32 *data_ptr;
	u32 *data;
	
	unswizzleImage(img);

	for (c = 0; c < strlen(text); c++) 
	{
		
		if (x < 0 || (x + 8) > img->origWidth || y < 0 
			|| y + 8 > (img->origHeight)) 
		{
			break;
		}
		
		char ch = text[c];
		data = (u32*)img->data + x + y * img->texWidth;
		
		font = &msx[ (int)ch * 8];
		for (i = l = 0; i < 8; i++, l += 8, font++) 
		{
			data_ptr  = data;
			for (j = 0; j < 8; j++) {
				if ((*font & (128 >> j))) *data_ptr = color;
				data_ptr++;
			}
			data += img->texWidth;
		}
		x += 8;
	}
	
	swizzleImage(img);
}
