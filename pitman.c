#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <X11/X.h>
#include <unistd.h>
#include <math.h>
#include <complex.h>
#include <limits.h>
#include <time.h>
#define THREADS 8

long double xx=-2,yy=-2,wh=4/512.;
int nx,ny;
unsigned char manor[512][512];
unsigned mxi=200;
Display*dpy;
Window win;

int min(int x,int y){return y+(x-y&x-y>>sizeof(int)*8-1);}
int max(int x,int y){return x-(x-y&x-y>>sizeof(int)*8-1);}
int abs(int x){return (~x>>sizeof(int)*8-1)-(x>>sizeof(int)*8-1);}

void*drawman(void*xv){
	unsigned i=*(unsigned*)xv;
	for(int j=0;j<512;j++){
		complex long double z=xx+wh*i+I*(yy+wh*j),c=z;
		unsigned k=0;
		do{
			z=z*z+c;
			if(__real(z)*__real(z)+__imag(z)*__imag(z)>=4){
				manor[i][j]=~((k<<8)/mxi);
				goto next;
			}
		}while(++k<mxi);
		manor[i][j]=0;
		next:;
	}
}

int main(int argc,char**argv){
	pthread_t a[THREADS];
	dpy=XOpenDisplay(0);
	GLXFBConfig*fbc=glXChooseFBConfig(dpy,DefaultScreen(dpy),0,(int[]){0});
	XVisualInfo*vi=glXChooseVisual(dpy,DefaultScreen(dpy),(int[]){GLX_RGBA,None});
	XSetWindowAttributes swa;
	swa.colormap=XCreateColormap(dpy,RootWindow(dpy,vi->screen),vi->visual,AllocNone);
	swa.border_pixel=0;
	swa.event_mask=StructureNotifyMask|KeyPressMask|ButtonPressMask|ButtonReleaseMask;
	win=XCreateWindow(dpy,RootWindow(dpy,vi->screen),0,0,512,512,0,vi->depth,InputOutput,vi->visual,CWBorderPixel|CWColormap|CWEventMask,&swa);
	XMapWindow(dpy,win);
	GLXContext ctx=glXCreateContext(dpy,vi,0,GL_TRUE);
	XEvent event;
	glXMakeCurrent(dpy,win,ctx);
	glOrtho(0,512,512,0,1,-1);
	pthread_attr_t pat;
	pthread_attr_init(&pat);
	pthread_attr_setstacksize(&pat,PTHREAD_STACK_MIN);
	pthread_attr_setguardsize(&pat,0);
	pthread_attr_setinheritsched(&pat,PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&pat,SCHED_RR);
    pthread_attr_setschedparam(&pat,(struct sched_param[]){{.sched_priority=sched_get_priority_max(SCHED_RR)}});
    pthread_setschedparam(pthread_self(),SCHED_RR,(struct sched_param[]){{.sched_priority=sched_get_priority_min(SCHED_RR)}});
	goto rend;
	for(;;){
		while(XPending(dpy)){
			XNextEvent(dpy,&event);
			switch(event.type){
			 case KeyPress:{
				KeySym keysym;
				char buffer;
				if(XLookupString((XKeyEvent*)&event,&buffer,1,&keysym,NULL)==1&&keysym==XK_Escape)
					return 0;
				break;
			}
			case ButtonPress:
				if(event.xbutton.button==Button1){
					nx=event.xbutton.x;
					ny=event.xbutton.y;
				}else if(event.xbutton.button==Button4) mxi+=25;
				else if(event.xbutton.button==Button5&&mxi>100) mxi-=25;
				else goto rend;
				break;
			case ButtonRelease:
				if(event.xbutton.button==Button1){
					if(event.xbutton.x==nx&&event.xbutton.y==ny){
						xx+=(event.xbutton.x-512)*wh;
						yy+=(event.xbutton.y-512)*wh;
						wh*=2;
					}else{
						xx+=min(event.xbutton.x,nx)*wh;
						yy+=min(event.xbutton.y,ny)*wh;
						wh*=max(abs(event.xbutton.x-nx),abs(event.xbutton.y-ny))/512.;
					}
					rend:;
					clock_t t=clock();
					unsigned n[THREADS],mans=THREADS-1;
					for(int i=0;i<THREADS;i++){
						n[i]=i;
						pthread_create(a+i,&pat,drawman,n+i);
					}
					while(mans<511+THREADS)
						for(int i=0;i<THREADS;i++)
							if(!pthread_tryjoin_np(a[i],0)){
								glBegin(GL_POINTS);
								for(int j=0,k=n[i];j<512;j++){
									glColor3f(manor[k][j]*manor[k][j]*manor[k][j]/16777216.,manor[k][j]*manor[k][j]/65536.,manor[k][j]/256.);
									glVertex2i(k,j);
								}
								glEnd();
								glFlush();
								if(++mans<512){
									n[i]=mans;
									pthread_create(a+i,&pat,drawman,n+i);
								}
							}
					printf("%f\n",(clock()-t)/1000000.);
					fflush(stdout);
				}
			}
		}
	}
	glXDestroyContext(dpy,glXGetCurrentContext());
}
