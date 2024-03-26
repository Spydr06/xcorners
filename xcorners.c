#include <X11/Xutil.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <getopt.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>

#include <cairo.h>
#include <cairo-xlib.h>

#define X_CLASS_NAME "xcorners"
#define DEFAULT_RADIUS 12

#define _(str) str
#define PANIC(...) do {                 \
        fprintf(stderr, __VA_ARGS__);   \
        exit(EXIT_FAILURE);             \
    } while(0)

#define UNSIGNED_OPT(c, v, base) do {                                                                   \
        (v) = (unsigned int) strtoul(optarg, NULL, (base));                                             \
        if(errno)                                                                                       \
            PANIC("%s: option -- '%c' expects numeric argument: %s\n", *argv, (c), strerror(errno));    \
    } while(0)

unsigned width = 0, height = 0, radius = DEFAULT_RADIUS, x_offset = 0, y_offset = 0, color = 0x000000ff;
bool top = true, bottom = false;

static const struct option cmdline_options[] = {
    {"help", 0, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

__attribute__((noreturn))
static void help(const char* pname) {
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
  -c <color>    Set the corner color. [%08x]\n\
  -1            Only allow one instance.\n\
  -h, --help    Print this help text and exit.\n\
"), 
        pname, width, height, x_offset, y_offset, radius,
        top ? "enabled" : "disabled", bottom ? "enabled" : "disabled",
        color
    );

    exit(EXIT_SUCCESS); 
}

static void check_other_instances(Display* d, Window root)
{
    Window root_return, parent_return, *children;
    unsigned num_children;
    if(!XQueryTree(d, root, &root_return, &parent_return, &children, &num_children))
        return;

    for(unsigned i = 0; i < num_children; i++) {
        XClassHint class_hint;
        if(!XGetClassHint(d, children[i], &class_hint))
            continue;

        if(strcmp(class_hint.res_class, X_CLASS_NAME) == 0) {
            XCloseDisplay(d);
            printf("Another instance is already running, terminating as indicated by `-1`.\n");
            exit(0);
        }

        XFree(class_hint.res_name);
        XFree(class_hint.res_class);
    }

    XFree(children);
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

        cairo_move_to(c, width, height); // bottom right
        cairo_arc(c, width - radius, height - radius, radius, 0, M_PI / 2);
        cairo_line_to(c, width, height);
        cairo_line_to(c, width - radius, height);
    }

    cairo_set_source_rgba(c,
        (double)((color >> 24) & 0xff) / 255.0,
        (double)((color >> 16) & 0xff) / 255.0,
        (double)((color >> 8) & 0xff) / 255.0,
        (double)(color & 0xff) / 255.0
    );
    cairo_fill(c);
}

int main(int argc, char** argv) {
    Display* d = XOpenDisplay(NULL);
    if(!d)
        PANIC("X11 Error: %s\n", strerror(errno));

    int s = DefaultScreen(d);
    Window root = RootWindow(d, s); 

    width = DisplayWidth(d, s);
    height = DisplayHeight(d, s);

    errno = 0;
    int ch;
    while((ch = getopt_long(argc, argv, "hx:y:W:H:r:tTbBc:1", cmdline_options, NULL)) != EOF) {
        switch(ch)
        {
            case 'h':
                help(*argv);
            case 'x':
                UNSIGNED_OPT('x', x_offset, 10);
                break;
            case 'y':
                UNSIGNED_OPT('y', y_offset, 10);
                break;
            case 'W':
                UNSIGNED_OPT('W', width, 10);
                break;
            case 'H':
                UNSIGNED_OPT('H', height, 10);
                break;
            case 'r':
                UNSIGNED_OPT('r', radius, 10);
                break;
            case 'c':
                UNSIGNED_OPT('c', color, 16);
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
            case '1':
                check_other_instances(d, root);
                break;
            default:
                PANIC("%s: invalid option -- '%c'\nTry `%s --help` for more information.\n", *argv, ch, *argv);
        }
    }

    if(optind < argc)
        PANIC("%s: invalid option -- '%s'\nTry `%s --help` for more information.\n", *argv, argv[optind], *argv);


    XVisualInfo vinfo;
    if(!XMatchVisualInfo(d, s, 32, TrueColor, &vinfo))
        PANIC("%s: no visual found supporting 32 bit color.\n", *argv);

    XSetWindowAttributes attrs = {
        .override_redirect = true,
        .colormap = XCreateColormap(d, root, vinfo.visual, AllocNone),
        .background_pixel = 0,
        .border_pixel = 0
    };

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

    XLowerWindow(d, window);

    cairo_surface_t* surface = cairo_xlib_surface_create(d, window, vinfo.visual, width, height);
    cairo_t* cr = cairo_create(surface);
    
    XEvent ev;
    while(1) {
        XNextEvent(d, &ev);
        switch(ev.type) {
            case Expose:
                draw(cr);
                cairo_surface_flush(surface);
                XFlush(d);
                break;
            default:
#ifndef NDEBUG
                printf("unhandled event: %d\n", ev.type);
#endif
                break;
        }
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    XUnmapWindow(d, window);
    XCloseDisplay(d);

    return EXIT_SUCCESS;
}

