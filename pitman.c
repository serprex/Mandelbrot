#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <unistd.h>
#include <math.h>
#include <complex.h>
#include <limits.h>
#define THREADS 4

long double xx=-2,yy=-2,wh=4/512.;
unsigned char manor[512][512];
unsigned mxi=200;
Display*dpy;
Window win;

void*drawman(void*xv){
	for(int j=0;j<512;j++){
		complex long double z=xx+wh*(xv-(void*)manor>>9)+I*(yy+wh*j),c=z;
		unsigned k=mxi;
		while(--k&&cabsl(z=z*z+c)<2);
		((unsigned char*)xv)[j]=(k<<8)/mxi;
	}
}

int main(int argc,char**argv){
	int nx,ny;
	pthread_t a[THREADS];
	dpy=XOpenDisplay(0);
	XVisualInfo*vi=glXChooseVisual(dpy,DefaultScreen(dpy),(int[]){GLX_RGBA,None});
	win=XCreateWindow(dpy,RootWindow(dpy,vi->screen),0,0,512,512,0,vi->depth,InputOutput,vi->visual,CWBorderPixel|CWColormap|CWEventMask,(XSetWindowAttributes[]){{.colormap=XCreateColormap(dpy,RootWindow(dpy,vi->screen),vi->visual,AllocNone),.border_pixel=0,.event_mask=ExposureMask|KeyPressMask|ButtonPressMask|ButtonReleaseMask}});
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
				if(XLookupString((XKeyEvent*)&event,&buffer,1,&keysym,NULL)==1&&keysym==XK_Escape){
					glXDestroyContext(dpy,glXGetCurrentContext());
					return 0;
				}
			}
			break;case Expose:
				glBegin(GL_POINTS);
				for(int i=0;i<512;i++)
					for(int j=0;j<512;j++){
						glColor3f(manor[i][j]*manor[i][j]*manor[i][j]/16777216.,manor[i][j]*manor[i][j]/65536.,manor[i][j]/256.);
						glVertex2i(i,j);
					}
				glEnd();
				glFlush();
			break;case ButtonPress:
				if(event.xbutton.button==Button1){
					nx=event.xbutton.x;
					ny=event.xbutton.y;
				}else if(event.xbutton.button==Button4) mxi+=25;
				else if(event.xbutton.button==Button5&&mxi>100) mxi-=25;
				else goto rend;
			break;case ButtonRelease:
				if(event.xbutton.button==Button1){
					if(event.xbutton.x==nx&&event.xbutton.y==ny){
						xx+=(event.xbutton.x-512)*wh;
						yy+=(event.xbutton.y-512)*wh;
						wh*=2;
					}else{
						if(event.xbutton.x<nx){
							int t=event.xbutton.x;
							event.xbutton.x=nx;
							nx=t;
						}
						if(event.xbutton.y<ny){
							int t=event.xbutton.y;
							event.xbutton.y=ny;
							ny=t;
						}
						xx+=nx*wh;
						yy+=ny*wh;
						nx=event.xbutton.x-nx;
						ny=event.xbutton.y-ny;
						wh*=(nx-(nx-ny&nx-ny>>sizeof(int)*8-1))/512.;
					}
					rend:
					printf("%Lf\n%Lf\n%Lf\n\n",xx,yy,wh);
					unsigned n[THREADS],mans=THREADS-1;
					for(int i=0;i<THREADS;i++){
						n[i]=i;
						pthread_create(a+i,&pat,drawman,manor+i);
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
									pthread_create(a+i,&pat,drawman,manor+mans);
								}
							}
				}
			}
		}
	}
}
