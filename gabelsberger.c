#define _GNU_SOURCE
#include <pthread.h>
#include <gmp.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <unistd.h>
#include <limits.h>
#define THREADS 4
mp_limb_t*xx,*yy,*wh,*z4;
_Bool xs=1,ys=1;
unsigned char manor[512][512];
unsigned mxi=300,q=59;
mp_size_t pr;
mp_limb_t mymp_lsh(mp_limb_t*r,mp_limb_t*s,mp_size_t p,unsigned n){
	while(n>=mp_bits_per_limb){
		mpn_lshift(r,s,p,mp_bits_per_limb-1);
		n-=mp_bits_per_limb-1;
	}
	return mpn_lshift(r,s,p,n);
}
mp_limb_t mymp_rsh(mp_limb_t*r,mp_limb_t*s,mp_size_t p,unsigned n){
	while(n>=mp_bits_per_limb){
		mpn_rshift(r,s,p,mp_bits_per_limb-1);
		n-=mp_bits_per_limb-1;
	}
	return mpn_rshift(r,s,p,n);
}
void mymp_add(mp_limb_t*r,const mp_limb_t*a,const mp_limb_t*b,mp_size_t n,_Bool as,_Bool bs,_Bool*rs){
	*rs=as;
	if(as==bs) mpn_add_n(r,a,b,n);
	else if(mpn_cmp(a,b,n)>=0) mpn_sub_n(r,a,b,n);
	else{
		*rs=!as;
		mpn_sub_n(r,b,a,n);
	}
}
void*drawman(void*xv){
	mp_limb_t zr[pr*2],zi[pr*2],ii[pr*2],i2[pr*2],cr[pr*2],ci[pr*2];
	_Bool zrs,zis,crs,cis,iis;
	mpn_mul_1(cr,wh,pr,xv-(void*)manor>>9);
	mymp_add(cr,cr,xx,pr,0,xs,&crs);
	mpn_copyi(ci,yy,pr);
	cis=ys;
	for(mp_limb_t j=0;j<512;j++){
		zrs=crs;
		mpn_copyi(zr,cr,pr);
		mymp_add(ci,ci,wh,pr,cis,0,&cis);
		zis=cis;
		mpn_copyi(zi,ci,pr);
		unsigned k=mxi;
		do{
			iis=zrs^zis;
			mpn_mul_n(ii,zr,zi,pr);
			mymp_rsh(ii,ii,pr*2,q-1);
			mpn_sqr(zr,zr,pr);
			mymp_rsh(zr,zr,pr*2,q);
			mpn_sqr(zi,zi,pr);
			mymp_rsh(zi,zi,pr*2,q);
			mymp_add(zr,zr,zi,pr,0,1,&zrs);
			mymp_add(zr,zr,cr,pr,zrs,crs,&zrs);
			mymp_add(zi,ii,ci,pr,iis,cis,&zis);
			mpn_sqr(ii,zr,pr);
			mymp_rsh(ii,ii,pr*2,q);
			mpn_sqr(i2,zi,pr);
			mymp_rsh(i2,i2,pr*2,q);
			mpn_add_n(ii,ii,i2,pr);
		}while(--k&&mpn_cmp(ii,z4,pr)<0);
		((unsigned char*)xv)[j]=(k<<8)/mxi;
	}
}
int main(int argc,char**argv){
	int nx,ny;
	pthread_t a[THREADS];
	pr=1+(q+4)/mp_bits_per_limb;
	xx=calloc(pr*4,sizeof(mp_limb_t));
	yy=xx+pr;
	wh=yy+pr;
	z4=wh+pr;
	yy[0]=xx[0]=2;
	wh[0]=z4[0]=4;
	mymp_lsh(xx,xx,pr,q);
	mymp_lsh(yy,yy,pr,q);
	mymp_lsh(z4,z4,pr,q);
	mymp_lsh(wh,wh,pr,q-9);
	Display*dpy=XOpenDisplay(0);
	XVisualInfo*vi=glXChooseVisual(dpy,DefaultScreen(dpy),(int[]){GLX_RGBA,None});
	Window win=XCreateWindow(dpy,RootWindow(dpy,vi->screen),0,0,511,511,0,vi->depth,InputOutput,vi->visual,CWBorderPixel|CWColormap|CWEventMask,(XSetWindowAttributes[]){{.colormap=XCreateColormap(dpy,RootWindow(dpy,vi->screen),vi->visual,AllocNone),.border_pixel=0,.event_mask=ExposureMask|KeyPressMask|ButtonPressMask|ButtonReleaseMask}});
	XMapWindow(dpy,win);
	GLXContext ctx=glXCreateContext(dpy,vi,0,GL_TRUE);
	XEvent event;
	glXMakeCurrent(dpy,win,ctx);
	glOrtho(0,511,511,0,1,-1);
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
					mpn_submul_1(xx,wh,pr,512-event.xbutton.x);
					mpn_submul_1(yy,wh,pr,512-event.xbutton.y);
					mpn_lshift(wh,wh,pr,1);
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
					mpn_addmul_1(xx,wh,pr,nx);
					mpn_addmul_1(yy,wh,pr,ny);
					nx=event.xbutton.x-nx;
					ny=event.xbutton.y-ny;
					mpn_mul_1(wh,wh,pr,nx-(nx-ny&nx-ny>>sizeof(int)*8-1));
					mpn_rshift(wh,wh,pr,9);
				}
				rend:;
				gmp_printf("%u\n%Nx\n%Nx\n%Nx\n",q,xx,pr,yy,pr,wh,pr);
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
				return 0;
			}
		}
	}
}
