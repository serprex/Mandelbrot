#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <X11/X.h>
#include <unistd.h>
#include <math.h>
#include <complex.h>
#include <limits.h>
#include <stdint.h>
#define THREADS 4

uint32_t*xx,*yy,*wh;
unsigned char manor[512][512];
unsigned mxi=100,pr=3;
Display*dpy;
Window win;

void prpr(uint32_t*x){
	printf("%c%x.",x[0]?'-':'+',x[pr-1]);
	for(int i=pr-2;i>1;i--) printf("%.8x ",x[i]);
	printf("%.8x\n",x[1]);
}

void cpy(uint32_t*restrict x,uint32_t*restrict y){
	for(int i=0;i<pr;i++) x[i]=y[i];
}
void add(uint32_t*restrict x,uint32_t*restrict y){
	uint32_t c=0;
	if(x[0]==y[0])
		for(int i=1;i<pr;i++){
			uint32_t t=x[i];
			c=(x[i]+=y[i]+c)<t;
		}
	else{
		for(int i=pr-1;i;i--){
			if(x[i]>y[i])
				for(int i=1;i<pr;i++){
					uint32_t t=x[i];
					x[i]-=y[i]+c;
					c=t<y[i]+c;
				}
			else if(x[i]<y[i]){
				x[0]^=1;
				for(int i=1;i<pr;i++){
					uint32_t t=x[i]+c;
					x[i]=y[i]-t;
					c=y[i]<t;
				}
			}else continue;
			return;
		}
		for(int i=1;i<pr;i++) x[i]=0;
	}
}
void sub(uint32_t*restrict x,uint32_t*restrict y){
	uint32_t c=0;
	if(x[0]==y[0]){
		for(int i=pr-1;i;i--){
			if(x[i]>y[i])
				for(int i=1;i<pr;i++){
					uint32_t t=x[i];
					x[i]-=y[i]+c;
					c=t<y[i]+c;
				}
			else if(x[i]<y[i]){
				x[0]^=1;
				for(int i=1;i<pr;i++){
					uint32_t t=x[i]+c;
					x[i]=y[i]-t;
					c=y[i]<t;
				}
			}else continue;
			return;
		}
		for(int i=1;i<pr;i++) x[i]=0;
	}else
		for(int i=1;i<pr;i++){
			uint32_t t=x[i];
			c=(x[i]+=y[i]+c)<t;
		}
}
void mul2(uint32_t*restrict x){
	int i=pr-1;
	do x[i]=x[i]<<1|x[i-1]>>31; while(--i);
}
void muli(uint32_t*restrict x,uint64_t y){
	uint64_t c=0;
	for(int i=1;i<pr;i++){
		x[i]=c+=x[i]*y;
		c>>=32;
	}
}
void mul(uint32_t*restrict x,uint32_t*restrict y){
	uint32_t z[pr];
	x[0]^=y[0];
	for(int i=0;i<pr;i++) z[i]=0;
	for(int i=1;i<pr;i++){
		uint64_t c=0;
		int j=1;
		for(;i+j<pr;j++)
			c=c+x[i]*(uint64_t)y[j]>>32;
		for(;j<pr;j++){
			z[i+j-pr]=c+=z[i+j-pr]+x[i]*(uint64_t)y[j];
			c>>=32;
		}
		z[i]=c;
	}
	for(int i=1;i<pr;i++) x[i]=z[i-1];
}
void mulx(uint32_t*restrict x){
	uint32_t z[pr*2];
	for(int i=1;i<pr*2;i++) z[i]=0;
	x[0]=0;
	for(int i=1;i<pr;i++){
		uint64_t f=x[i],c=z[i*2-1]+f*f;
		z[i*2-1]=c;
		c>>=32;
		for(int j=i;j<pr-1;j++){
			z[i+j]=c+=z[i+j]+2*x[j+1]*f;
			c>>=32;
		}
		z[i+pr-1]=c;
	}
	for(int i=1;i<pr;i++) x[i]=z[i+pr-2];
}
void*drawman(void*xv){
	uint32_t zr[pr],zi[pr],zz[pr],ii[pr],cr[pr],ci[pr];
	cpy(cr,wh);
	muli(cr,xv-(void*)manor>>9);
	add(cr,xx);
	cpy(ci,yy);
	for(int j=0;j<512;j++){
		cpy(zr,cr);
		add(ci,wh);
		cpy(zi,ci);
		unsigned k=mxi;
		do{
			cpy(zz,zr);
			cpy(ii,zi);
			mulx(ii);
			mulx(zr);
			sub(zr,ii);
			add(zr,cr);
			mul(zi,zz);
			mul2(zi);
			add(zi,ci);
			cpy(zz,zr);
			mulx(zz);
			cpy(ii,zi);
			mulx(ii);
			add(zz,ii);
		}while(--k&&zz[pr-1]<4);
		((unsigned char*)xv)[j]=(k<<8)/mxi;
	}
}

int main(int argc,char**argv){
	int nx,ny;
	pthread_t a[THREADS];
	xx=malloc(12);
	yy=malloc(12);
	wh=malloc(12);
	for(int i=0;i<pr;i++) xx[i]=yy[i]=wh[i]=0;
	xx[0]=yy[0]=1;
	xx[pr-1]=yy[pr-1]=2;
	wh[pr-2]=0x02000000;
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
	pthread_attr_setstacksize(&pat,PTHREAD_STACK_MIN+pr*16);
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
				if(XLookupString((XKeyEvent*)&event,(char[]){0},1,&keysym,NULL)==1&&keysym==XK_Escape){
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
					if((unsigned)event.xbutton.x>512||(unsigned)event.xbutton.y>512) break;
					nx=event.xbutton.x;
					ny=event.xbutton.y;
				}else if(event.xbutton.button==Button4) mxi+=25;
				else if(event.xbutton.button==Button5&&mxi>100) mxi-=25;
				else goto rend;
			break;case ButtonRelease:
				if(event.xbutton.button==Button1){
					if((unsigned)event.xbutton.x>512||(unsigned)event.xbutton.y>512) break;
					if(event.xbutton.x==nx&&event.xbutton.y==ny){
						uint32_t bx[pr];
						cpy(bx,wh);
						muli(bx,512-event.xbutton.x);
						sub(xx,bx);
						cpy(bx,wh);
						muli(bx,512-event.xbutton.y);
						sub(yy,bx);
						mul2(wh);
					}else{
						uint32_t bx[pr];
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
						cpy(bx,wh);
						muli(bx,nx);
						add(xx,bx);
						cpy(bx,wh);
						muli(bx,ny);
						add(yy,bx);
						nx=event.xbutton.x-nx;
						ny=event.xbutton.y-ny;
						muli(wh,nx-(nx-ny&nx-ny>>sizeof(int)*8-1));
						for(uint32_t i=2;i<pr;i++)
							wh[i-1]=wh[i-1]>>9|(wh[i]&511)<<23;
						wh[pr-1]>>=9;
						if(!wh[2]&&wh[1]){
							pr++;
							xx=realloc(xx,4*pr);
							yy=realloc(yy,4*pr);
							wh=realloc(wh,4*pr);
							for(int i=pr-1;i>1;i--){
								xx[i]=xx[i-1];
								yy[i]=yy[i-1];
								wh[i]=wh[i-1];
							}
							xx[1]=yy[1]=wh[1]=0;
						}
					}
					rend:;
					prpr(xx);
					prpr(yy);
					prpr(wh);
					puts("");
					uint32_t n[THREADS],mans=THREADS-1;
					for(int i=0;i<THREADS;i++){
						n[i]=i;
						pthread_create(a+i,&pat,drawman,manor+i);
					}
					while(mans<511+THREADS)
						for(int i=0;i<THREADS;i++)
							if(!pthread_tryjoin_np(a[i],0)){
								int k=n[i];
								if(++mans<512){
									n[i]=mans;
									pthread_create(a+i,&pat,drawman,manor+mans);
								}
								glBegin(GL_POINTS);
								for(int j=0;j<512;j++){
									glColor3ub(manor[k][j]*manor[k][j]*manor[k][j]>>16,manor[k][j]*manor[k][j]>>8,manor[k][j]);
									glVertex2i(k,j);
								}
								glEnd();
								glFlush();
							}
				}
			}
		}
	}
}
