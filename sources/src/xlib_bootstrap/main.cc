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
#include <cstring>

#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include "oglbase/error.h"
#include "appbase/layer_mediator.h"

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
#ifdef SR_GL_DEBUG_CONTEXT
    GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
#endif
    None
};


#if 0
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
    std::unique_ptr<uibase::GizmoLayer> gizmo_layer;
};
#endif


static constexpr int boot_width = 1280;
static constexpr int boot_height = 720;

std::unique_ptr<appbase::LayerMediator> layer_mediator;

int main(int __argc, char* __argv[])
{
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

    Atom const wm_delete_window = [](Display* display, Window window)
    {
        int wm_protocols_size = 0;
        Atom* wm_protocols = nullptr;
        Status result = XGetWMProtocols(display, window, &wm_protocols, &wm_protocols_size);
        std::cout << "XGetWMProtocols status " << std::to_string(result) << std::endl;
        std::cout << "protocols found " << std::to_string(wm_protocols_size) << std::endl;
        XFree(wm_protocols);

        Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", True);
        if (wm_delete_window == None)
            std::cout << "[ERROR] WM_DELETE_WINDOW doesn't exist" << std::endl;
        result = XSetWMProtocols(display, window, &wm_delete_window, 1);
        std::cout << "XSetWMProtocols status " << std::to_string(result) << std::endl;

        result = XGetWMProtocols(display, window, &wm_protocols, &wm_protocols_size);
        std::cout << "XGetWMProtocols status " << std::to_string(result) << std::endl;
        std::cout << "protocols found " << std::to_string(wm_protocols_size) << std::endl;
        XFree(wm_protocols);

        int property_count = 0;
        Atom* x_properties = XListProperties(display, window, &property_count);
        std::cout << "properties found " << std::to_string(property_count) << std::endl;
        XFree(x_properties);

        return wm_delete_window;
    }(display, window);

    static constexpr long kEventMask =
        StructureNotifyMask
        | ButtonPressMask | ButtonReleaseMask
        | PointerMotionMask
        | KeyPressMask | KeyReleaseMask;
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

#ifdef SR_GL_DEBUG_CONTEXT
    oglbase::DebugMessageControl<> debugMessageControl{};
#endif

    layer_mediator = std::make_unique<appbase::LayerMediator>(
        appbase::Vec2i_t{ boot_width, boot_height },
        appbase::LayerFlag::kShaderunner
        // | appbase::LayerFlag::kGizmo
        | appbase::LayerFlag::kImgui
    );

    layer_mediator->SpecialKey(appbase::eKey::kTab, (std::uint8_t)(XK_Tab & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kLeft, (std::uint8_t)(XK_Left & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kRight, (std::uint8_t)(XK_Right & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kUp, (std::uint8_t)(XK_Up & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kDown, (std::uint8_t)(XK_Down & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kPageUp, (std::uint8_t)(XK_Page_Up & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kPageDown, (std::uint8_t)(XK_Page_Down & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kHome, (std::uint8_t)(XK_Home & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kEnd, (std::uint8_t)(XK_End & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kInsert, (std::uint8_t)(XK_Insert & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kDelete, (std::uint8_t)(XK_Delete & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kBackspace, (std::uint8_t)(XK_BackSpace & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kEnter, (std::uint8_t)(XK_Return & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kEscape, (std::uint8_t)(XK_Escape & 0xff));

    if (__argc > 1)
    {
        layer_mediator->sr_layer_->WatchKernelFile(sr::ShaderStage::kFragment, __argv[1]);
    }
    if (__argc > 3)
    {
        layer_mediator->sr_layer_->WatchKernelFile(sr::ShaderStage::kVertex, __argv[3]);
    }
    if (__argc > 2)
    {
        layer_mediator->sr_layer_->WatchKernelFile(sr::ShaderStage::kGeometry, __argv[2]);
    }


    glXMakeCurrent(display, 0, 0);

    using StdClock = std::chrono::high_resolution_clock;

    glXMakeCurrent(display, window, glx_context);
    int i = 0;
    float frame_time = 0.f;
    for(bool run = true; run;)
    {
        auto start = StdClock::now();

        XEvent xevent;
        while (XCheckWindowEvent(display, window, kEventMask, &xevent))
        {
            switch(xevent.type)
            {
            case ConfigureNotify:
            {
                XConfigureEvent const& xcevent = xevent.xconfigure;
                layer_mediator->ResizeEvent({ xcevent.width, xcevent.height });
            } break;

            case ButtonPress:
            case ButtonRelease:
            {
                layer_mediator->MouseDown(xevent.type == ButtonPress);
                //XButtonEvent const& xbevent = xevent.xbutton;
            } break;

            case KeyPress:
            case KeyRelease:
            {
                XKeyEvent& xkevent = xevent.xkey;
                {
                    unsigned mod_mask = 0;
                    {
                        Window a, b; int c, d, e, f;
                        XQueryPointer(display, window, &a, &b, &c, &d, &e, &f, &mod_mask);
                    }

                    std::uint32_t km = 0u;
                    km |= (mod_mask & ControlMask) ? appbase::fKeyMod::kCtrl : 0u;
                    km |= (mod_mask & ShiftMask) ? appbase::fKeyMod::kShift : 0u;
                    km |= (mod_mask & Mod1Mask) ? appbase::fKeyMod::kAlt : 0u;

                    KeySym ks = XLookupKeysym(&xkevent, xkevent.state);
                    if (ks == NoSymbol)
                        ks = XLookupKeysym(&xkevent, xkevent.state & ShiftMask);
                    if (ks == NoSymbol)
                        ks = XLookupKeysym(&xkevent, 0);

                    layer_mediator->KeyDown((std::uint32_t)ks, km, (xevent.type == KeyPress));
                }
            } break;

            case MotionNotify:
            {
                XMotionEvent const& xmevent = xevent.xmotion;
                layer_mediator->MousePos({ xmevent.x, layer_mediator->state_.screen_size[1] - xmevent.y });
            } break;

            case DestroyNotify:
            {
                XDestroyWindowEvent const& xdwevent = xevent.xdestroywindow;
                std::cout << "window destroy" << std::endl;
                run = !(xdwevent.display == display && xdwevent.window == window);
            } break;
            default: break;
            }
        }

        if (XCheckTypedWindowEvent(display, window, ClientMessage, &xevent))
        {
            std::cout << "Client message" << std::endl;
            std::cout << XGetAtomName(display, xevent.xclient.message_type) << std::endl;
            run = !(xevent.xclient.data.l[0] == wm_delete_window);
        }
        if (!run) break;

        if (!layer_mediator->RunFrame()) break;

        glXSwapBuffers(display, window);

        auto end = StdClock::now();
        frame_time += static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count());
        static int const kFrameInterval = 0xff;
        i = (i + 1) & kFrameInterval;
        if (!i)
        {
            std::cout << "avg frame_time: " << (frame_time / float(kFrameInterval)) << std::endl;
            frame_time = 0.f;
        }
    }
    glXMakeCurrent(display, 0, 0);

    glXMakeCurrent(display, window, glx_context);
    layer_mediator.reset(nullptr);
    glXMakeCurrent(display, 0, 0);

    glXDestroyContext(display, glx_context);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}
