#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <gmp.h>
#define THREADS 4
mp_limb_t*xx,*yy,*wh;
_Bool xs=1,ys=1;
unsigned char manor[512][512];
unsigned mxi=300;
mp_size_t pr=1;
void mymp_add(mp_limb_t*r,const mp_limb_t*a,const mp_limb_t*b,mp_size_t n,_Bool as,_Bool bs,_Bool*rs){
	*rs=as;
	if(as==bs) mpn_add_n(r,a,b,n);
	else if(mpn_cmp(a,b,n)<0){
		*rs=!as;
		mpn_sub_n(r,b,a,n);
	}else mpn_sub_n(r,a,b,n);
}
void*drawman(void*xv){
	mp_limb_t zr[pr*2],zi[pr*2],ii[pr*2],i2[pr*2],cr[pr],ci[pr];
	_Bool zrs,zis,crs,cis=ys,iis;
	mpn_mul_1(cr,wh,pr,xv-(void*)manor>>9);
	mymp_add(cr,cr,xx,pr,0,xs,&crs);
	mpn_copyi(ci,yy,pr);
	for(int j=0;j<512;j++){
		zrs=crs;
		zis=cis;
		mpn_copyi(zr,cr,pr);
		mymp_add(ci,ci,wh,pr,cis,0,&cis);
		mpn_copyi(zi,ci,pr);
		unsigned k=mxi;
		do{
			iis=zrs^zis;
			mpn_mul_n(ii,zr,zi,pr);
			mpn_lshift(ii+pr,ii+pr,pr,6);
			mpn_sqr(zr,zr,pr);
			mpn_sqr(zi,zi,pr);
			mymp_add(zr,zr+pr,zi+pr,pr,0,1,&zrs);
			mpn_lshift(zr,zr,pr,5);
			mymp_add(zr,zr,cr,pr,zrs,crs,&zrs);
			mymp_add(zi,ii+pr,ci,pr,iis,cis,&zis);
			mpn_sqr(ii,zr,pr);
			mpn_sqr(i2,zi,pr);
			mpn_add_n(ii+pr,ii+pr,i2+pr,pr);
		}while(--k&&ii[pr*2-1]<(4UL<<mp_bits_per_limb-10));
		((unsigned char*)xv)[j]=(k<<8)/mxi;
	}
}
int main(int argc,char**argv){
	int nx,ny,n[THREADS],mans=0;
	pthread_t a[THREADS];
	wh=(yy=(xx=malloc(3*sizeof(mp_limb_t)))+1)+1;
	wh[0]=(yy[0]=xx[0]=2ULL<<mp_bits_per_limb-5)>>8;
	Display*dpy=XOpenDisplay(0);
	XVisualInfo*vi=glXChooseVisual(dpy,DefaultScreen(dpy),(int[]){GLX_RGBA,None});
	Window win=XCreateWindow(dpy,RootWindow(dpy,vi->screen),0,0,511,511,0,vi->depth,InputOutput,vi->visual,CWBorderPixel|CWColormap|CWEventMask,(XSetWindowAttributes[]){{.colormap=XCreateColormap(dpy,RootWindow(dpy,vi->screen),vi->visual,AllocNone),.border_pixel=0,.event_mask=ExposureMask|KeyPressMask|ButtonPressMask|ButtonReleaseMask}});
	XMapWindow(dpy,win);
	GLXContext ctx=glXCreateContext(dpy,vi,0,GL_TRUE);
	glXMakeCurrent(dpy,win,ctx);
	glOrtho(0,511,511,0,1,-1);
	pthread_attr_t pat;
	pthread_attr_init(&pat);
	pthread_attr_setguardsize(&pat,0);
	goto rend;
	for(;;){
		XEvent ev;
		if(XPending(dpy)){
			XNextEvent(dpy,&ev);
			switch(ev.type){
			case KeyPress:{
				KeySym keysym;
				char buff;
				if(XLookupString((XKeyEvent*)&ev,&buff,1,&keysym,NULL)==1&&keysym==XK_Escape)return 0;
			}
			break;case Expose:
				glBegin(GL_POINTS);
				for(int i=0;i<512;i++)
					for(int j=0;j<512;j++){
						glColor3ub(manor[i][j]*manor[i][j]*manor[i][j]>>16,manor[i][j]*manor[i][j]>>8,manor[i][j]);
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
						mp_limb_t t[pr];
						if(xs)mpn_addmul_1(xx,wh,pr,512-ev.xbutton.x);
						else{
							mpn_mul_1(t,wh,pr,512-ev.xbutton.x);
							if(mpn_cmp(xx,t,pr)<0){
								xs=!xs;
								mpn_sub_n(xx,t,xx,pr);
							}else mpn_sub_n(xx,xx,t,pr);
						}
						if(ys)mpn_addmul_1(yy,wh,pr,512-ev.xbutton.y);
						else{
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
						mp_limb_t t[pr];
						if(xs){
							mpn_mul_1(t,wh,pr,nx);
							if(mpn_cmp(xx,t,pr)<0){
								xs=!xs;
								mpn_sub_n(xx,t,xx,pr);
							}else mpn_sub_n(xx,xx,t,pr);
						}else mpn_addmul_1(xx,wh,pr,nx);
						if(ys){
							mpn_mul_1(t,wh,pr,ny);
							if(mpn_cmp(yy,t,pr)<0){
								ys=!ys;
								mpn_sub_n(yy,t,yy,pr);
							}else mpn_sub_n(yy,yy,t,pr);
						}else mpn_addmul_1(yy,wh,pr,ny);
						nx=ev.xbutton.x-nx;
						ny=ev.xbutton.y-ny;
						mpn_mul_1(wh,wh,pr,nx-(nx-ny&nx-ny>>sizeof(int)*8-1));
						mpn_rshift(wh,wh,pr,9);
						if(xx[0]||yy[0]||wh[0]){
							pr++;
							wh=(yy=(xx=realloc(xx,pr*3*sizeof(mp_limb_t)))+pr)+pr;
							mpn_copyd(wh+1,wh-2,pr-1);
							mpn_copyd(yy+1,yy-1,pr-1);
							mpn_copyd(xx+1,xx,pr-1);
							xx[yy[wh[0]=0]=0]=0;
						}
					}
					rend:gmp_printf("%u %u\n%Nx\n%Nx\n%Nx\n",pr,mxi,xx,pr,yy,pr,wh,pr);
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
				if(!pthread_tryjoin_np(a[i],0)){
					int k=n[i];
					if(++mans<512){
						n[i]=mans;
						pthread_create(a+i,&pat,drawman,manor+mans);
					}else if(mans>=511+THREADS){
						mans=0;
						glFlush();
						break;
					}
					glBegin(GL_POINTS);
					for(int j=0;j<512;j++){
						glColor3ub(manor[k][j]*manor[k][j]*manor[k][j]>>16,manor[k][j]*manor[k][j]>>8,manor[k][j]);
						glVertex2i(k,j);
					}
					glEnd();
				}
	}
}
