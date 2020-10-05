#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Graphics.h"

#define STBI_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// 
/* alpha-blending, a simple vectorized version */
typedef unsigned int v4ui __attribute__((ext_vector_type(4)));
static inline uint32_t mix_color(const uint32_t* const bg, const uint32_t* const fg)
{
    uint32_t a = *fg >> 24;

    if (a == 0)         return *bg;
    else if (a == 0xFF) return *fg;

    v4ui vin = (v4ui){ *fg, *fg, *bg, *bg }; // vload
    v4ui vt = (v4ui){ 0x00FF00FF, 0x0000FF00, 0x00FF00FF, 0x0000FF00 }; // vload

    vin &= vt;
    vt = (v4ui){ a, a, 255 - a, 255 - a }; // vload, reuse t
    vin *= vt;

    vt = (v4ui){ vin.x + vin.z, vin.y + vin.w, 0xFF00FF00, 0x00FF0000 };
    vin = (v4ui){ vt.x & vt.z, vt.y & vt.w };

    uint32_t Fg = a + ((*bg >> 24) * (255 - a) / 255);
    return (Fg << 24) | ((vin.x | vin.y) >> 8);
}

// Linearly interpolate x with y over s
inline float lerp(float x, float y, float s)
{
    return x * (1.0f - s) + y * s;
}

Graphics::Graphics() {
	int rc;

	// Setup variable
	Width = FRAME_WIDTH;
	Height = FRAME_HEIGHT;
	Depth = FRAME_DEPTH;
	video = -1;
	ActiveFrameBufferIdx = 0;
	FrameBufferSize = 0;
	FrameBufferCount = 0;
	FrameBufferSize = Width * Height * Depth;

	// Load freetype module
	rc = sceSysmoduleLoadModule(0x009A);
	if (rc < 0)
	{
		printf("[ERROR] Failed to load freetype module\n");
	}

	// Initialize freetype
	rc = FT_Init_FreeType(&ftLib);
	if (rc != 0)
	{
		printf("[ERROR] Failed to initialize freetypee\n");
	}

	// Open a handle to video out
	video = sceVideoOutOpen(ORBIS_VIDEO_USER_MAIN, ORBIS_VIDEO_OUT_BUS_MAIN, 0, 0);
	if (video < 0)
	{
		printf("[ERROR] Failed to open a video out handle\n");
	}
	else {
		printf("[VIDEO] Handle is %i (0x%08x)\n", video, video);
	}

	// Create a queue for flip events
	if (initializeFlipQueue(video) < 0)
	{
		sceVideoOutClose(video);

		printf("[ERROR] Failed to create an event queue\n");
	}

	// Allocate direct memory for the frame buffers
	rc = allocateVideoMem(0xC000000, 0x200000);

	if (rc < 0)
	{
		sceVideoOutClose(video);

		printf("[ERROR] Failed to allocate video memory\n");
	}

	// Set the frame buffers
	printf("[INFO] Allocating framebuffer ...\n");
	rc = allocateFrameBuffers(video, FRAMEBUFFER_NBR);

	if (rc < 0)
	{
		deallocateVideoMem();
		sceVideoOutClose(video);

		printf("[ERROR] Failed to allocate frame buffers from video memory\n");
	}

	setActiveFrameBuffer(0);

	// Set the flip rate
	sceVideoOutSetFlipRate(video, 0);
}

Graphics::~Graphics() {
	deallocateVideoMem();
	sceVideoOutClose(video);
}

void Graphics::WaitFlip() {
	//printf("[INFO] WaitFlip ...");

	frameWait(video, frameBufferGetLastFlip() + 1);
	frameBufferClear();
}

void Graphics::SwapBuffer(int flipArgs) {
	sceGnmFlushGarlic();
	sceVideoOutSubmitFlip(video, ActiveFrameBufferIdx, ORBIS_VIDEO_OUT_FLIP_VSYNC, flipArgs);
	frameBufferSaveFlip(flipArgs);

	frameBufferSwap();
}


// setActiveFrameBuffer takes the given index and sets it to the active index. Returns nothing.
void Graphics::setActiveFrameBuffer(int idx)
{
    ActiveFrameBufferIdx = idx;
}

// initializeFlipQueue creates an event queue for sceVideo flip events. Returns error code.
int Graphics::initializeFlipQueue(int video)
{
    // Create an event queue
    int rc = sceKernelCreateEqueue(&FlipQueue, "homebrew flip queue");

    if (rc < 0)
        return rc;

    // Unknown if we *have* to do this, but most ELFs seem to do this, so we'll do it too
    sceVideoOutAddFlipEvent(FlipQueue, video, 0);
    return rc;
}

// allocateVideoMem takes a given size and alignment, and allocates and maps direct memory and sets the VideoMem pointer to it.
// Returns error code.
int Graphics::allocateVideoMem(size_t size, int alignment)
{
    int rc;

    // Align the allocation size
    DirectMemAllocationSize = (size + alignment - 1) / alignment * alignment;

    // Allocate memory for display buffer
    rc = sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), DirectMemAllocationSize, alignment, 3, &DirectMemOff);

    if (rc < 0)
    {
		printf("sceKernelAllocateDirectMemory: an error occured. (%i => 0x%08x)", rc, rc);
        DirectMemAllocationSize = 0;
        return rc;
    }

	printf("DirectMemOff: %p", (void*)DirectMemOff);
	printf("DirectMemAllocationSize: %p", (void*)DirectMemAllocationSize);
	printf("alignment: %i (0x%08x)", alignment, alignment);

    // Map the direct memory
    rc = sceKernelMapDirectMemory(&VideoMem, DirectMemAllocationSize, 0x33, 0, DirectMemOff, alignment);

	printf("VideoMem: %p", VideoMem);

    if (rc < 0)
    {
		printf("sceKernelMapDirectMemory: an error occured. (%i => 0x%08x)", rc, rc);

        sceKernelReleaseDirectMemory(DirectMemOff, DirectMemAllocationSize);

        DirectMemOff = 0;
        DirectMemAllocationSize = 0;

        return rc;
    }

    // Set the stack pointer to the beginning of the buffer
    VideoMemSP = (uintptr_t)VideoMem;

    return rc;
}

// deallocateVideoMem is a function to be called upon error, and it'll free the mapped direct memory, zero out meta-data, and
// free the FrameBuffer array. Returns nothing.
void Graphics::deallocateVideoMem()
{
    // Free the direct memory
    sceKernelReleaseDirectMemory(DirectMemOff, DirectMemAllocationSize);

    // Zero out meta data
    VideoMem = 0;
    VideoMemSP = 0;
    DirectMemOff = 0;
    DirectMemAllocationSize = 0;

    // Free the frame buffer array
    free(FrameBuffers);
    FrameBuffers = 0;
}

// allocateFrameBuffer takes a given video handle and number of frames, and allocates an array with the give number of frames
// for the FrameBuffers, and also sets the attributes for the video handle. Finally, it registers the buffers to the handle
// before returning. Returns error code.
int Graphics::allocateFrameBuffers(int video, int num)
{
	flipArgLog = (int64_t*)calloc(num, sizeof(int64_t));

    // Allocate frame buffers array
    FrameBuffers = (void **)calloc(num, sizeof(void *));

	printf("handle: %i (0x%08x)\n", video, video);

	printf("flipArgLog: %p\n", flipArgLog);
	printf("FrameBuffers: %p\n", FrameBuffers);

    // Allocate the framebuffer flip args log and set default value
    for (int dpBuffer = 0; dpBuffer < num; dpBuffer++)
        flipArgLog[dpBuffer] = -2;

    // Set the display buffers
    for (int i = 0; i < num; i++)
        FrameBuffers[i] = allocDisplayMemBump(FrameBufferSize);

    // Set SRGB pixel format
    sceVideoOutSetBufferAttribute(&Attr, 0x80000000, 1, 0, Width, Height, Width);
	int ret = sceVideoOutRegisterBuffers(video, 0, FrameBuffers, num, &Attr);
	printf("sceVideoOutRegisterBuffers: 0x%08x\n", ret);

    FrameBufferCount = num;

    // Register the buffers to the video handle
	return ret;
}

// allocDisplayMem is essentially a bump allocator, which will allocate space off of VideoMem for FrameBuffers. Returns a pointer
// to the chunk requested.
void* Graphics::allocDisplayMemBump(size_t sz)
{
    // Essentially just bump allocation
    void *allocatedPtr = (void *)VideoMemSP;
    VideoMemSP += sz;

    return allocatedPtr;
}

int64_t Graphics::frameBufferGetLastFlip() {
	//printf("[INFO] GetLastFlip\n");
    return flipArgLog[ActiveFrameBufferIdx];
}

void Graphics::frameBufferSaveFlip(int64_t flipArgs) {
	//printf("[INFO] SaveFlip\n");
    flipArgLog[ActiveFrameBufferIdx] = flipArgs;
}

// frameBufferSwap swaps the ActiveFrameBufferIdx. Should be called at the end of every draw loop iteration. Returns nothing.
void Graphics::frameBufferSwap()
{
	//printf("[INFO] BufferSwap\n");
    // Swap the frame buffer for some perf
    ActiveFrameBufferIdx = (ActiveFrameBufferIdx + 1) % FrameBufferCount;
}

// frameBufferClear fills the frame buffer with white pixels. Returns nothing.
void Graphics::frameBufferClear()
{
    // Clear the screen with a black frame buffer
    Color blank = { 0, 0, 0, 255 };
    frameBufferFill(blank);
}

// frameBufferFill fills the frame buffer with pixels of the given red, green, and blue values. Returns nothing.
void Graphics::frameBufferFill(Color color)
{
    int xPos, yPos;

    // Draw row-by-row, column-by-column
    for (yPos = 0; yPos < Height; yPos++)
    {
        for (xPos = 0; xPos < Width; xPos++)
        {
            drawPixel(xPos, yPos, color);
        }
    }
}

// frameWait waits on the event queue before swapping buffers
void Graphics::frameWait(int video, int64_t flipArgs)
{
	//printf("[INFO] FrameWait\n");
    OrbisKernelEvent evt;
    int count;

    // If the video handle is not initialized, bail out. This is mostly a failsafe, this should never happen.
    if (video == 0)
        return;

    for (;;)
    {
        OrbisVideoOutFlipStatus flipStatus;

        // Get the flip status and check the arg for the given frame ID
        sceVideoOutGetFlipStatus(video, &flipStatus);

        if (flipStatus.flipArg >= flipArgs)
            break;

        // Wait on next flip event
        if (sceKernelWaitEqueue(FlipQueue, &evt, 1, &count, 0) != 0)
            break;
    }
}

// drawPixel draws the given color to the given x-y coordinates in the frame buffer. Returns nothing.
void Graphics::drawPixel(int x, int y, Color color)
{
    // Get pixel location based on pitch
    int pixel = (y * Width) + x;

    // Encode to 24-bit color
    uint32_t encodedColor = 0x80000000 + (color.a << 24) + (color.r << 16) + (color.g << 8) + color.b;

    // Draw to the frame buffer
    if (color.a == 255) {
        ((uint32_t*)FrameBuffers[ActiveFrameBufferIdx])[pixel] = encodedColor;
    }
    else {
        uint32_t* pixel_ptr = &((uint32_t*)FrameBuffers[ActiveFrameBufferIdx])[pixel];
        *pixel_ptr = mix_color(pixel_ptr, &encodedColor); // Consumn alot of CPU !
    }
}

// drawRectangle draws a rectangle at the given x-y xoordinates with the given width, height, and color to the frame
// buffer. Returns nothing.
void Graphics::drawRectangle(int x, int y, int width, int height, Color color)
{
    int xPos, yPos;

    // Draw row-by-row, column-by-column
    for (yPos = y; yPos < y + height; yPos++)
    {
        for (xPos = x; xPos < x + width; xPos++)
        {
            drawPixel(xPos, yPos, color);
        }
    }
}

// initFont takes a font path and a font size, initializes the face pointer, and sets the font size. Returns error code.
int Graphics::initFont(FT_Face *face, const char *fontPath, int fontSize)
{
    int rc;

    rc = FT_New_Face(ftLib, fontPath, 0, face);

    if (rc < 0)
        return rc;

    return FT_Set_Pixel_Sizes(*face, 0, fontSize);
}

int Graphics::setFontSize(FT_Face face, int fontSize)
{
    return FT_Set_Pixel_Sizes(face, 0, fontSize);
}

// drawText uses the given face, starting coordinates, and background/foreground colors, and renders txt to the
// frame buffer. Returns nothing.
void Graphics::drawText(char *txt, FT_Face face, int startX, int startY, Color bgColor, Color fgColor)
{
    int rc;
    int xOffset = 0;
    int yOffset = 0;

    // Get the glyph slot for bitmap and font metrics
    FT_GlyphSlot slot = face->glyph;

    // Iterate each character of the text to write to the screen
    for (int n = 0; n < strlen(txt); n++)
    {
        FT_UInt glyph_index;

        // Get the glyph for the ASCII code
        glyph_index = FT_Get_Char_Index(face, txt[n]);

        // Load and render in 8-bit color
        rc = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);

        if (rc)
            continue;

        rc = FT_Render_Glyph(slot, ft_render_mode_normal);

        if (rc)
            continue;

        // If we get a newline, increment the y offset, reset the x offset, and skip to the next character
        if (txt[n] == '\n')
        {
            xOffset = 0;
            yOffset += slot->bitmap.width * 2;

            continue;
        }

        // Parse and write the bitmap to the frame buffer
        for (int yPos = 0; yPos < slot->bitmap.rows; yPos++)
        {
            for (int xPos = 0; xPos < slot->bitmap.width; xPos++)
            {
                // Decode the 8-bit bitmap
                char pixel = slot->bitmap.buffer[(yPos * slot->bitmap.width) + xPos];

                // Get new pixel coordinates to account for the character position and baseline, as well as newlines
                int x = startX + xPos + xOffset + slot->bitmap_left;
                int y = startY + yPos + yOffset - slot->bitmap_top;

                // Linearly interpolate between the foreground and background for smoother rendering
                uint8_t r = lerp(float(fgColor.r), float(bgColor.r), 255.0f);
                uint8_t g = lerp(float(fgColor.g), float(bgColor.g), 255.0f);
                uint8_t b = lerp(float(fgColor.b), float(bgColor.b), 255.0f);

                // Create new color struct with lerp'd values
                Color finalColor = { r, g, b, 255 };

                // We need to do bounds checking before commiting the pixel write due to our transformations, or we
                // could write out-of-bounds of the frame buffer
                if (x < 0 || y < 0 || x >= Width || y >= Height)
                    continue;

                // If the pixel in the bitmap isn't blank, we'll draw it
                if(pixel != 0x00)
                    drawPixel(x, y, finalColor);
            }
        }

        // Increment x offset for the next character
        xOffset += slot->advance.x >> 6;
    }
}

void Graphics::getTextSize(char* txt, FT_Face face, FontSize* size) {
    int rc;
    int xOffset = 0;
    int yOffset = 0;
  
    // Get the glyph slot for bitmap and font metrics
    FT_GlyphSlot slot = face->glyph;

    // Iterate each character of the text to write to the screen
    for (int n = 0; n < strlen(txt); n++)
    {
        FT_UInt glyph_index;

        // Get the glyph for the ASCII code
        glyph_index = FT_Get_Char_Index(face, txt[n]);

        // Load and render in 8-bit color
        rc = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);

        if (rc)
            continue;

        rc = FT_Render_Glyph(slot, ft_render_mode_normal);

        if (rc)
            continue;

        // If we get a newline, increment the y offset, reset the x offset, and skip to the next character
        if (txt[n] == '\n')
        {
            xOffset = 0;
            yOffset += slot->bitmap.width * 2;

            continue;
        }

        // Parse and write the bitmap to the frame buffer
        for (int yPos = 0; yPos < slot->bitmap.rows; yPos++)
        {
            for (int xPos = 0; xPos < slot->bitmap.width; xPos++)
            {
                // Get new pixel coordinates to account for the character position and baseline, as well as newlines
                int x = xPos + xOffset + slot->bitmap_left;
                int y = yPos + yOffset - slot->bitmap_top;
            }
        }

        // Increment x offset for the next character
        xOffset += slot->advance.x >> 6;
    }

    size->width = xOffset;
    size->height = yOffset - (face->glyph->metrics.horiBearingY >> 6);
}

void Graphics::drawTextContainer(char *txt, FT_Face face, int startX, int startY, int maxWidth, int maxHeight, Color bgColor, Color fgColor)
{
	int rc;
	int xOffset = 0;
	int yOffset = 0;
	int cWidth = 0;
	int cHeight = 0;
	int padding = 5;

	// Get the glyph slot for bitmap and font metrics
	FT_GlyphSlot slot = face->glyph;

	// Iterate each character of the text to write to the screen
	for (int n = 0; n < strlen(txt); n++)
	{
		FT_UInt glyph_index;

		// Get the glyph for the ASCII code
		glyph_index = FT_Get_Char_Index(face, txt[n]);

		// Load and render in 8-bit color
		rc = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);

		if (rc)
			continue;

		rc = FT_Render_Glyph(slot, ft_render_mode_normal);

		if (rc)
			continue;

		// If we get a newline, increment the y offset, reset the x offset, and skip to the next character
		if (txt[n] == '\n')
		{
			xOffset = 0;
			yOffset += slot->bitmap.width * 2;

			continue;
		}

		// word wrapping checks
		if ((cWidth + slot->bitmap.width) >= maxWidth)
		{
			cWidth = 0;
			cHeight += slot->bitmap.width * 2;
			xOffset = 0;
			yOffset += (slot->bitmap.width * 2) + padding;
		}
		else
		{
			cWidth += slot->bitmap.width;
		}


		//printf("%d - [WW] cWidth(%d) cHeight(%d) xOffset(%d) yOffset(%d) letter(%c)\n", n, cWidth, cHeight, xOffset, yOffset, txt[n]);
		//sceKernelUsleep(1000);

		// Parse and write the bitmap to the frame buffer
		for (int yPos = 0; yPos < slot->bitmap.rows; yPos++)
		{
			for (int xPos = 0; xPos < slot->bitmap.width; xPos++)
			{
				// Decode the 8-bit bitmap
				char pixel = slot->bitmap.buffer[(yPos * slot->bitmap.width) + xPos];

				// Get new pixel coordinates to account for the character position and baseline, as well as newlines
				int x = startX + xPos + xOffset + slot->bitmap_left;
				int y = startY + yPos + yOffset - slot->bitmap_top;

				// Linearly interpolate between the foreground and background for smoother rendering
				uint8_t r = lerp(float(fgColor.r), float(bgColor.r), 255.0f);
				uint8_t g = lerp(float(fgColor.g), float(bgColor.g), 255.0f);
				uint8_t b = lerp(float(fgColor.b), float(bgColor.b), 255.0f);

				// Create new color struct with lerp'd values
				Color finalColor = { r, g, b, 255 };

				// We need to do bounds checking before commiting the pixel write due to our transformations, or we
				// could write out-of-bounds of the frame buffer
				if (x < 0 || y < 0 || x >= Width || y >= Height)
					continue;

				// If the pixel in the bitmap isn't blank, we'll draw it
				if (pixel != 0x00)
					drawPixel(x, y, finalColor);
			}
		}

		// Increment x offset for the next character
		xOffset += slot->advance.x >> 6;
	}
}

int Graphics::loadPNG(Image* img, const char* imagePath) {
	printf("Loading png ...");

    img->img = (uint32_t*)stbi_load(imagePath, &img->width, &img->height, &img->channels, STBI_rgb_alpha);
    if (img->img == NULL)
    {
        printf("[ERROR] Failed to load img: %s\n", stbi_failure_reason());
        return -1;
    }

	printf("PNG Loaded !");

    img->use_alpha = true;

    return 0;
}

int Graphics::loadPNGFromMemory(Image* img, unsigned char* data, int len) {
	printf("Loading png from memory ...");

	img->img = (uint32_t*)stbi_load_from_memory((stbi_uc*)data, len, &img->width, &img->height, &img->channels, STBI_rgb_alpha);
	if (img->img == NULL)
	{
		printf("[ERROR] Failed to load img: %s\n", stbi_failure_reason());
		return -1;
	}

	printf("PNG Loaded !");

	img->use_alpha = false;
	return 0;
}

void Graphics::drawPNG(Image* img, int startX, int startY)
{

    if (img == NULL)
    {
        printf("[ERROR] Image is not initialized !");
        return;
    }

    // Iterate the bitmap and draw the pixels
    for (int yPos = 0; yPos < img->height; yPos++)
    {
        for (int xPos = 0; xPos < img->width; xPos++)
        {
            // Decode the 32-bit color
            uint32_t encodedColor = img->img[(yPos * img->width) + xPos];

            // Get new pixel coordinates to account for start coordinates
            int x = startX + xPos;
            int y = startY + yPos;

            // Re-encode the color
            uint8_t A = encodedColor >> 24;
            if (!img->use_alpha) {
                A = 0xFF;
            }

            uint8_t B = encodedColor >> 16;
            uint8_t G = encodedColor >> 8;
            uint8_t R = encodedColor;

            Color color = {R, G, B, A};

            // Do some bounds checking to make sure the pixel is actually inside the frame buffer
            if (xPos < 0 || yPos < 0 || xPos >= img->width || yPos >= img->height)
                continue;

            drawPixel(x, y, color);
        }
    }
}

void Graphics::drawSizedPNG(Image* img, int startX, int startY, int width, int height)
{

    if (img == NULL)
    {
        printf("[ERROR] Image is not initialized !");
        return;
    }

    int x_ratio = 1;
    int y_ratio = 1;

    if (width <= 0 || height <= 0) {
        printf("[ERROR] Trying to divide by 0 !\n");
    }
    else {
        x_ratio = (int)((img->width << 16) / width); // +1
        y_ratio = (int)((img->height << 16) / height); // +1
    }

    int x2, y2;

    // Iterate the bitmap and draw the pixels
    for (int yPos = 0; yPos < height; yPos++)
    {
        for (int xPos = 0; xPos < width; xPos++)
        {
            // Calculate 
            x2 = ((xPos * x_ratio) >> 16);
            y2 = ((yPos * y_ratio) >> 16);

            // Decode the 32-bit color
            uint32_t encodedColor = img->img[(y2 * img->width) + x2];

            // Get new pixel coordinates to account for start coordinates
            int x = startX + xPos;
            int y = startY + yPos;

            // Re-encode the color
            uint8_t r = (uint8_t)(encodedColor >> 0);
            uint8_t g = (uint8_t)(encodedColor >> 8);
            uint8_t b = (uint8_t)(encodedColor >> 16);
            uint8_t a = (uint8_t)(encodedColor >> 24);

            if (!img->use_alpha) {
                a = 0xFF;
            }

            Color color = { r, g, b, a };
            drawPixel(x, y, color);
        }
    }
}

void Graphics::unloadPNG(Image* img) {
	if (img->img)
		stbi_image_free(img->img);

    img->img = NULL;
    img->width = 0;
    img->height = 0;
    img->channels = 0;
}

int Graphics::GetScreenWidth() {
    return Width;
}

int Graphics::GetScreenHeight() {
    return Height;
}
