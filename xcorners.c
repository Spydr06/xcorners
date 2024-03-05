#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>

#include <X11/extensions/shapeconst.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include <getopt.h>

#define X_CLASS_NAME "xcorners"
#define DEFAULT_RADIUS 12

#define _(str) str
#define PANIC(...) do {                 \
        fprintf(stderr, __VA_ARGS__);   \
        exit(EXIT_FAILURE);             \
    } while(0)

#define UNSIGNED_OPT(c, v) do {                                                                         \
        (v) = (unsigned int) strtoul(optarg, NULL, 10);                                                 \
        if(errno)                                                                                       \
            PANIC("%s: option -- '%c' expects numeric argument: %s\n", *argv, (c), strerror(errno));    \
    } while(0)

unsigned width = 0, height = 0, radius = DEFAULT_RADIUS, x_offset = 0, y_offset = 0;
bool top = true, bottom = false;

Display* d = NULL;

static const struct option cmdline_options[] = {
    {"help", 0, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

static void destroy_display(void) {
    if(d)
        XCloseDisplay(d);
}

static __attribute__((noreturn)) void help(const char* pname) {
    printf(_("Usage: %s [OPTIONS]\n\
\n\
Options:\n\
  -W <width>    Set the horizontal space between the corners. [%u]\n\
  -H <height>   Set the vertical space between the corners. [%u]\n\
  -x <offset>   Set the horizontal offset. [%u]\n\
  -y <offset>   Set the vertical offset. [%u]\n\
  -r <radius>   Set the corner radius. [%u]\n\
  -t, -T        Enable or disable top corners. [%s]\n\
  -b, -B        Enable or disable bottom corners. [%s]\n\
  -h, --help    Print this help text and exit.\n\
"), 
        pname, width, height, x_offset, y_offset, radius,
        top ? "enabled" : "disabled", bottom ? "enabled" : "disabled"
    );

    exit(EXIT_SUCCESS); 
}

static void draw(cairo_t* c) {
    if(top) {
        cairo_move_to(c, 0, 0); // top left
        cairo_arc(c, radius, radius, radius, -M_PI, -M_PI / 2);
        cairo_line_to(c, 0, 0);
        cairo_line_to(c, 0, radius);

        cairo_move_to(c, width, 0); // top right
        cairo_arc(c, width - radius, radius, radius, -M_PI / 2, 0);
        cairo_line_to(c, width, 0);
        cairo_line_to(c, width - radius, 0);
    }

    if(bottom) {
        cairo_move_to(c, 0, height); // bottom left
        cairo_arc(c, radius, height - radius, radius, M_PI / 2, M_PI);
        cairo_line_to(c, 0, height);
        cairo_line_to(c, 0, height - radius);

        // bottom right
        cairo_move_to(c, width, height);
        cairo_arc(c, width - radius, height - radius, radius, 0, M_PI / 2);
        cairo_line_to(c, width, height);
        cairo_line_to(c, width - radius, height);
    }

    cairo_set_source_rgba(c, 0.0, 0.0, 0.0, 1.0);
    cairo_fill(c);
}

int main(int argc, char** argv) {
    d = XOpenDisplay(NULL);
    if(!d)
        PANIC("X11 Error: %s\n", strerror(errno));
    atexit(destroy_display);

    int s = DefaultScreen(d);
    errno = 0;

    width = DisplayWidth(d, s);
    height = DisplayHeight(d, s);

    int ch;
    while((ch = getopt_long(argc, argv, "hx:y:W:H:r:tTbB", cmdline_options, NULL)) != EOF) {
        switch(ch)
        {
            case 'h':
                help(*argv);
            case 'x':
                UNSIGNED_OPT('x', x_offset);
                break;
            case 'y':
                UNSIGNED_OPT('y', y_offset);
                break;
            case 'W':
                UNSIGNED_OPT('W', width);
                break;
            case 'H':
                UNSIGNED_OPT('H', height);
                break;
            case 'r':
                UNSIGNED_OPT('r', radius);
                break;
            case 't':
                top = true;
                break;
            case 'T':
                top = false;
                break;
            case 'b':
                bottom = true;
                break;
            case 'B':
                bottom = false;
                break;
            default:
                PANIC("%s: invalid option -- '%c'\nTry `%s --help` for more information.\n", *argv, ch, *argv);
        }
    }

    Window root = RootWindow(d, s);

    XVisualInfo vinfo;
    if(!XMatchVisualInfo(d, s, 32, TrueColor, &vinfo))
        PANIC("%s: no visual found supporting 32 bit color.\n", *argv);

    XSetWindowAttributes attrs;
    attrs.override_redirect = true;
    attrs.colormap = XCreateColormap(d, root, vinfo.visual, AllocNone);
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;

    Window window = XCreateWindow(
        d, root,
        x_offset, y_offset, width, height, 0,
        vinfo.depth, InputOutput,
        vinfo.visual,
        CWOverrideRedirect | CWColormap | CWBackPixel | CWBorderPixel,
        &attrs
    );

    XSetClassHint(d, window, 
        &(XClassHint){.res_name = X_CLASS_NAME, .res_class = X_CLASS_NAME}
    );

    XSelectInput(d, window, ExposureMask | PropertyChangeMask);

    XserverRegion region = XFixesCreateRegion(d, NULL, 0);
    XFixesSetWindowShapeRegion(d, window, ShapeInput, x_offset, y_offset, region);
    XFixesDestroyRegion(d, region);

    XMapWindow(d, window);

    cairo_surface_t* surface = cairo_xlib_surface_create(d, window, vinfo.visual, width, height);
    cairo_t* cr = cairo_create(surface);
    
    Atom wm_state = XInternAtom(d, "_NET_WM_STATE", False),
         fullscreen = XInternAtom(d, "_NET_WM_STATE_FULLSCREEN", False);

    XEvent ev;
    while(1) {
        XNextEvent(d, &ev);
        switch(ev.type) {
            case Expose:
                draw(cr);
                cairo_surface_flush(surface);
                XFlush(d);
                break;
            case PropertyNotify: {
                if(ev.xproperty.atom != wm_state)
                    break;
                 
                int format;
                unsigned long nitems, bytes_after;
                Atom* data = NULL, type;

                //XGetWindowAttributes(d, root, &ev.xproperty.window);
                XGetWindowProperty(d, root, wm_state, 0, 1024, False, AnyPropertyType, &type, &format, &nitems, &bytes_after, (unsigned char**) &data);

                if(!data)
                    break;

                for(unsigned long i = 0; i < nitems; i++) {
                    if(data[i] == fullscreen)
                        XUnmapWindow(d, window);
                }
                XFree(data);
            } break;
            default:
                printf("unhandled event: %d\n", ev.type);
                break;
        }
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    XUnmapWindow(d, window);
    return EXIT_SUCCESS;
}

