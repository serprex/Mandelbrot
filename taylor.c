#include <pthread.h>
#include <stdio.h>
#include <GL/glx.h>
#include <complex.h>
#include <string.h>
#include <stdint.h>
#include <xmmintrin.h>
#define THREADS 4
volatile _Bool pull;
double xx=-2,yy=-2,wh=1/128.;
unsigned char manor[512][512];
unsigned long long done[8];
unsigned mx=300,col;
pthread_mutex_t xcol;
void*drawman(void*x){
	pthread_mutex_lock(&xcol);
	int c=col++;
	pthread_mutex_unlock(&xcol);
	do{
		__m128d cr=_mm_set1_pd(xx+wh*c);
		for(int j=0;j<512;j+=2){
			__m128d zr=cr,
				zi=_mm_set_pd(yy+wh*j,yy+wh*(j+1)),ci=zi,
				zr2=_mm_mul_pd(zr,zr),zi2=_mm_mul_pd(zi,zi);
			uint64_t kkini[2]__attribute__((aligned(16)))={mx-1,mx-1};
			__m128i k=_mm_load_si128((__m128i*)kkini);
			unsigned mk=mx;
			while(--mk){
				zi=_mm_mul_pd(zi,zr);
				zi=_mm_add_pd(zi,zi);
				zi=_mm_add_pd(zi,ci);
				zr=_mm_add_pd(_mm_sub_pd(zr2,zi2),cr);
				zr2=_mm_mul_pd(zr,zr);
				zi2=_mm_mul_pd(zi,zi);
				__m128d n=_mm_cmplt_pd(_mm_add_pd(zr2,zi2),_mm_set1_pd(4));
				if(!_mm_movemask_pd(n))break;
				k=_mm_add_epi64(k,_mm_castpd_si128(n));
			}
			uint64_t kk[2]__attribute__((aligned(16)));
			_mm_store_si128((__m128i*)kk,k);
			manor[c][j]=(kk[1]<<8)/mx;
			manor[c][j+1]=(kk[0]<<8)/mx;
		}
		pthread_mutex_lock(&xcol);
		done[c>>6]|=1ULL<<(c&63);
		c=col++;
		pthread_mutex_unlock(&xcol);
	}while(c<512&&!pull);
}
int main(int argc,char**argv){
	int nx,ny,mans=0;
	pthread_t a[THREADS];
	unsigned char C[256][3];
	for(int i=0;i<256;i++){
		C[i][0]=i*i*i>>16;
		C[i][1]=i*i>>8;
		C[i][2]=i;
	}
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
		ever:if(XPending(dpy)){
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
				case Button1:case Button3:
					nx=ev.xbutton.x;
					ny=ev.xbutton.y;
				break;case Button4:case Button5:mx+=ev.xbutton.button==Button4?25:mx>25?-25:0;
				}
			break;case ButtonRelease:
				if(ev.xbutton.button==Button1){
					pull=1;
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
						from3:
						xx+=nx*wh;
						yy+=ny*wh;
						if(ev.xbutton.button==Button1){
							nx=ev.xbutton.x-nx;
							ny=ev.xbutton.y-ny;
							wh*=(nx-(nx-ny&nx-ny>>sizeof(int)*8-1))/512.;
						}
					}
					rend:printf("%u\n%f\n%f\n%f\n\n",mx,xx,yy,wh);
					for(int i=0;i<THREADS;i++)
						pthread_join(a[i],0);
					pull=mans=col=0;
					memset(done,0,sizeof(done));
					fend:for(int i=0;i<THREADS;i++)
						pthread_create(a+i,&pat,drawman,0);
				}else if(ev.xbutton.button==Button3){
					pull=1;
					nx-=ev.xbutton.x;
					ny-=ev.xbutton.y;
					goto from3;
				}
			}
		}
		if(mans!=512){
			pthread_mutex_lock(&xcol);
			for(int m64=mans>>6,i=m64;i>=m64-1;i--)
				if(done[i]){
					int lo=__builtin_ctzll(done[i]);
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
			pthread_mutex_unlock(&xcol);
		}
	}
}
