/**************************************************************************\
* IMGCAT - a simple (single) image viewer 
*
* Author: Jesse McClure, copyright 2013
* License: GPLv3
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
\**************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <Imlib2.h>

static void configurenotify(XEvent *);
static void expose(XEvent *);
static void keypress(XEvent *);
static int command_line(int, const char **);
static int draw();

static Display *dpy;
static Window win = 0, root;
static Visual *vis;
static Colormap cmap;
static int scr, iw, ih, ww, wh, mw = 640, mh = 480, sw, sh, wx, wy;
static double scale = 1.0;
static Bool running = True, fullscreen = False;
static Imlib_Image *img = NULL;
static XClassHint class;
static void (*handler[LASTEvent])(XEvent *) = {
	[ConfigureNotify]	= configurenotify,
	[Expose]			= expose,
	[KeyPress]			= keypress,
};

void configurenotify(XEvent *e) {
	unsigned int ui;
	XGetGeometry(dpy,win,&root,&wx,&wy,&ww,&wh,&ui,&ui);
}

void expose(XEvent *e) {
	draw();
}

void keypress(XEvent *e) {
	int t;
	KeySym key = XkbKeycodeToKeysym(dpy,e->xkey.keycode,0,0);
	if (key == XK_q) running = False;
	else if (key == XK_Up || key == XK_equal || key == XK_k) {
		t = ww; wx -= ((ww*=1.1) - t)/2; t = wh; wy -= ((wh*=1.1) - t)/2;
	}
	else if (key == XK_Down || key == XK_minus || key == XK_j) {
		t = ww; wx += (t - (ww/=1.1))/2; t = wh; wy += (t - (wh/=1.1))/2;
	}
	else if (key == XK_Right || key == XK_l) {
		wx-=5; wy+=5; ww+=10; wh-=10;
	}
	else if (key == XK_Left || key == XK_h) {
		wx+=5; wy-=5; ww-=10; wh+=10;
	}
	else if (key == XK_r) {
		ww = iw; wh = ih; 
		wx = ( sw > ww ? (sw-ww)/2 : 0); wy = ( sh > wh ? (sh-wh)/2 : 0);
	}
	else if (key == XK_s || (key == XK_f && (fullscreen=!fullscreen))) {
		scale = ( sw/(float)iw > sh/(float)ih ? sh/(float)ih : sw/(float)iw );
		ww = iw * scale + 0.5; wh = ih * scale + 0.5;
		wx = (sw-ww)/2; wy = (sh-wh)/2;
	}
	else {
		return;
	}
	if (fullscreen) XMoveResizeWindow(dpy,win,0,0,sw,sh);
	else XMoveResizeWindow(dpy,win,wx,wy,ww,wh);
	XFlush(dpy);
	draw();
}

int command_line(int argc, const char **argv) {
	if ( !(dpy=XOpenDisplay(0x0)) ) exit(1);
	scr = DefaultScreen(dpy); root = DefaultRootWindow(dpy);
	sw = DisplayWidth(dpy,scr); sh = DisplayHeight(dpy,scr);
	int i; const char *c;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') break;
		if ( *(c=argv[i]+1) == '-' ) c++;
		if (*c == '\0') continue;
		if (*c == 'b') win = root;
	}
	if ( !(img=imlib_load_image(argv[i])) ) exit(2);;
	imlib_context_set_image(img);
	imlib_context_set_display(dpy);
	imlib_context_set_visual(DefaultVisual(dpy,scr));
	imlib_context_set_colormap(DefaultColormap(dpy,scr));
	iw = imlib_image_get_width();
	ih = imlib_image_get_height();
	imlib_context_set_anti_alias(0);
	imlib_context_set_dither(0);
	imlib_context_set_blend(0);
	XSetWindowAttributes wa;
	if (win == root) {
		Pixmap buf = XCreatePixmap(dpy,root,sw,sh,DefaultDepth(dpy,scr));
		imlib_context_set_drawable(buf);
		imlib_render_image_on_drawable_at_size(0, 0, sw, sh);
		wa.background_pixmap = buf;
		XSetWindowBackgroundPixmap(dpy,root,buf);
		XFlush(dpy);
		XFreePixmap(dpy,buf);
		XCloseDisplay(dpy);
		exit(0);
	}
	if (iw > mw) scale = mw / (float) iw;
	if (ih * scale > mh) scale = mh / (float) ih;
	ww = iw * scale + 0.5; wh = ih * scale + 0.5;
	wa.background_pixel = BlackPixel(dpy,scr);
	wa.event_mask = KeyPressMask | ExposureMask | StructureNotifyMask;
	win = XCreateWindow(dpy,root,0,0,ww,wh,0,DefaultDepth(dpy,scr),
			InputOutput, DefaultVisual(dpy,scr),CWEventMask|CWBackPixel,&wa);
	const char *name;
	if ( (name=strrchr(argv[i],'/')) ) name++;
	else name = argv[i];
	XStoreName(dpy,win,name);
	class.res_name = (char *) argv[0];
	class.res_class = "float";
	XSetClassHint(dpy,win,&class);
	XMapWindow(dpy,win); XFlush(dpy);
	imlib_context_set_drawable(win);
	return 0;
}

int draw() {
	XClearWindow(dpy,win);
	if (fullscreen) imlib_render_image_on_drawable_at_size(wx, wy, ww, wh);
	else imlib_render_image_on_drawable_at_size(0, 0, ww, wh);
	XSync(dpy,True);
}

int main(int argc, const char **argv) {
	command_line(argc,argv);
	draw();
	unsigned int ui;
	XGetGeometry(dpy,win,&root,&wx,&wy,&ww,&wh,&ui,&ui);
	XEvent ev;
	while ( running && !XNextEvent(dpy,&ev) )
		if (handler[ev.type]) handler[ev.type](&ev);
	XDestroyWindow(dpy,win);
	XCloseDisplay(dpy);
	return 0;
}
