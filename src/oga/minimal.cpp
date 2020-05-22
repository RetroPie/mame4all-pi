/*
*/

#include "minimal.h"
#include "driver.h"

#include <SDL.h>
#include <assert.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <go2/display.h>
#include <go2/input.h>
#include <go2/audio.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm/drm_fourcc.h>


#include <gbm.h>


#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


#define BUFFER_MAX (3)

typedef struct go2_display
{
    int fd;
    uint32_t connector_id;
    drmModeModeInfo mode;
    uint32_t width;
    uint32_t height;
    uint32_t crtc_id;
} go2_display_t;

typedef struct go2_surface
{
    go2_display_t* display;
    uint32_t gem_handle;
    uint64_t size;
    int width;
    int height;
    int stride;
    uint32_t format;
    int prime_fd;
    bool is_mapped;
    uint8_t* map;
} go2_surface_t;

typedef struct buffer_surface_pair
{
    struct gbm_bo* gbmBuffer;
    go2_surface_t* surface;
} buffer_surface_pair_t;

typedef struct go2_context
{
    go2_display_t* display;    
    int width;
    int height;
    go2_context_attributes_t attributes;
    struct gbm_device* gbmDevice;
    EGLDisplay eglDisplay;
    struct gbm_surface* gbmSurface;
    EGLSurface eglSurface;
    EGLContext eglContext;
    uint32_t drmFourCC;
    buffer_surface_pair_t bufferMap[BUFFER_MAX];
    int bufferCount;
} go2_context_t;


//static SDL_Surface* sdlscreen = NULL;

go2_display_t* go2_display = NULL;
go2_surface_t* go2_surface = NULL;
go2_presenter_t* go2_presenter = NULL;
go2_context_t* go2_context3D = NULL;

unsigned long 			gp2x_dev[3];
unsigned char 			*gp2x_screen8;
unsigned short 			*gp2x_screen15;
void                    *rpi_screen;
volatile unsigned short 	gp2x_palette[512];
int 				rotate_controls=0;

static int surface_width;
static int surface_height;

#define MAX_SAMPLE_RATE (44100*2)

void gp2x_video_flip(void)
{
    DisplayScreen();
}

extern void gles2_palette_changed();

void gles2_palette_changed();
void (*gles2_draw)(void *screen, int width, int height);

void gp2x_video_setpalette(void)
{
	gles2_palette_changed();
}

extern void keyprocess(int /*SDLKey*/ inkey, SDL_bool pressed);
extern void joyprocess(Uint8 button, SDL_bool pressed, Uint8 njoy);
extern void mouse_motion_process(int x, int y);
extern void mouse_button_process(Uint8 button, SDL_bool pressed);

unsigned long gp2x_joystick_read()
{
    SDL_Event event;

	//Reset mouse incase there is no motion
	mouse_motion_process(0,0);

	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_KEYDOWN:
				keyprocess(event.key.keysym.sym, SDL_TRUE);
				break;
			case SDL_KEYUP:
				keyprocess(event.key.keysym.sym, SDL_FALSE);
				break;
			case SDL_JOYBUTTONDOWN:
				joyprocess(event.jbutton.button, SDL_TRUE, event.jbutton.which);
				break;
			case SDL_JOYBUTTONUP:
				joyprocess(event.jbutton.button, SDL_FALSE, event.jbutton.which);
				break;
			case SDL_MOUSEMOTION:
				mouse_motion_process(event.motion.xrel, event.motion.yrel);
				break;
			case SDL_MOUSEBUTTONDOWN:
				mouse_button_process(event.button.button, SDL_TRUE);
				break;
			case SDL_MOUSEBUTTONUP:
				mouse_button_process(event.button.button, SDL_FALSE);
				break;

			default:
				break;
		}
	}

	
	return 0;
}

void gp2x_sound_volume(int l, int r)
{
}

void gp2x_timer_delay(unsigned long ticks)
{
	unsigned long ini=gp2x_timer_read();
	while (gp2x_timer_read()-ini<ticks);
}


unsigned long gp2x_timer_read(void)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC_RAW, &now);

	return ((unsigned long long)now.tv_sec * 1000000LL + (now.tv_nsec / 1000LL));

}

void gles2_create(int display_width, int display_height, int bitmap_width, int bitmap_height, int depth);
void gles2_destroy();
void gles2_palette_changed();

EGLDisplay display = NULL;
EGLSurface surface = NULL;
static EGLContext context = NULL;
//static EGL_DISPMANX_WINDOW_T nativewindow;

static SDL_GLContext glcontext = NULL;

void exitfunc()
{
	SDL_Quit();
	gp2x_frontend_deinit();
}

SDL_Joystick* myjoy[4];

int init_SDL(void)
{
	myjoy[0]=0;
	myjoy[1]=0;
	myjoy[2]=0;
	myjoy[3]=0;

	printf("[trngaje] minimal.cpp:init_SDL\n");
    if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        return(0);
    }
	// for sdl1.2
    //sdlscreen = SDL_SetVideoMode(0,0, 16, SDL_SWSURFACE);
	// for sdl2.0 by trngaje
#if 1	
	SDL_Window *window = SDL_CreateWindow("mame4all", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		0, 0, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);
		
	glcontext = SDL_GL_CreateContext(window);

	printf("[trngaje] window= %x, glcontext=%x\n", window, glcontext);	
#endif
	// now you can make GL calls.
	//glClearColor(0,0,0,1);
	//glClear(GL_COLOR_BUFFER_BIT);
	//SDL_GL_SwapWindow(window);
					  
	//We handle up to four joysticks
	if(SDL_NumJoysticks()) 
	{
		printf("[trngaje] minimal.cpp:init_SDL:SDL_NumJoysticks=%d\n", SDL_NumJoysticks());
		int i;
    	SDL_JoystickEventState(SDL_ENABLE);
		
		for(i=0;i<SDL_NumJoysticks();i++) {	
			myjoy[i]=SDL_JoystickOpen(i);
		}
		if(myjoy[0]) 
		{
			logerror("Found %d joysticks\n",SDL_NumJoysticks());
			printf("Found %d joysticks\n",SDL_NumJoysticks());
		}
	}
#if 1
//	SDL_EventState(SDL_ACTIVEEVENT,SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT,SDL_IGNORE);
//	SDL_EventState(SDL_VIDEORESIZE,SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT,SDL_IGNORE);
	SDL_ShowCursor(SDL_DISABLE);
#endif
    //Initialise dispmanx
    //bcm_host_init();
    
	//Clean exits, hopefully!
	atexit(exitfunc);
    
    return(1);
}

void deinit_SDL(void)
{
#if 0
    if(sdlscreen)
    {
        SDL_FreeSurface(sdlscreen);
        sdlscreen = NULL;
    }
#endif	
	if(glcontext)
	{
		// Once finished with OpenGL functions, the SDL_GLContext can be deleted.
		SDL_GL_DeleteContext(glcontext);  
	}
	
    SDL_Quit();
    
    //bcm_host_deinit();
}


void gp2x_deinit(void)
{
	printf("[trngaje] gp2x_deinit++\n");
	int ret;

    gles2_destroy();
    // Release OpenGL resources
    eglMakeCurrent( display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    eglDestroySurface( display, surface );
    eglDestroyContext( display, context );
    eglTerminate( display );
#if 0

	if(gp2x_screen8) free(gp2x_screen8);
	if(gp2x_screen15) free(gp2x_screen15);
	gp2x_screen8=0;
	gp2x_screen15=0;
	rpi_screen=0;
#endif

	if (go2_context3D != NULL)
	{
		go2_context_destroy(go2_context3D);
		go2_context3D = NULL;
	}
	
	if (go2_presenter != NULL)
	{
		go2_presenter_destroy(go2_presenter);
		go2_presenter = NULL;
	}


	if (go2_display != NULL)
	{
		go2_display_destroy(go2_display);
		go2_display = NULL;
	} 
}

static uint32_t display_adj_width, display_adj_height;		//display size minus border

unsigned char ucbpp=8; // default

void gp2x_set_video_mode(struct osd_bitmap *bitmap, int bpp,int width,int height)
{
	uint32_t display_width, display_height, surface_size;
	uint32_t display_x=0, display_y=0;
	float display_ratio,game_ratio;

	printf("[trngaje] gp2x_set_video_mode bpp=%d, width=%d, height=%d\n", bpp, width, height);
	ucbpp = bpp;
	if (go2_display == NULL)
		go2_display = go2_display_create();
	printf("[trngaje] gp2x_set_video_mode-step1\n");
	
	if (go2_presenter == NULL)
		go2_presenter = go2_presenter_create(go2_display, DRM_FORMAT_XRGB8888, 0xff080808);  // ABGR
	printf("[trngaje] gp2x_set_video_mode-step2\n");
	go2_context_attributes_t attr;
	attr.major = 1;
	attr.minor = 0;
	attr.red_bits = 8;
	attr.green_bits = 8;
	attr.blue_bits = 8;
	attr.alpha_bits = 8;
	attr.depth_bits = 24;
	attr.stencil_bits = 0;
	printf("[trngaje] gp2x_set_video_mode-step3\n");
	if (go2_context3D == NULL)
		go2_context3D = go2_context_create(go2_display, width, height, &attr);
	printf("[trngaje] gp2x_set_video_mode-step4\n");



#if 1
	if (go2_surface == NULL)
		go2_surface = go2_surface_create(go2_display, width, height, DRM_FORMAT_RGB565);
#else	
	go2_surface = (go2_surface_t*)malloc(sizeof(*go2_surface));
	if (!go2_surface)
	{
		printf("go2_surface malloc failed.\n");
	}

    memset(go2_surface, 0, sizeof(*go2_surface));


    struct drm_mode_create_dumb args = {0};
    args.width = width;
    args.height = height;
	//if (bitmap->depth == 8)
	//	args.bpp = 8;
	//else
		args.bpp = 16;
    args.flags = 0;

    int io = drmIoctl(go2_display->fd, DRM_IOCTL_MODE_CREATE_DUMB, &args);
    if (io < 0)
    {
        printf("go2_display->fdDRM_IOCTL_MODE_CREATE_DUMB failed.\n");
    }


    go2_surface->display = go2_display;
    go2_surface->gem_handle = args.handle;
    go2_surface->size = args.size;
    go2_surface->width = width;
    go2_surface->height = height;
    go2_surface->stride = args.pitch;
    go2_surface->format = DRM_FORMAT_RGB565;	
	
#endif	
	//gp2x_screen15 = (unsigned short *)go2_surface_map(go2_surface);
	//gp2x_screen8=(unsigned char *)go2_surface_map(go2_surface);	
	
	
	
	surface_width = width;
	surface_height = height;
	
	surface_size = width*height;
	
	surface = go2_context3D->eglSurface;
	display = go2_context3D->eglDisplay;
	context = go2_context3D->eglContext;
#if 1	
	gp2x_screen8=0;
	gp2x_screen15=0;
	
    if (bitmap->depth == 8)
    {
    	gp2x_screen8=(unsigned char *) calloc(1, surface_size);
    	rpi_screen=gp2x_screen8;
    	gles2_draw = gles2_draw_8;
    }
    else
    {
        surface_size = surface_size * 2;
    	gp2x_screen15=(unsigned short *) calloc(1, surface_size);
    	rpi_screen=gp2x_screen15;
    	gles2_draw = gles2_draw_16;
    }
#else
	if (bitmap->depth == 8)
	{
		rpi_screen=gp2x_screen8;
    	gles2_draw = gles2_draw_8;
	}
	else
	{
        surface_size = surface_size * 2;
    	//gp2x_screen15=(unsigned short *) calloc(1, surface_size);
    	rpi_screen=gp2x_screen15;
    	gles2_draw = gles2_draw_16;		
	}
#endif
	// get an EGL display connection
	//display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	//assert(display != EGL_NO_DISPLAY);
	

#if 1
	// initialize the EGL display connection
	EGLBoolean result = eglInitialize(display, NULL, NULL);
	assert(EGL_FALSE != result);
	
	// get an appropriate EGL frame buffer configuration
	EGLint num_config;
	EGLConfig config;
	static const EGLint attribute_list[] =
	{
	    EGL_RED_SIZE, 8,
	    EGL_GREEN_SIZE, 8,
	    EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8, 
	    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT, // by trngaje
	    EGL_NONE
	};
	
	

	result = eglChooseConfig(display, attribute_list, &config, 1, &num_config);
	assert(EGL_FALSE != result);

	result = eglBindAPI(EGL_OPENGL_ES_API);
	assert(EGL_FALSE != result);

	// create an EGL rendering context
	static const EGLint context_attributes[] =
	{
	    EGL_CONTEXT_CLIENT_VERSION, 2,
	    EGL_NONE
	};
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
	assert(context != EGL_NO_CONTEXT);
#endif
	// create an EGL window surface
	//int32_t success = graphics_get_display_size(0, &display_width, &display_height);
	//assert(success >= 0);
	display_width = go2_surface_width_get(go2_surface);
	display_height = go2_surface_height_get(go2_surface);
	display_adj_width = display_width - (options.display_border * 2);
	display_adj_height = display_height - (options.display_border * 2);

	if (options.display_smooth_stretch) 
	{
		//We use the dispmanx scaler to smooth stretch the display
		//so GLES2 doesn't have to handle the performance intensive postprocessing

	    uint32_t sx, sy;

	 	// Work out the position and size on the display
	 	display_ratio = (float)display_width/(float)display_height;
	 	game_ratio = (float)width/(float)height;
	 
		display_x = sx = display_adj_width;
		display_y = sy = display_adj_height;

	 	if (game_ratio>display_ratio) 
			sy = (float)display_adj_width/(float)game_ratio;
	 	else 
			sx = (float)display_adj_height*(float)game_ratio;
	 
		// Centre bitmap on screen
	 	display_x = (display_x - sx) / 2;
	 	display_y = (display_y - sy) / 2;
	}


#if 0
	if (options.display_smooth_stretch) 
		vc_dispmanx_rect_set( &src_rect, 0, 0, width << 16, height << 16);
	else
		vc_dispmanx_rect_set( &src_rect, 0, 0, display_adj_width << 16, display_adj_height << 16);

	dispman_display = vc_dispmanx_display_open(0);
	dispman_update = vc_dispmanx_update_start(0);
	dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display,
	  				10, &dst_rect, 0, &src_rect, 
					DISPMANX_PROTECTION_NONE, NULL, NULL, DISPMANX_NO_ROTATE);

	//Black background surface dimensions
	vc_dispmanx_rect_set( &dst_rect, 0, 0, display_width, display_height );
	vc_dispmanx_rect_set( &src_rect, 0, 0, 128 << 16, 128 << 16);
 
	//Create a blank background for the whole screen, make sure width is divisible by 32!
	uint32_t crap;
	resource_bg = vc_dispmanx_resource_create(VC_IMAGE_RGB565, 128, 128, &crap);
	dispman_element_bg = vc_dispmanx_element_add(  dispman_update, dispman_display,
	                                      9, &dst_rect, resource_bg, &src_rect,
	                                      DISPMANX_PROTECTION_NONE, 0, 0,
	                                      (DISPMANX_TRANSFORM_T) 0 );

	nativewindow.element = dispman_element;
	if (options.display_smooth_stretch) {
		nativewindow.width = width;
		nativewindow.height = height;
	}
	else {
		nativewindow.width = display_adj_width;
		nativewindow.height = display_adj_height;
	}

	vc_dispmanx_update_submit_sync(dispman_update);
#endif

	 
	//surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)(go2_context3D->gbmSurface)/*nativewindow*/, NULL);
	//assert(surface != EGL_NO_SURFACE);

	// connect the context to the surface
	result = eglMakeCurrent(display, surface, surface, context);
	assert(EGL_FALSE != result);

	//Smooth stretch the display size for GLES2 is the size of the bitmap
	//otherwise let GLES2 upscale (NEAR) to the size of the display
#if 1
	if (options.display_smooth_stretch) 
		gles2_create(width, height, width, height, bitmap->depth);
	else
		gles2_create(display_adj_width, display_adj_height, width, height, bitmap->depth);
#endif
	update_throttle();
}

extern EGLDisplay display;
extern EGLSurface surface;

void update_throttle(void)
{
    // updated from video.cpp
	extern int throttle;
	static int save_throttle=0;

	if (throttle != save_throttle)
	{
		if(throttle)
			eglSwapInterval(display, 1);
		else
			eglSwapInterval(display, 0);

		save_throttle=throttle;
	}
}

extern volatile unsigned short gp2x_palette[512];

void DisplayScreen(void)
{
	//printf("[trngaje] DisplayScreen++\n");
    //Draw to the screen
  	gles2_draw(rpi_screen, surface_width, surface_height);

    eglSwapBuffers(display, surface);		

	if (ucbpp==8)
	{
		uint8_t* src = (uint8_t *)rpi_screen;
		uint16_t* dst = (uint16_t *)go2_surface_map(go2_surface);
		for (int i=0; i< (surface_width * surface_height); i++)
		{
			*dst = (uint16_t) gp2x_palette[*src];
			dst++;
			src++;
		}
	}
	else{
		uint16_t* src = (uint16_t *)rpi_screen;
		uint16_t* dst = (uint16_t *)go2_surface_map(go2_surface);
		for (int i=0; i< (surface_width * surface_height); i++)
		{
			*dst = (uint16_t) *src;
			dst++;
			src++;
		}		
	}
	
	//go2_context_swap_buffers(go2_context3D);
	//go2_surface_t* gles_surface = go2_context_surface_lock(go2_context3D);
	
	go2_presenter_post(go2_presenter,
				go2_surface/*gles_surface*/, //go2_surface
				0, 0, surface_width, surface_height,
				0, 0, 320, 480,
				GO2_ROTATION_DEGREES_270);
	//go2_context_surface_unlock(go2_context3D, gles_surface);

	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		
	//printf("[trngaje] DisplayScreen--\n");
}


void gp2x_frontend_init(void)
{
	printf("[trngaje] gp2x_frontend_init\n");
	
	go2_display = go2_display_create();
    go2_presenter = go2_presenter_create(go2_display, DRM_FORMAT_XRGB8888, 0xff080808);  // ABGR

	go2_context_attributes_t attr;
	attr.major = 1;
	attr.minor = 0;
	attr.red_bits = 8;
	attr.green_bits = 8;
	attr.blue_bits = 8;
	attr.alpha_bits = 8;
	attr.depth_bits = 24;
	attr.stencil_bits = 0;

	go2_context3D = go2_context_create(go2_display, 640, 480, &attr);
	
	surface = go2_context3D->eglSurface;
	display = go2_context3D->eglDisplay;
	context = go2_context3D->eglContext;
	surface_width = 640;
	surface_height = 480;


	go2_surface = go2_surface_create(go2_display, surface_width, surface_height, DRM_FORMAT_RGB565);
	gp2x_screen15 = (unsigned short *)go2_surface_map(go2_surface);
	gp2x_screen8=(unsigned char *)go2_surface_map(go2_surface);
	
#if 0 	
	int ret;
        
	uint32_t display_width, display_height;
    
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;
    
	surface_width = 640;
	surface_height = 480;
    
	gp2x_screen8=0;
	gp2x_screen15=(unsigned short *) calloc(1, 640*480*2);
	
	graphics_get_display_size(0 /* LCD */, &display_width, &display_height);
    
	dispman_display = vc_dispmanx_display_open( 0 );
	assert( dispman_display != 0 );
        
	// Add border around bitmap for TV
	display_width -= options.display_border * 2;
	display_height -= options.display_border * 2;
    

    
	// setup swapping of double buffers
	cur_res = resource1;
	prev_res = resource0;
 #endif     
}

void gp2x_frontend_deinit(void)
{
	printf("[trngaje] gp2x_frontend_deinit++\n");
	int ret;
#if 0    
   
	if(gp2x_screen8) free(gp2x_screen8);
	if(gp2x_screen15) free(gp2x_screen15);
	gp2x_screen8=0;
	gp2x_screen15=0;
#endif   

	if (go2_context3D != NULL)
	{
		go2_context_destroy(go2_context3D);
		go2_context3D = NULL;
	}
	
	if (go2_presenter != NULL)
	{
		go2_presenter_destroy(go2_presenter);
		go2_presenter = NULL;
	}


	if (go2_display != NULL)
	{
		go2_display_destroy(go2_display);
		go2_display = NULL;
	} 
}

void FE_DisplayScreen(void)
{
	printf("[trngaje] FE_DisplayScreen++\n");
#if 1	
	//go2_surface_t* gles_surface = go2_context_surface_lock(go2_context3D);
	
	go2_presenter_post(go2_presenter,
				go2_surface,
				0, 0, surface_width, surface_height,
				0, 0, 320, 480,
				GO2_ROTATION_DEGREES_270);
	//go2_context_surface_unlock(go2_context3D, gles_surface);	
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		
#endif
	printf("[trngaje] FE_DisplayScreen--\n");	
#if 0
	VC_RECT_T dst_rect;


	// swap current resource
	tmp_res = cur_res;
	cur_res = prev_res;
	prev_res = tmp_res;
#endif
}


static unsigned char fontdata8x8[] =
{
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x3C,0x42,0x99,0xBD,0xBD,0x99,0x42,0x3C,0x3C,0x42,0x81,0x81,0x81,0x81,0x42,0x3C,
	0xFE,0x82,0x8A,0xD2,0xA2,0x82,0xFE,0x00,0xFE,0x82,0x82,0x82,0x82,0x82,0xFE,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x64,0x74,0x7C,0x38,0x00,0x00,
	0x80,0xC0,0xF0,0xFC,0xF0,0xC0,0x80,0x00,0x01,0x03,0x0F,0x3F,0x0F,0x03,0x01,0x00,
	0x18,0x3C,0x7E,0x18,0x7E,0x3C,0x18,0x00,0xEE,0xEE,0xEE,0xCC,0x00,0xCC,0xCC,0x00,
	0x00,0x00,0x30,0x68,0x78,0x30,0x00,0x00,0x00,0x38,0x64,0x74,0x7C,0x38,0x00,0x00,
	0x3C,0x66,0x7A,0x7A,0x7E,0x7E,0x3C,0x00,0x0E,0x3E,0x3A,0x22,0x26,0x6E,0xE4,0x40,
	0x18,0x3C,0x7E,0x3C,0x3C,0x3C,0x3C,0x00,0x3C,0x3C,0x3C,0x3C,0x7E,0x3C,0x18,0x00,
	0x08,0x7C,0x7E,0x7E,0x7C,0x08,0x00,0x00,0x10,0x3E,0x7E,0x7E,0x3E,0x10,0x00,0x00,
	0x58,0x2A,0xDC,0xC8,0xDC,0x2A,0x58,0x00,0x24,0x66,0xFF,0xFF,0x66,0x24,0x00,0x00,
	0x00,0x10,0x10,0x38,0x38,0x7C,0xFE,0x00,0xFE,0x7C,0x38,0x38,0x10,0x10,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1C,0x1C,0x1C,0x18,0x00,0x18,0x18,0x00,
	0x6C,0x6C,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x28,0x7C,0x28,0x7C,0x28,0x00,0x00,
	0x10,0x38,0x60,0x38,0x0C,0x78,0x10,0x00,0x40,0xA4,0x48,0x10,0x24,0x4A,0x04,0x00,
	0x18,0x34,0x18,0x3A,0x6C,0x66,0x3A,0x00,0x18,0x18,0x20,0x00,0x00,0x00,0x00,0x00,
	0x30,0x60,0x60,0x60,0x60,0x60,0x30,0x00,0x0C,0x06,0x06,0x06,0x06,0x06,0x0C,0x00,
	0x10,0x54,0x38,0x7C,0x38,0x54,0x10,0x00,0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00,
	0x00,0x00,0x00,0x00,0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00,0x3E,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x04,0x08,0x10,0x20,0x40,0x00,0x00,
	0x38,0x4C,0xC6,0xC6,0xC6,0x64,0x38,0x00,0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00,
	0x7C,0xC6,0x0E,0x3C,0x78,0xE0,0xFE,0x00,0x7E,0x0C,0x18,0x3C,0x06,0xC6,0x7C,0x00,
	0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x00,0xFC,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00,
	0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00,0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00,
	0x78,0xC4,0xE4,0x78,0x86,0x86,0x7C,0x00,0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00,
	0x00,0x00,0x18,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x18,0x18,0x30,
	0x1C,0x38,0x70,0xE0,0x70,0x38,0x1C,0x00,0x00,0x7C,0x00,0x00,0x7C,0x00,0x00,0x00,
	0x70,0x38,0x1C,0x0E,0x1C,0x38,0x70,0x00,0x7C,0xC6,0xC6,0x1C,0x18,0x00,0x18,0x00,
	0x3C,0x42,0x99,0xA1,0xA5,0x99,0x42,0x3C,0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0x00,
	0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0xFC,0x00,0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00,
	0xF8,0xCC,0xC6,0xC6,0xC6,0xCC,0xF8,0x00,0xFE,0xC0,0xC0,0xFC,0xC0,0xC0,0xFE,0x00,
	0xFE,0xC0,0xC0,0xFC,0xC0,0xC0,0xC0,0x00,0x3E,0x60,0xC0,0xCE,0xC6,0x66,0x3E,0x00,
	0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00,0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00,
	0x06,0x06,0x06,0x06,0xC6,0xC6,0x7C,0x00,0xC6,0xCC,0xD8,0xF0,0xF8,0xDC,0xCE,0x00,
	0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00,0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00,
	0xC6,0xE6,0xF6,0xFE,0xDE,0xCE,0xC6,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,
	0xFC,0xC6,0xC6,0xC6,0xFC,0xC0,0xC0,0x00,0x7C,0xC6,0xC6,0xC6,0xDE,0xCC,0x7A,0x00,
	0xFC,0xC6,0xC6,0xCE,0xF8,0xDC,0xCE,0x00,0x78,0xCC,0xC0,0x7C,0x06,0xC6,0x7C,0x00,
	0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,
	0xC6,0xC6,0xC6,0xEE,0x7C,0x38,0x10,0x00,0xC6,0xC6,0xD6,0xFE,0xFE,0xEE,0xC6,0x00,
	0xC6,0xEE,0x3C,0x38,0x7C,0xEE,0xC6,0x00,0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00,
	0xFE,0x0E,0x1C,0x38,0x70,0xE0,0xFE,0x00,0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00,
	0x60,0x60,0x30,0x18,0x0C,0x06,0x06,0x00,0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00,
	0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
	0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x06,0x3E,0x66,0x66,0x3C,0x00,
	0x60,0x7C,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x3C,0x66,0x60,0x60,0x66,0x3C,0x00,
	0x06,0x3E,0x66,0x66,0x66,0x66,0x3E,0x00,0x00,0x3C,0x66,0x66,0x7E,0x60,0x3C,0x00,
	0x1C,0x30,0x78,0x30,0x30,0x30,0x30,0x00,0x00,0x3E,0x66,0x66,0x66,0x3E,0x06,0x3C,
	0x60,0x7C,0x76,0x66,0x66,0x66,0x66,0x00,0x18,0x00,0x38,0x18,0x18,0x18,0x18,0x00,
	0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0x0C,0x38,0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00,
	0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0xEC,0xFE,0xFE,0xFE,0xD6,0xC6,0x00,
	0x00,0x7C,0x76,0x66,0x66,0x66,0x66,0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x3C,0x00,
	0x00,0x7C,0x66,0x66,0x66,0x7C,0x60,0x60,0x00,0x3E,0x66,0x66,0x66,0x3E,0x06,0x06,
	0x00,0x7E,0x70,0x60,0x60,0x60,0x60,0x00,0x00,0x3C,0x60,0x3C,0x06,0x66,0x3C,0x00,
	0x30,0x78,0x30,0x30,0x30,0x30,0x1C,0x00,0x00,0x66,0x66,0x66,0x66,0x6E,0x3E,0x00,
	0x00,0x66,0x66,0x66,0x66,0x3C,0x18,0x00,0x00,0xC6,0xD6,0xFE,0xFE,0x7C,0x6C,0x00,
	0x00,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x06,0x3C,
	0x00,0x7E,0x0C,0x18,0x30,0x60,0x7E,0x00,0x0E,0x18,0x0C,0x38,0x0C,0x18,0x0E,0x00,
	0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00,0x70,0x18,0x30,0x1C,0x30,0x18,0x70,0x00,
	0x00,0x00,0x76,0xDC,0x00,0x00,0x00,0x00,0x10,0x28,0x10,0x54,0xAA,0x44,0x00,0x00,
};

static void gp2x_text(unsigned short *screen, int x, int y, char *text, int color)
{
	unsigned int i,l;
	screen=screen+(x*2)+(y*2)*640;

	for (i=0;i<strlen(text);i++) {
		
		for (l=0;l<16;l=l+2) {
			screen[l*640+0]=(fontdata8x8[((text[i])*8)+l/2]&0x80)?color:screen[l*640+0];
			screen[l*640+1]=(fontdata8x8[((text[i])*8)+l/2]&0x80)?color:screen[l*640+1];

			screen[l*640+2]=(fontdata8x8[((text[i])*8)+l/2]&0x40)?color:screen[l*640+2];
			screen[l*640+3]=(fontdata8x8[((text[i])*8)+l/2]&0x40)?color:screen[l*640+3];

			screen[l*640+4]=(fontdata8x8[((text[i])*8)+l/2]&0x20)?color:screen[l*640+4];
			screen[l*640+5]=(fontdata8x8[((text[i])*8)+l/2]&0x20)?color:screen[l*640+5];

			screen[l*640+6]=(fontdata8x8[((text[i])*8)+l/2]&0x10)?color:screen[l*640+6];
			screen[l*640+7]=(fontdata8x8[((text[i])*8)+l/2]&0x10)?color:screen[l*640+7];

			screen[l*640+8]=(fontdata8x8[((text[i])*8)+l/2]&0x08)?color:screen[l*640+8];
			screen[l*640+9]=(fontdata8x8[((text[i])*8)+l/2]&0x08)?color:screen[l*640+9];

			screen[l*640+10]=(fontdata8x8[((text[i])*8)+l/2]&0x04)?color:screen[l*640+10];
			screen[l*640+11]=(fontdata8x8[((text[i])*8)+l/2]&0x04)?color:screen[l*640+11];

			screen[l*640+12]=(fontdata8x8[((text[i])*8)+l/2]&0x02)?color:screen[l*640+12];
			screen[l*640+13]=(fontdata8x8[((text[i])*8)+l/2]&0x02)?color:screen[l*640+13];

			screen[l*640+14]=(fontdata8x8[((text[i])*8)+l/2]&0x01)?color:screen[l*640+14];
			screen[l*640+15]=(fontdata8x8[((text[i])*8)+l/2]&0x01)?color:screen[l*640+15];
		}
		for (l=1;l<16;l=l+2) {
			screen[l*640+0]=(fontdata8x8[((text[i])*8)+l/2]&0x80)?color:screen[l*640+0];
			screen[l*640+1]=(fontdata8x8[((text[i])*8)+l/2]&0x80)?color:screen[l*640+1];

			screen[l*640+2]=(fontdata8x8[((text[i])*8)+l/2]&0x40)?color:screen[l*640+2];
			screen[l*640+3]=(fontdata8x8[((text[i])*8)+l/2]&0x40)?color:screen[l*640+3];

			screen[l*640+4]=(fontdata8x8[((text[i])*8)+l/2]&0x20)?color:screen[l*640+4];
			screen[l*640+5]=(fontdata8x8[((text[i])*8)+l/2]&0x20)?color:screen[l*640+5];

			screen[l*640+6]=(fontdata8x8[((text[i])*8)+l/2]&0x10)?color:screen[l*640+6];
			screen[l*640+7]=(fontdata8x8[((text[i])*8)+l/2]&0x10)?color:screen[l*640+7];

			screen[l*640+8]=(fontdata8x8[((text[i])*8)+l/2]&0x08)?color:screen[l*640+8];
			screen[l*640+9]=(fontdata8x8[((text[i])*8)+l/2]&0x08)?color:screen[l*640+9];

			screen[l*640+10]=(fontdata8x8[((text[i])*8)+l/2]&0x04)?color:screen[l*640+10];
			screen[l*640+11]=(fontdata8x8[((text[i])*8)+l/2]&0x04)?color:screen[l*640+11];

			screen[l*640+12]=(fontdata8x8[((text[i])*8)+l/2]&0x02)?color:screen[l*640+12];
			screen[l*640+13]=(fontdata8x8[((text[i])*8)+l/2]&0x02)?color:screen[l*640+13];

			screen[l*640+14]=(fontdata8x8[((text[i])*8)+l/2]&0x01)?color:screen[l*640+14];
			screen[l*640+15]=(fontdata8x8[((text[i])*8)+l/2]&0x01)?color:screen[l*640+15];
		}
		screen+=16;
	} 
}

void gp2x_gamelist_text_out(int x, int y, char *eltexto, int color)
{
	char texto[33];
	strncpy(texto,eltexto,32);
	texto[32]=0;
	gp2x_text(gp2x_screen15,x,y,texto,color);
}

void gp2x_gamelist_text_out_fmt(int x, int y, char* fmt, ...)
{
	char strOut[128];
	va_list marker;
	
	va_start(marker, fmt);
	vsprintf(strOut, fmt, marker);
	va_end(marker);	

	gp2x_gamelist_text_out(x, y, strOut, 255);
}

static int pflog=0;

void gp2x_printf_init(void)
{
	pflog=0;
}

#define gp2x_color15(R,G,B)  ((R >> 3) << 11) | (( G >> 2) << 5 ) | (( B >> 3 ) << 0 )

void gp2x_text_log(char *texto)
{
    if (!pflog)
    {
        memset(gp2x_screen15,0,320*240*2);
    }
    gp2x_text(gp2x_screen15,0,pflog,texto,gp2x_color15(255,255,255));
    pflog+=8;
    if(pflog>239) pflog=0;
}

/* Variadic functions guide found at http://www.unixpapa.com/incnote/variadic.html */
void gp2x_printf(char* fmt, ...)
{
	int i,c;
	char strOut[4096];
	char str[41];
	va_list marker;

	va_start(marker, fmt);
	vsprintf(strOut, fmt, marker);
	va_end(marker);

	//printf("[trngaje] gp2x_printf++\n");
	gp2x_frontend_init();

	c=0;
	for (i=0;i<strlen(strOut);i++)
	{
	    str[c]=strOut[i];
	    if (str[c]=='\n')
	    {
	        str[c]=0;
	        gp2x_text_log(str);
	        c=0;
	    }
	    else if (c==39)
	    {
	        str[40]=0;
	        gp2x_text_log(str);
	        c=0;
	    }
	    else
	    {
	        c++;
	    }
	}

	FE_DisplayScreen();
	sleep(6);	
	gp2x_frontend_deinit();

	pflog=0;
	//printf("[trngaje] gp2x_printf--\n");
}
