#define _GNU_SOURCE
#include <CL/cl.h>
#include <stdio.h>
#include <GL/glx.h>
#include <string.h>
float xx=-2,yy=-2,wh=1/128.;
unsigned char manor[512][512];
unsigned long long done[8];
unsigned mxi=300;
int col;
#define CL_CHECK(_expr)do{cl_int _err=_expr;if(_err!=CL_SUCCESS)return printf("'%s' returned %d!\n",#_expr,_err);}while(0)
#define CL_CHECK_ERR(_expr)({cl_int _err;typeof(_expr)_ret=_expr;if(_err!=CL_SUCCESS)return printf("'%s' returned %d!\n",#_expr,(int)_err);_ret;})
int main(int argc,char**argv){
	cl_platform_id pl;
	cl_device_id dv;
	cl_uint pls,dvs;
	CL_CHECK(clGetPlatformIDs(1,&pl,&pls));
	if(!pls)return printf("No platforms");
	CL_CHECK(clGetDeviceIDs(pl,CL_DEVICE_TYPE_GPU,1,&dv,&dvs));
	if(!dvs)return printf("No devices");
	cl_context cx=CL_CHECK_ERR(clCreateContext(0,1,&dv,0,0,&_err));
	const char*S="__kernel void m(__global int*dst,const float x,const float y,const float w,const int t){const int g=get_global_id(0),G=g*4;const float4 Y=y+w*(float4)(G,G+1,G+2,G+3);float4 a=x,b=Y,c=a*a,d=b*b;int4 i=t-1;for(;;){b=(a*b)*2+Y;a=c-d+x;c=a*a;d=b*b;const int4 n=(c+d<4)&(i>0);if(!any(n))break;i-=n&1;}i=(i<<8)/t;dst[g]=i.s0|i.s1<<8|i.s2<<16|i.s3<<24;}";
	cl_program pg=CL_CHECK_ERR(clCreateProgramWithSource(cx,1,&S,0,&_err));
	if(clBuildProgram(pg,1,&dv,"",0,0)!=CL_SUCCESS){
		char buffer[8192];
		clGetProgramBuildInfo(pg,dv,CL_PROGRAM_BUILD_LOG,sizeof(buffer),buffer,0);
		printf("%s",buffer);
		return 1;
	}
	clUnloadCompiler();
	cl_mem px=CL_CHECK_ERR(clCreateBuffer(cx,CL_MEM_WRITE_ONLY,sizeof(int)*128,0,&_err));
	cl_kernel k=CL_CHECK_ERR(clCreateKernel(pg,"m",&_err));
	cl_command_queue q=CL_CHECK_ERR(clCreateCommandQueue(cx,dv,0,&_err));
	clSetKernelArg(k,0,sizeof(cl_mem),&px);
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
	goto rend;
	for(;;){
		XEvent ev;
		xne:XNextEvent(dpy,&ev);
		switch(ev.type){
		case Expose:;
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
						printf("%d %d %d %d\n",ev.xbutton.x,nx,ev.xbutton.y,ny);
						nx=ev.xbutton.x-nx;
						ny=ev.xbutton.y-ny;
						wh*=(nx-(nx-ny&nx-ny>>sizeof(int)*8-1))/512.;
					}
				}
				rend:printf("%u\n%f\n%f\n%f\n\n",mxi,xx,yy,wh);
				clSetKernelArg(k,2,sizeof(float),&yy);
				clSetKernelArg(k,3,sizeof(float),&wh);
				clSetKernelArg(k,4,sizeof(int),&mxi);
				float x=xx;
				for(int i=0;i<512;i++){
					x+=wh;
					clSetKernelArg(k,1,sizeof(float),&x);
					size_t gws=128;
					CL_CHECK(clEnqueueNDRangeKernel(q,k,1,0,&gws,0,0,0,0));
					clFinish(q);
					CL_CHECK(clEnqueueReadBuffer(q,px,CL_TRUE,0,128*sizeof(int),manor[i],0,0,0));
					glBegin(GL_POINTS);
					for(int j=0;j<512;j++){
						glColor3ubv(C[manor[i][j]]);
						glVertex2i(i,j);
					}
					glEnd();
				}
				glFlush();
			}else if(ev.xbutton.button==Button3){
				nx-=ev.xbutton.x;
				ny-=ev.xbutton.y;
				goto from3;
			}
		}
	}
}