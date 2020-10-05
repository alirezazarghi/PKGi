#include <stdint.h>
#include <orbis/Sysmodule.h>
#include <orbis/libkernel.h>
#include <orbis/VideoOut.h>
#include <orbis/GnmDriver.h>

#include <proto-include.h>

#ifndef GRAPHICS_H
#define GRAPHICS_H

// Dimensions
#define FRAME_WIDTH     1920
#define FRAME_HEIGHT    1080
#define FRAME_DEPTH        4
#define FRAMEBUFFER_NBR    2

// Color is used to pack together RGB information, and is used for every function that draws colored pixels.
typedef struct Color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Color;

typedef struct Image
{
    int width;
    int height;
    int channels;
    uint32_t* img;
    bool use_alpha;
} Image;

typedef struct FontSize
{
    int width;
    int height;
} FontSize;

class Graphics
{
public:
	Graphics();
	~Graphics();

	// General
	void WaitFlip();
	void SwapBuffer(int flipArgs);

	// Used for setting dimensions
	void setDimensions(int width, int height, int depth);
	int GetScreenWidth();
	int GetScreenHeight();

	void setActiveFrameBuffer(int idx);

	// Initializes flip event queue and allocates video memory for frame buffers
	int initializeFlipQueue(int video);
	int allocateVideoMem(size_t size, int alignment);
	void deallocateVideoMem();
	int allocateFrameBuffers(int video, int num);

	// Allocates display memory - bump allocator for video mem
	void * allocDisplayMemBump(size_t sz);

	// Frame buffer functions
	void frameWait(int video, int64_t flipArgs);
	void frameBufferSwap();
	void frameBufferClear();
	int64_t frameBufferGetLastFlip();
	void frameBufferSaveFlip(int64_t flipArgs);
	void frameBufferFill(Color color);

	// Draw functions
	void drawPixel(int x, int y, Color color);
	void drawRectangle(int x, int y, int width, int height, Color color);

	// Font functions
	int initFont(FT_Face *face, const char *fontPath, int fontSize);
	int setFontSize(FT_Face face, int fontSize);
	void drawText(char *txt, FT_Face face, int startX, int startY, Color bgColor, Color fgColor);
	void drawTextContainer(char *txt, FT_Face face, int startX, int startY, int maxWidth, int maxHeight, Color bgColor, Color fgColor);
	void getTextSize(char* txt, FT_Face face, FontSize* size);

	// Image functions
	void drawPNG(Image* img, int startX, int startY);
	void drawSizedPNG(Image* img, int startX, int startY, int width, int height);
	int loadPNG(Image* img, const char* imagePath);
	int loadPNGFromMemory(Image* img, unsigned char* data, int len);
	void unloadPNG(Image* img);

private:
	FT_Library ftLib;

	// Video
	int video;

	// Pointer to direct video memory and the frame buffer array
	void *VideoMem;
	void **FrameBuffers;

	// Points to the top of video memory, used for the bump allocator
	uintptr_t VideoMemSP;

	// Event queue for flips
	OrbisKernelEqueue FlipQueue;

	// Attributes for the frame buffers
	OrbisVideoOutBufferAttribute Attr;

	// Direct memory offset and allocation size
	off_t DirectMemOff;
	size_t DirectMemAllocationSize;

	// Meta info for frame information
	int Width;
	int Height;
	int Depth;

	// Frame buffer size and count - initialized by allocateFrameBuffers()
	int FrameBufferSize;
	int FrameBufferCount;

	// Active frame buffer for swapping
	int ActiveFrameBufferIdx;

	// flipArgLog
	int64_t* flipArgLog;
};

#endif
