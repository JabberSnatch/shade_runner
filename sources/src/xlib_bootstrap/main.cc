/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include <iostream>
#include <memory>

#include <chrono>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include "oglbase/framebuffer.h"
#include "shaderunner/shaderunner.h"
#include "../shaderunner/gizmo.cc"

using proc_glXCreateContextAttribsARB =
    GLXContext(*)(Display*, GLXFBConfig, GLXContext, Bool, int const*);
using proc_glXSwapIntervalEXT =
    void(*)(Display*, GLXDrawable, int);
using proc_glXSwapIntervalMESA =
    int(*)(unsigned);

static const int kVisualAttributes[] = {
    GLX_X_RENDERABLE, True,
    GLX_DOUBLEBUFFER, True,
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_SAMPLE_BUFFERS, 1,
    GLX_SAMPLES, 1,
    GLX_DEPTH_SIZE, 24,
    GLX_STENCIL_SIZE, 8,
#if 0
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
#endif
    None
};
static const int kGLContextAttributes[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
    GLX_CONTEXT_MINOR_VERSION_ARB, 5,
    None
};


struct ApplicationHandles
{
    ~ApplicationHandles()
    {
        glXMakeCurrent(display, window, glx_context);
        framebuffer.reset(nullptr);
        sr_context.reset(nullptr);
        gizmo_layer.reset(nullptr);
        glXMakeCurrent(display, 0, 0);

        glXDestroyContext(display, glx_context);
        XDestroyWindow(display, window);
        XCloseDisplay(display);
    };
    Display* display;
    Window window;
    GLXContext glx_context;
    std::unique_ptr<oglbase::Framebuffer> framebuffer;
    std::unique_ptr<sr::RenderContext> sr_context;
    std::unique_ptr<GizmoLayer> gizmo_layer;
};


static constexpr int boot_width = 1280;
static constexpr int boot_height = 720;

std::unique_ptr<oglbase::Framebuffer> framebuffer;
std::unique_ptr<sr::RenderContext> sr_context;
std::unique_ptr<GizmoLayer> gizmo_layer;

int main(int __argc, char* __argv[])
{
    // =========================================================================
    // FLAG(APP_CONTEXT)
    int current_width = boot_width;
    int current_height = boot_height;
    float aspect_ratio = static_cast<float>(current_height) / static_cast<float>(current_width);
    // =========================================================================

    // =========================================================================
    // FLAG(UI_CONTEXT)
    int mouse_x = 0;
    int mouse_y = 0;
    bool mouse_down = false;
    int active_gizmo = 0;

#define PERSPECTIVE(aspect) perspective(0.01f, 1000.f, 3.1415926534f*0.5f, (aspect))
    Matrix_t projection = PERSPECTIVE(aspect_ratio);
    // =========================================================================

    Display * const display = XOpenDisplay(nullptr);
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
        selected_config = fb_configs[0];
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
                                   0, 0, boot_width, boot_height, 0, visual_info->depth, InputOutput,
                                   visual_info->visual,
                                   swa_mask, &swa);
    if (!window)
    {
        std::cerr << "Window creation failed" << std::endl;
        return 1;
    }
    XFree(visual_info);

    static constexpr long kEventMask =
        StructureNotifyMask |
        ButtonPressMask | ButtonReleaseMask |
        PointerMotionMask;
    XSelectInput(display, window, kEventMask);

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

    auto const glXSwapIntervalEXT = reinterpret_cast<proc_glXSwapIntervalEXT>(
        glXGetProcAddressARB((GLubyte const*)"glXSwapIntervalEXT")
        );
    if (!glXSwapIntervalEXT)
    {
        std::cerr << "glXSwapIntervalEXT procedure unavailable" << std::endl;
        return 1;
    }

    auto const glXSwapIntervalMESA = reinterpret_cast<proc_glXSwapIntervalMESA>(
        glXGetProcAddressARB((GLubyte const*)"glXSwapIntervalMESA")
        );

    GLXContext glx_context = glXCreateContextAttribsARB(display, selected_config, 0,
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
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "glew failed to initalize." << std::endl;
    }

    //glXSwapIntervalMESA(1);
    //glXSwapIntervalEXT(display, window, 1);
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

    // =========================================================================
    // FLAG(APP_CONTEXT)
    oglbase::Framebuffer::AttachmentDescs const fbo_attachments{
        { GL_COLOR_ATTACHMENT0, GL_RGBA8 },
        { GL_COLOR_ATTACHMENT1, GL_R32F }
    };

    framebuffer = std::make_unique<oglbase::Framebuffer>(
        current_width, current_height, fbo_attachments, true
    );
    // =========================================================================

    // =========================================================================
    // FLAG(SR_CONTEXT)
    sr_context = std::make_unique<sr::RenderContext>();
    sr_context->SetResolution(current_width, current_height);
    if (__argc > 1)
    {
        sr_context->WatchKernelFile(sr::ShaderStage::kFragment, __argv[1]);
    }
    if (__argc > 3)
    {
        sr_context->WatchKernelFile(sr::ShaderStage::kVertex, __argv[3]);
    }
    if (__argc > 2)
    {
        sr_context->WatchKernelFile(sr::ShaderStage::kGeometry, __argv[2]);
    }
    // =========================================================================

    // =========================================================================
    // FLAG(UI_CONTEXT)
    static Vec3_t const kGizmoColorOff{ 0.f, 1.f, 0.f };
    static Vec3_t const kGizmoColorOn{ 1.f, 0.f, 0.f };
#if 1
    gizmo_layer = std::make_unique<GizmoLayer>(projection);
    for (float i = 0.f; i < 10.f; i+=1.f)
        for (float j = 0.f; j < 10.f; j+=1.f)
            for (float k = 0.f; k < 10.f; k+=1.f)
                gizmo_layer->gizmos_.emplace_back(
                    GizmoDesc{ Vec3_t{ i - 5.f, j - 5.f, -k }, kGizmoColorOff});
#endif
    // =========================================================================

    glXMakeCurrent(display, 0, 0);

    using StdClock = std::chrono::high_resolution_clock;

    glXMakeCurrent(display, window, glx_context);
    for(bool run = true; run;)
    {
        XEvent xevent;
        while (XCheckWindowEvent(display, window, kEventMask, &xevent))
        {
            switch(xevent.type)
            {
            case ConfigureNotify:
            {
                XConfigureEvent const& xcevent = xevent.xconfigure;

                if ((xcevent.width - current_width) * (xcevent.height - current_height))
                {
                // =============================================================
                // FLAG(APP_CONTEXT)
                current_width = xcevent.width; current_height = xcevent.height;
                aspect_ratio =
                    static_cast<float>(current_height) / static_cast<float>(current_width);
                framebuffer.reset(new oglbase::Framebuffer(current_width, current_height,
                                                           fbo_attachments, true));
                // =============================================================
                // =============================================================
                // FLAG(SR_CONTEXT)
                sr_context->SetResolution(current_width, current_height);
                // =============================================================
                // =============================================================
                // FLAG(UI_CONTEXT)
                projection = PERSPECTIVE(aspect_ratio);
#if 1
                gizmo_layer->projection_ = projection;
#endif
                // =============================================================
                }
            } break;
            case ButtonPress:
            {
                //XButtonEvent const& xbevent = xevent.xbutton;
                // =============================================================
                // FLAG(UI_CONTEXT)
                mouse_down = true;
                // =============================================================
            } break;
            case ButtonRelease:
            {
                //XButtonEvent const& xbevent = xevent.xbutton;
                // =============================================================
                // FLAG(UI_CONTEXT)
                mouse_down = false;
#if 1
                if (active_gizmo)
                    gizmo_layer->gizmos_[active_gizmo-1].color_ = kGizmoColorOff;
#endif
                active_gizmo = 0;
                // =============================================================
            } break;
            case MotionNotify:
            {
                XMotionEvent const& xmevent = xevent.xmotion;
                // =============================================================
                // FLAG(UI_CONTEXT)
                mouse_x = xmevent.x; mouse_y = current_height - xmevent.y;
                // =============================================================
            } break;
            case DestroyNotify:
            {
                XDestroyWindowEvent const& xdwevent = xevent.xdestroywindow;
                std::cout << "window destroy" << std::endl;
                run = !(xdwevent.display == display && xdwevent.window == window);
            } break;
            }
        }
        if (!run) break;

        // =====================================================================
        // FLAG(UI_CONTEXT)
#if 1
        if (mouse_down)
        {
            framebuffer->bind();
            glReadBuffer(GL_COLOR_ATTACHMENT1);
            int gizmo_id = 0;
            static_assert(sizeof(int) == sizeof(float), "");
            glReadPixels(mouse_x, mouse_y,
                         1, 1,
                         GL_RED, GL_FLOAT, (float*)&gizmo_id);
            glReadBuffer(GL_NONE);
            framebuffer->unbind();
#if 1
            if (gizmo_id != active_gizmo)
            {
                gizmo_layer->gizmos_[active_gizmo-1].color_ = kGizmoColorOff;
                active_gizmo = gizmo_id;
                if (active_gizmo)
                    gizmo_layer->gizmos_[active_gizmo-1].color_ = kGizmoColorOn;
            }
#endif
        }
#endif
        // =====================================================================


        auto start = StdClock::now();
        framebuffer->bind();

        if (!sr_context->RenderFrame()) break;

#if 1
        gizmo_layer->RenderFrame();
#endif

        framebuffer->unbind();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer->fbo_);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK);
        glBlitFramebuffer(0, 0, current_width, current_height,
                          0, 0, current_width, current_height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glXSwapBuffers(display, window);
        auto end = StdClock::now();
        //std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << std::endl;
    }
    //glXMakeCurrent(display, 0, 0);

#if 0
    glXMakeCurrent(display, window, glx_context);
    framebuffer.reset(nullptr);
    sr_context.reset(nullptr);
    gizmo_layer.reset(nullptr);
    glXMakeCurrent(display, 0, 0);

    glXDestroyContext(display, glx_context);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
#endif
    return 0;
}
