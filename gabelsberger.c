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
unsigned mxi=300,q;
mp_size_t pr=1;
mp_limb_t mymp_lsh(mp_limb_t*r,mp_size_t p,unsigned n){
	while(n>=mp_bits_per_limb){
		mpn_lshift(r,r,p,mp_bits_per_limb-1);
		n-=mp_bits_per_limb-1;
	}
	return mpn_lshift(r,r,p,n);
}
mp_limb_t mymp_rsh(mp_limb_t*r,mp_size_t p,unsigned n){
	while(n>=mp_bits_per_limb){
		mpn_rshift(r,r,p,mp_bits_per_limb-1);
		n-=mp_bits_per_limb-1;
	}
	return mpn_rshift(r,r,p,n);
}
void mymp_add(mp_limb_t*r,const mp_limb_t*a,const mp_limb_t*b,mp_size_t n,_Bool as,_Bool bs,_Bool*rs){
	*rs=as;
	if(as==bs) mpn_add_n(r,a,b,n);
	else if(mpn_cmp(a,b,n)<0){
		*rs=!as;
		mpn_sub_n(r,b,a,n);
	}else mpn_sub_n(r,a,b,n);
}
void*drawman(void*xv){
	mp_limb_t zr[pr*2],zi[pr*2],ii[pr*2],i2[pr*2],cr[pr*2],ci[pr*2];
	_Bool zrs,zis,crs,cis,iis;
	mpn_mul_1(cr,wh,pr,xv-(void*)manor>>9);
	mymp_add(cr,cr,xx,pr,0,xs,&crs);
	mpn_copyi(ci,yy,pr);
	cis=ys;
	unsigned q=pr*mp_bits_per_limb-5;
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
			mymp_rsh(ii,pr*2,q-1);
			mpn_sqr(zr,zr,pr);
			mymp_rsh(zr,pr*2,q);
			mpn_sqr(zi,zi,pr);
			mymp_rsh(zi,pr*2,q);
			mymp_add(zr,zr,zi,pr,0,1,&zrs);
			mymp_add(zr,zr,cr,pr,zrs,crs,&zrs);
			mymp_add(zi,ii,ci,pr,iis,cis,&zis);
			mpn_sqr(ii,zr,pr);
			mymp_rsh(ii,pr*2,q);
			mpn_sqr(i2,zi,pr);
			mymp_rsh(i2,pr*2,q);
			mpn_add_n(ii,ii,i2,pr);
		}while(--k&&mpn_cmp(ii,z4,pr)<0);
		((unsigned char*)xv)[j]=(k<<8)/mxi;
	}
}
int main(int argc,char**argv){
	int nx,ny;
	pthread_t a[THREADS];
	xx=calloc(pr*4,sizeof(mp_limb_t));
	yy=xx+pr;
	wh=yy+pr;
	z4=wh+pr;
	yy[0]=xx[0]=2;
	wh[0]=z4[0]=4;
	mymp_lsh(xx,pr,pr*mp_bits_per_limb-5);
	mymp_lsh(yy,pr,pr*mp_bits_per_limb-5);
	mymp_lsh(z4,pr,pr*mp_bits_per_limb-5);
	mymp_lsh(wh,pr,pr*mp_bits_per_limb-14);
	Display*dpy=XOpenDisplay(0);
	XVisualInfo*vi=glXChooseVisual(dpy,DefaultScreen(dpy),(int[]){GLX_RGBA,None});
	Window win=XCreateWindow(dpy,RootWindow(dpy,vi->screen),0,0,511,511,0,vi->depth,InputOutput,vi->visual,CWBorderPixel|CWColormap|CWEventMask,(XSetWindowAttributes[]){{.colormap=XCreateColormap(dpy,RootWindow(dpy,vi->screen),vi->visual,AllocNone),.border_pixel=0,.event_mask=ExposureMask|KeyPressMask|ButtonPressMask|ButtonReleaseMask}});
	XMapWindow(dpy,win);
	GLXContext ctx=glXCreateContext(dpy,vi,0,GL_TRUE);
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
		XEvent ev;
		XNextEvent(dpy,&ev);
		switch(ev.type){
		case KeyPress:{
			KeySym keysym;
			if(XLookupString((XKeyEvent*)&ev,(char[]){0},1,&keysym,NULL)==1&&keysym==XK_Escape){
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
			if(ev.xbutton.button==Button1){
				if((unsigned)ev.xbutton.x>512||(unsigned)ev.xbutton.y>512) break;
				nx=ev.xbutton.x;
				ny=ev.xbutton.y;
			}else if(ev.xbutton.button==Button4) mxi+=25;
			else if(ev.xbutton.button==Button5&&mxi>100) mxi-=25;
			else goto rend;
		break;case ButtonRelease:
			if(ev.xbutton.button==Button1){
				if((unsigned)ev.xbutton.x>512||(unsigned)ev.xbutton.y>512) break;
				if(ev.xbutton.x==nx&&ev.xbutton.y==ny){
					if(xs){
						mpn_addmul_1(xx,wh,pr,512-ev.xbutton.x);
					}else{
						mp_limb_t t[pr*2];
						mpn_mul_1(t,wh,pr,512-ev.xbutton.x);
						if(mpn_cmp(xx,t,pr)<0){
							xs=!xs;
							mpn_sub_n(xx,t,xx,pr);
						}else mpn_sub_n(xx,xx,t,pr);
					}
					if(ys){
						mpn_addmul_1(yy,wh,pr,512-ev.xbutton.y);
					}else{
						mp_limb_t t[pr*2];
						mpn_mul_1(t,wh,pr,512-ev.xbutton.y);
						if(mpn_cmp(yy,t,pr)<0){
							ys=!ys;
							mpn_sub_n(yy,t,yy,pr);
						}else mpn_sub_n(yy,yy,t,pr);
					}
					mpn_lshift(wh,wh,pr,1);
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
					if(!xs){
						mpn_addmul_1(xx,wh,pr,nx);
					}else{
						mp_limb_t t[pr*2];
						mpn_mul_1(t,wh,pr,nx);
						if(mpn_cmp(xx,t,pr)<0){
							xs=!xs;
							mpn_sub_n(xx,t,xx,pr);
						}else mpn_sub_n(xx,xx,t,pr);
					}
					if(!ys){
						mpn_addmul_1(yy,wh,pr,ny);
					}else{
						mp_limb_t t[pr*2];
						mpn_mul_1(t,wh,pr,ny);
						if(mpn_cmp(yy,t,pr)<0){
							ys=!ys;
							mpn_sub_n(yy,t,yy,pr);
						}else mpn_sub_n(yy,yy,t,pr);
					}
					nx=ev.xbutton.x-nx;
					ny=ev.xbutton.y-ny;
					mpn_mul_1(wh,wh,pr,nx-(nx-ny&nx-ny>>sizeof(int)*8-1));
					mpn_rshift(wh,wh,pr,9);
					if(xx[0]||yy[0]){
						pr++;
						xx=realloc(xx,pr*4*sizeof(mp_limb_t));
						yy=xx+pr;
						wh=yy+pr;
						z4=wh+pr;
						for(int i=pr-1;i>-1;i--){
							z4[i]=z4[i-4];
							wh[i]=wh[i-3];
							yy[i]=yy[i-2];
							xx[i]=xx[i-1];
						}
						xx[0]=yy[0]=wh[0]=z4[0]=0;
					}
				}
				rend:gmp_printf("%u\n%Nx\n%Nx\n%Nx\n",pr,xx,pr,yy,pr,wh,pr);
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
