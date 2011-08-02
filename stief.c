#include <stdio.h>
#include <GL/glew.h>
#include <GL/glx.h>
void printLog(GLuint obj)
{
	int len;
	glGetShaderiv(obj,GL_INFO_LOG_LENGTH,&len);
	char info[len];
	glGetShaderInfoLog(obj, len, &len, info);
	if(len)
		printf("%s\n",info);
}
int main(int argc,char**argv){
	float xx=-2,yy=-2,wh=1/128.;
	unsigned char manor[512][512];
	unsigned mxi=300;
	int nx,ny;
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
	glOrtho(0,511,511,0,-1,1);
	glewInit();
	const char*S="#version 120\nuniform vec3 p;uniform int m;void main(void){vec2 z=p.xy+vec2(gl_FragCoord.x*p[2],(512-gl_FragCoord.y)*p[2]),a=z*z,c=z;for(int i=m-1;i>-1;i--){z=vec2(a.x-a.y,z.x*z.y*2)+c;a=z*z;if(a.x+a.y>4){float I=float(i)/m;gl_FragColor.xyz=vec3(I*I*I,I*I,I);break;}}}";
	GLuint fs=glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &S, 0);
	glCompileShader(fs);
	printLog(fs);
	GLuint sp = glCreateProgram();
	glAttachShader(sp, fs);
	glLinkProgram(sp);
	printLog(sp);
	glUseProgram(sp);
	GLint xy=glGetUniformLocationARB(sp,"p"),mloc=glGetUniformLocationARB(sp,"m");
	glUniform1iARB(mloc,mxi);
	goto rend;
	for(;;){
		XEvent ev;
		xne:XNextEvent(dpy,&ev);
		switch(ev.type){
		case Expose:
			glBegin(GL_QUADS);
			glVertex2i(ev.xexpose.x,ev.xexpose.y);
			glVertex2i(ev.xexpose.x+ev.xexpose.width,ev.xexpose.y);
			glVertex2i(ev.xexpose.x+ev.xexpose.width,ev.xexpose.y+ev.xexpose.height);
			glVertex2i(ev.xexpose.x,ev.xexpose.y+ev.xexpose.height);
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
			break;case Button4:case Button5:
				mxi+=ev.xbutton.button==Button4?25:mxi>25?-25:0;
				glUniform1iARB(mloc,mxi);
			}
		break;case ButtonRelease:
			if(ev.xbutton.button==Button1&&(unsigned)ev.xbutton.x<512&&(unsigned)ev.xbutton.y<512){
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
				rend:printf("%u\n%f\n%f\n%f\n\n",mxi,xx,yy,wh);
				glUniform3fARB(xy,xx,yy,wh);
				glBegin(GL_QUADS);
				glVertex2i(0,0);
				glVertex2i(512,0);
				glVertex2i(512,512);
				glVertex2i(0,512);
				glEnd();
				glFlush();
			}else if(ev.xbutton.button==Button3){
				nx-=ev.xbutton.x;
				ny-=ev.xbutton.y;
				goto from3;
			}
		}
	}
}