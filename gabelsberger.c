#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <GL/glx.h>
#include <stdlib.h>
#include <gmp.h>
#include <string.h>
#define THREADS 4
mp_limb_t*xx,*yy,*wh;
_Bool xs=1,ys=1;
volatile _Bool pull;
unsigned char manor[512][512];
unsigned long long done[8];
unsigned mxi=300;
mp_size_t pr=1;
pthread_mutex_t xcol;
int col;
static inline void mymp_add(mp_limb_t*r,const mp_limb_t*a,const mp_limb_t*b,mp_size_t n,_Bool as,_Bool bs,_Bool*rs){
	*rs=as;
	if(as==bs)mpn_add_n(r,a,b,n);
	else if(mpn_cmp(a,b,n)<0){
		*rs=!as;
		mpn_sub_n(r,b,a,n);
	}else mpn_sub_n(r,a,b,n);
}
void*drawman(void*x){
	int c=-1;
	for(;;){
		pthread_mutex_lock(&xcol);
		if(c!=-1)done[c>>6]|=1ULL<<(c&63);
		c=col++;
		pthread_mutex_unlock(&xcol);
		if(c>=512||pull)return 0;
		mp_limb_t cr[pr],ci[pr],zr[pr],zi[pr],ii[pr*2],i2[pr*2];
		_Bool zrs,zis,crs,cis=ys,iis;
		mpn_mul_1(cr,wh,pr,c);
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
				mpn_lshift(ii+pr,ii+pr,pr,5);
				mpn_sqr(i2,zi,pr);
				mymp_add(zi,ii+pr,ci,pr,iis,cis,&zis);
				mpn_sqr(ii,zr,pr);
				mymp_add(zr,ii+pr,i2+pr,pr,0,1,&zrs);
				mpn_lshift(zr,zr,pr,4);
				mymp_add(zr,zr,cr,pr,zrs,crs,&zrs);
				mpn_sqr(ii,zr,pr);
				mpn_sqr(i2,zi,pr);
			}while(--k&&ii[pr*2-1]+i2[pr*2-1]<(1ULL<<mp_bits_per_limb-6));
			manor[c][j]=(k<<8)/mxi;
		}
	}
}
int main(int argc,char**argv){
	int nx,ny,mans=0;
	_Bool first=1;
	pthread_t a[THREADS]={};
	unsigned char C[256][3];
	for(int i=0;i<256;i++){
		C[i][0]=i*i*i>>16;
		C[i][1]=i*i>>8;
		C[i][2]=i;
	}
	wh=(yy=(xx=malloc(3*sizeof(mp_limb_t)))+1)+1;
	*wh=(*yy=*xx=1ULL<<mp_bits_per_limb-3)>>8;
	Display*dpy=XOpenDisplay(0);
	XVisualInfo*vi=glXChooseVisual(dpy,DefaultScreen(dpy),(int[]){GLX_RGBA,None});
	Window win=XCreateWindow(dpy,RootWindow(dpy,vi->screen),0,0,511,511,0,vi->depth,InputOutput,vi->visual,CWColormap|CWEventMask,(XSetWindowAttributes[]){{.colormap=XCreateColormap(dpy,RootWindow(dpy,vi->screen),vi->visual,AllocNone),.event_mask=ExposureMask|ButtonPressMask|ButtonReleaseMask}});
	XMapWindow(dpy,win);
	glXMakeCurrent(dpy,win,glXCreateContext(dpy,vi,0,GL_TRUE));
	glOrtho(0,511,511,0,1,-1);
	pthread_mutex_init(&xcol,0);
	pthread_attr_t pat;
	pthread_attr_init(&pat);
	pthread_attr_setguardsize(&pat,0);
	goto fend;
	for(;;){
		XEvent ev;
		ever:if(XPending(dpy)||mans==512){
			xne:XNextEvent(dpy,&ev);
			switch(ev.type){
			case Expose:
				glBegin(GL_POINTS);
				for(int i=ev.xexpose.x;i<=ev.xexpose.x+ev.xexpose.width;i++)
					for(int j=ev.xexpose.y;j<=ev.xexpose.y+ev.xexpose.height;j++){
						glColor3ubv(C[manor[i][j]]);
						glVertex2i(i,j);
					}
				glEnd();
				glFlush();
			break;case ButtonPress:
				switch(ev.xbutton.button){
				default:goto rend;
				case Button1:
					if((unsigned)ev.xbutton.x>=512||(unsigned)ev.xbutton.y>=512)break;
					case Button3:
					nx=ev.xbutton.x;
					ny=ev.xbutton.y;
				break;case Button4:case Button5:mxi+=ev.xbutton.button==Button4?25:mxi>25?-25:0;
				}
			break;case ButtonRelease:
				if(ev.xbutton.button==Button1&&(unsigned)ev.xbutton.x<512&&(unsigned)ev.xbutton.y<512){
					if(mans!=512){
						pull=1;
						for(int i=0;i<THREADS;i++)
							pthread_join(a[i],0);
						pull=0;
					}
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
						from3:;
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
						if(ev.xbutton.button==Button1){
							nx=ev.xbutton.x-nx;
							ny=ev.xbutton.y-ny;
							mpn_mul_1(wh,wh,pr,nx-(nx-ny&nx-ny>>sizeof(int)*8-1));
							mpn_rshift(wh,wh,pr,9);
							if(*xx&4095||*yy&4095||*wh&4095){
								pr++;
								wh=(yy=(xx=realloc(xx,pr*3*sizeof(mp_limb_t)))+pr)+pr;
								mpn_copyd(wh+1,wh-2,pr-1);
								mpn_copyd(yy+1,yy-1,pr-1);
								mpn_copyd(xx+1,xx,pr-1);
								*xx=*yy=*wh=0;
							}
						}
					}
					rend:gmp_printf("%u %u\n%Nx\n%Nx\n%Nx\n",pr,mxi,xx,pr,yy,pr,wh,pr);
					for(int i=0;i<THREADS;i++)
						pthread_join(a[i],0);
					pull=0;
					mans=col=0;
					memset(done,0,sizeof(done));
					fend:for(int i=0;i<THREADS;i++)
						pthread_create(a+i,&pat,drawman,0);
				}else if(ev.xbutton.button==Button3){
					if(mans!=512)
						pull=1;
					nx-=ev.xbutton.x;
					ny-=ev.xbutton.y;
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
					goto rend;
				}
			}
		}
		if(mans!=512){
			pthread_mutex_lock(&xcol);
			for(int i=mans>>6;i>=0;i--)
				if(done[i]){
					int lo=ffsll(done[i])-1;
					if(done[i]&1ULL<<lo){
						done[i]^=1ULL<<lo;
						pthread_mutex_unlock(&xcol);
						int io=i*64+lo;
						glBegin(GL_POINTS);
						for(int j=0;j<512;j++){
							glColor3ubv(C[manor[io][j]]);
							glVertex2i(io,j);
						}
						glEnd();
						if(++mans==512){
							for(int i=0;i<THREADS;i++)
								pthread_join(a[i],0);
							glFlush();
							goto xne;
						}
						goto ever;
					}
				}
			pthread_mutex_unlock(&xcol);
		}
	}
}
