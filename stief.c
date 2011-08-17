#include <stdio.h>
#include <GL/glew.h>
#include <GL/glx.h>
#include <unistd.h>
int main(int argc,char**argv){
	float xx=-2,yy=-2,wh=1/128.,mx=300;
	int nx,ny,r=1;
	Display*dpy=XOpenDisplay(0);
	XVisualInfo*vi=glXChooseVisual(dpy,DefaultScreen(dpy),(int[]){GLX_RGBA,None});
	Window win=XCreateWindow(dpy,RootWindow(dpy,vi->screen),0,0,511,511,0,vi->depth,InputOutput,vi->visual,CWColormap|CWEventMask,(XSetWindowAttributes[]){{.colormap=XCreateColormap(dpy,RootWindow(dpy,vi->screen),vi->visual,AllocNone),.event_mask=ExposureMask|ButtonPressMask|ButtonReleaseMask}});
	XMapWindow(dpy,win);
	glXMakeCurrent(dpy,win,glXCreateContext(dpy,vi,0,GL_TRUE));
	glOrtho(0,511,511,0,-1,1);
	glewInit();
	const char*S="#version 150\nlayout(origin_upper_left) in vec4 gl_FragCoord;out vec4 C;uniform vec4 p;void main(void){vec2 z=p.xy+gl_FragCoord.xy*p.z,a=z*z,c=z;for(float i=p.w-1;i>-1;i--){z=vec2(a.x-a.y,z.x*z.y*2)+c;a=z*z;if(a.x+a.y>4){float I=i/p.w;C.xyz=vec3(I*I*I,I*I,I);break;}}}";
	GLuint fs=glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs,1,&S,0);
	glCompileShader(fs);
	GLuint sp=glCreateProgram();
	glBindFragDataLocation(sp,0,"C");
	glAttachShader(sp,fs);
	glLinkProgram(sp);
	glUseProgram(sp);
	GLint xy=glGetUniformLocationARB(sp,"p");
	for(;;){
		if(!XPending(dpy)&&r){
			rend:r=0;
			printf("%d\n%f\n%f\n%f\n\n",(int)mx,xx,yy,wh);
			glUniform4fARB(xy,xx,yy,wh,mx);
			glRecti(0,0,512,512);
			glFinish();
		}
		XEvent ev;
		xne:XNextEvent(dpy,&ev);
		switch(ev.type){
		case Expose:r=1;
		break;case ButtonPress:
			switch(ev.xbutton.button){
			case Button1:case Button3:
				nx=ev.xbutton.x;
				ny=ev.xbutton.y;
			break;case Button4:case Button5:
				mx+=ev.xbutton.button==Button4?100:mx>100?-100:0;
			}
		break;case ButtonRelease:
			if(ev.xbutton.button==Button1){
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
						wh*=(nx-(nx-ny&nx-ny>>31))/512.;
					}
				}
				r=1;
			}else if(ev.xbutton.button==Button3){
				nx-=ev.xbutton.x;
				ny-=ev.xbutton.y;
				goto from3;
			}
		}
	}
}