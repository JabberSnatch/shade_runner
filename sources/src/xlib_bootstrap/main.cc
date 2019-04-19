#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <GL/gl.h>
#include <GL/glx.h>

using proc_glXCreateContextAttribsARB =
    GLXContext(*)(Display*, GLXFBConfig, GLXContext, Bool, int const*);

static const int kVisualAttributes[] = {
    GLX_X_RENDERABLE, True,
    GLX_DOUBLEBUFFER, True,
#if 0
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
    GLX_DEPTH_SIZE, 24,
#endif
    None
};
static const int kGLContextAttributes[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
    GLX_CONTEXT_MINOR_VERSION_ARB, 5,
    None
};

int main()
{
    Display* const display = XOpenDisplay(nullptr);
    if (!display)
    {
        std::cerr << "Can't connect to X server" << std::endl;
        return 1;
    }
    std::cout << "Connected to X server" << std::endl;


    int glx_major = -1, glx_minor = -1;
    if (!glXQueryVersion(display, &glx_major, &glx_minor))
    {
        std::cerr << "glXQueryVersion failed." << std::endl;
        return 1;
    }
    if (!(glx_major > 1 || (glx_major == 1 && glx_minor >= 3)))
    {
        std::cerr << "GLX 1.3 is required." << std::endl;
        return 1;
    }
    std::cout << "GLX " << glx_major << "." << glx_minor << " was found." << std::endl;


    GLXFBConfig selected_config{};
    {
        int fb_count = 0;
        GLXFBConfig* const fb_configs = glXChooseFBConfig(display, DefaultScreen(display),
                                                          kVisualAttributes, &fb_count);
        if (!fb_configs)
        {
            std::cerr << "No framebuffer configuration found." << std::endl;
            return 1;
        }
        std::cout << fb_count << " configs found" << std::endl;
        selected_config = fb_configs[fb_count - 1];
        XFree(fb_configs);
    }

    XVisualInfo* const visual_info = glXGetVisualFromFBConfig(display, selected_config);

    XSetWindowAttributes swa;
    swa.colormap = XCreateColormap(display,
                                   RootWindow(display, visual_info->screen),
                                   visual_info->visual, AllocNone);
    swa.border_pixel = 0;
    swa.event_mask = StructureNotifyMask;
    int const swa_mask = CWColormap | CWBorderPixel | CWEventMask;

    Window window = XCreateWindow(display, RootWindow(display, visual_info->screen),
                                  0, 0, 800, 600, 0, visual_info->depth, InputOutput,
                                  visual_info->visual,
                                  swa_mask, &swa);
    if (!window)
    {
        std::cerr << "Window creation failed" << std::endl;
        return 1;
    }
    XFree(visual_info);

    XStoreName(display, window, "x11_bootstrap");
    XMapWindow(display, window);

    auto const glXCreateContextAttribsARB = reinterpret_cast<proc_glXCreateContextAttribsARB>(
        glXGetProcAddressARB((GLubyte const*)"glXCreateContextAttribsARB")
        );
    if (!glXCreateContextAttribsARB)
    {
        std::cerr << "glXCreateContextAttribsARB procedure unavailable" << std::endl;
        return 1;
    }

    GLXContext const glx_context = glXCreateContextAttribsARB(display, selected_config, 0,
                                                              True, kGLContextAttributes);
    XSync(display, False);
    if (!glx_context)
    {
        std::cerr << "Modern GL context creation failed" << std::endl;
        return 1;
    }

    if (!glXIsDirect(display, glx_context))
    {
        std::cout << "Indirect GLX rendering context created" << std::endl;
    }

    glXMakeCurrent(display, window, glx_context);
	std::cout << "GL init complete : " << std::endl;
	std::cout << "OpenGL version : " << glGetString(GL_VERSION) << std::endl;
	std::cout << "Manufacturer : " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Drivers : " << glGetString(GL_RENDERER) << std::endl;
	{
		std::cout << "Context flags : ";
		int context_flags = 0;
		glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
		if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT)
			std::cout << "debug, ";
		std::cout << std::endl;
	}


    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glXSwapBuffers(display, window);

    glXMakeCurrent(display, 0, 0);

    std::cout << "Hello GL !" << std::endl;
    int i = 0;
    std::cin >> i;

    glXDestroyContext(display, glx_context);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}
