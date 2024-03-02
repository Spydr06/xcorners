#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>

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

#define DEFAULT_RADIUS 12

#define _(str) str
#define PANIC(...) do {                 \
        fprintf(stderr, __VA_ARGS__);   \
        exit(EXIT_FAILURE);             \
    } while(0)

#define UNSIGNED_OPT(c, v) do {                                                                     \
        (v) = (unsigned int) strtoul(optarg, NULL, 10);                                             \
        if(errno)                                                                                   \
            PANIC("%s: otion -- '%c' expects numeric argument: %s\n", *argv, (c), strerror(errno)); \
    } while(0)

unsigned width = 0, height = 0, radius = DEFAULT_RADIUS, x_offset = 0, y_offset = 0;

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
  -h, --help    Print this help text and exit.\n\
"), 
        pname, width, height, x_offset, y_offset, radius
    );

    exit(EXIT_SUCCESS); 
}

static void draw(cairo_t* c) {
    cairo_arc(c, x_offset + radius, y_offset + radius, radius, -M_PI, -M_PI / 2);
    cairo_line_to(c, x_offset, y_offset);
    cairo_line_to(c, x_offset, y_offset + radius);

    cairo_move_to(c, x_offset + width, y_offset);
    cairo_arc(c, x_offset + width - radius, y_offset + radius, radius, -M_PI / 2, 0);
    cairo_line_to(c, x_offset + width, y_offset);
    cairo_line_to(c, x_offset + width - radius, y_offset);

    cairo_set_source_rgba(c, 0.7, 0.7, 0.7, 1.0);
    cairo_fill_preserve(c);
    
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
    while((ch = getopt_long(argc, argv, "hx:y:W:H:r:", cmdline_options, NULL)) != EOF) {
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
           default:
                PANIC("%s: invalid option -- '%c'\nTry `%s --help` for more information.\n", *argv, ch, *argv);
        }
    }

    Window root = RootWindow(d, s);

    XCompositeRedirectSubwindows(d, root, CompositeRedirectAutomatic);
    XSelectInput(d, root, SubstructureNotifyMask);

    Window overlay = XCompositeGetOverlayWindow(d, root);
    {
        XserverRegion region = XFixesCreateRegion(d, NULL, 0);
        XFixesSetWindowShapeRegion(d, overlay, ShapeBounding, 0, 0, 0);
        XFixesSetWindowShapeRegion(d, overlay, ShapeInput, 0, 0, region);
        XFixesDestroyRegion(d, region);
    }

    cairo_surface_t* surface = cairo_xlib_surface_create(d, overlay, DefaultVisual(d, s), width, radius + y_offset);
    cairo_t* cr = cairo_create(surface);

    cairo_surface_t* offscreen_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, radius + y_offset);
    cairo_t* offscreen_cr = cairo_create(offscreen_surface);
    draw(offscreen_cr); // corners don't change, so this is fine
    cairo_set_source_surface(cr, offscreen_surface, 0, 0);
   
    unsigned refresh_rate = 60;
    struct timespec ts = {0, (unsigned long)(1.0L/((long double) refresh_rate) * 1000000000.0L)};
    while(1) {
        nanosleep(&ts, NULL);
        XSync(d, False);
        cairo_paint(cr);
        XFlush(d);
    }

    cairo_destroy(cr);
    cairo_destroy(offscreen_cr);
    cairo_surface_destroy(surface);
    cairo_surface_destroy(offscreen_surface);
    return EXIT_SUCCESS;
}

