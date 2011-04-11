#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <GL/glx.h>
#include <math.h>
#include <complex.h>
#define THREADS 4
long double xx=-2,yy=-2,wh=4/512.;
unsigned char manor[512][512];
unsigned mxi=300;
void*drawman(void*xv){
	for(int j=0;j<512;j++){
		complex long double z=xx+wh*(xv-(void*)manor>>9)+I*(yy+wh*j),c=z;
		unsigned k=mxi;
		while(--k&&cabsl(z=z*z+c)<2);
		((unsigned char*)xv)[j]=(k<<8)/mxi;
	}
}
int main(int argc,char**argv){
	int nx,ny,n[THREADS],mans=0;
	pthread_t a[THREADS];
	unsigned char col[256][3];
	for(int i=0;i<256;i++){
		col[i][0]=i*i*i>>16;
		col[i][1]=i*i>>8;
		col[i][2]=i;
	}
	Display*dpy=XOpenDisplay(0);
	XVisualInfo*vi=glXChooseVisual(dpy,DefaultScreen(dpy),(int[]){GLX_RGBA,None});
	Window win=XCreateWindow(dpy,RootWindow(dpy,vi->screen),0,0,511,511,0,vi->depth,InputOutput,vi->visual,CWColormap|CWEventMask,(XSetWindowAttributes[]){{.colormap=XCreateColormap(dpy,RootWindow(dpy,vi->screen),vi->visual,AllocNone),.border_pixel=0,.event_mask=ExposureMask|KeyPressMask|ButtonPressMask|ButtonReleaseMask}});
	XMapWindow(dpy,win);
	glXMakeCurrent(dpy,win,glXCreateContext(dpy,vi,0,GL_TRUE));
	glOrtho(0,511,511,0,1,-1);
	pthread_attr_t pat;
	pthread_attr_init(&pat);
	pthread_attr_setguardsize(&pat,0);
	goto rend;
	for(;;){
		XEvent ev;
		if(XPending(dpy)||!mans){
			XNextEvent(dpy,&ev);
			switch(ev.type){
			case KeyPress:{
				KeySym keysym;
				char buff;
				if(XLookupString((XKeyEvent*)&ev,&buff,1,&keysym,0)==1&&keysym==XK_Escape)return 0;
			}
			break;case Expose:
				glBegin(GL_POINTS);
				for(int i=ev.xexpose.x;i<=ev.xexpose.x+ev.xexpose.width;i++)
					for(int j=ev.xexpose.y;j<=ev.xexpose.y+ev.xexpose.height;j++){
						glColor3ubv(col[manor[i][j]]);
						glVertex2i(i,j);
					}
				glEnd();
				glFlush();
			break;case ButtonPress:
				switch(ev.xbutton.button){
				default:goto rend;
				case Button1:
					if((unsigned)ev.xbutton.x>=512||(unsigned)ev.xbutton.y>=512)break;
					nx=ev.xbutton.x;
					ny=ev.xbutton.y;
				break;case Button4:case Button5:mxi+=ev.xbutton.button==Button4?25:mxi>25?-25:0;
				}
			break;case ButtonRelease:
				if(ev.xbutton.button==Button1&&(unsigned)ev.xbutton.x<512&&(unsigned)ev.xbutton.y<512){
					if(mans)
						for(int i=0;i<THREADS;i++)pthread_join(a[i],0);
					if(ev.xbutton.x==nx&&ev.xbutton.y==ny){
						xx+=(ev.xbutton.x-512)*wh;
						yy+=(ev.xbutton.y-512)*wh;
						wh*=2;
					}else{
						if(ev.xbutton.x<nx){
							int t=ev.xbutton.x;
							ev.xbutton.x=nx;
							nx=t;
						}
						if(ev.xbutton.y<ny){
							int t=ev.xbutton.y;
							ev.xbutton.y=ny;
							ny=t;
						}
						xx+=nx*wh;
						yy+=ny*wh;
						nx=ev.xbutton.x-nx;
						ny=ev.xbutton.y-ny;
						wh*=(nx-(nx-ny&nx-ny>>sizeof(int)*8-1))/512.;
					}
					rend:printf("%u\n%Lf\n%Lf\n%Lf\n\n",mxi,xx,yy,wh);
					mans=THREADS-1;
					for(int i=0;i<THREADS;i++){
						n[i]=i;
						pthread_create(a+i,&pat,drawman,manor+i);
					}
				}
			}
		}
		if(mans)
			for(int i=0;i<THREADS;i++)
				if(a[i]&&!pthread_tryjoin_np(a[i],0)){
					int k=n[i];
					if(++mans<512){
						n[i]=mans;
						pthread_create(a+i,&pat,drawman,manor+mans);
					}else a[i]=0;
					glBegin(GL_POINTS);
					for(int j=0;j<512;j++){
						glColor3ubv(col[manor[k][j]]);
						glVertex2i(k,j);
					}
					glEnd();
					if(mans>=511+THREADS){
						mans=0;
						glFlush();
						break;
					}
				}
	}
}
