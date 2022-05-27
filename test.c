#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
Display *display;
    display = XOpenDisplay(NULL);
if (!display) {fprintf(stderr, "unable to connect to display");return 7;}
    Window w;
    int x,y,i;
    unsigned m;
    Window root = XDefaultRootWindow(display);
if (!root) {fprintf(stderr, "unable to open rootwindow");return 8;}
    //sleep(1);
    if(!XQueryPointer(display,root,&root,&w,&x,&y,&i,&i,&m))
{  printf("unable to query pointer\n"); return 9;}
    XImage *image;
    XWindowAttributes attr;
    XGetWindowAttributes(display, root, &attr);
    image = XGetImage(display,root,0,0,attr.width,attr.height,AllPlanes,XYPixmap);
    XCloseDisplay(display);
    for(int i=0;i!=100;i++){
	printf("%x\n", XGetPixel(image, i, i));
    }
if (!image) {printf("unable to get image\n"); return 10;}
}
