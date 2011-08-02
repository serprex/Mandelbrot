#include <CL/cl.h>
#include <stdio.h>
#include <GL/glx.h>
int main(int argc,char**argv){
	const size_t gws=128;
	float xx=-2,yy=-2,wh=1/128.;
	unsigned char manor[512][512];
	int nx,ny,mans,mxi=300;
	unsigned char C[256][3];
	for(int i=0;i<256;i++){
		C[i][0]=i*i*i>>16;
		C[i][1]=i*i>>8;
		C[i][2]=i;
	}
	cl_platform_id pl;
	cl_device_id dv;
	cl_uint pls,dvs;
	clGetPlatformIDs(1,&pl,&pls);
	if(!pls)return printf("No platforms");
	clGetDeviceIDs(pl,CL_DEVICE_TYPE_GPU,1,&dv,&dvs);
	if(!dvs)return printf("No devices");
	cl_context cx=clCreateContext(0,1,&dv,0,0,0);
	const char*S="__kernel void m(__global int*dst,const float x,const float y,const float w,const int t){const int g=get_global_id(0),G=g*4;const float4 Y=y+w*(float4)(G,G+1,G+2,G+3);float4 a=x,b=Y,c=a*a,d=b*b;int4 i=t-1;for(;;){b=a*b*2+Y;a=c-d+x;c=a*a;d=b*b;const int4 n=(c+d<4)&(i>0);if(!any(n))break;i-=n&1;}i=(i<<8)/t;dst[g]=i.s0|i.s1<<8|i.s2<<16|i.s3<<24;}";
	cl_int _err;
	cl_program pg=clCreateProgramWithSource(cx,1,&S,0,0);
	if(clBuildProgram(pg,1,&dv,"",0,0)!=CL_SUCCESS){
		char buffer[8192];
		clGetProgramBuildInfo(pg,dv,CL_PROGRAM_BUILD_LOG,8192,buffer,0);
		return printf("%s",buffer);
	}
	clUnloadCompiler();
	cl_kernel k=clCreateKernel(pg,"m",0);
	cl_command_queue q=clCreateCommandQueue(cx,dv,0,0);
	cl_mem px=clCreateBuffer(cx,CL_MEM_WRITE_ONLY,512,0,0);
	clSetKernelArg(k,0,sizeof(cl_mem),&px);
	clSetKernelArg(k,4,4,&mxi);
	Display*dpy=XOpenDisplay(0);
	XVisualInfo*vi=glXChooseVisual(dpy,DefaultScreen(dpy),(int[]){GLX_RGBA,None});
	Window win=XCreateWindow(dpy,RootWindow(dpy,vi->screen),0,0,511,511,0,vi->depth,InputOutput,vi->visual,CWColormap|CWEventMask,(XSetWindowAttributes[]){{.colormap=XCreateColormap(dpy,RootWindow(dpy,vi->screen),vi->visual,AllocNone),.event_mask=ExposureMask|ButtonPressMask|ButtonReleaseMask}});
	XMapWindow(dpy,win);
	glXMakeCurrent(dpy,win,glXCreateContext(dpy,vi,0,GL_TRUE));
	glOrtho(0,511,511,0,-1,1);
	goto rend;
	for(;;){
		XEvent ev;
		if(XPending(dpy)||mans==512){
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
				break;case Button4:case Button5:
					mxi+=ev.xbutton.button==Button4?25:mxi>25?-25:0;
					clSetKernelArg(k,4,4,&mxi);
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
					mans=0;
					clSetKernelArg(k,1,4,&xx);
					clSetKernelArg(k,2,4,&yy);
					clSetKernelArg(k,3,4,&wh);
					clEnqueueNDRangeKernel(q,k,1,0,&gws,0,0,0,0);
				}else if(ev.xbutton.button==Button3){
					nx-=ev.xbutton.x;
					ny-=ev.xbutton.y;
					goto from3;
				}
			}
		}
		if(mans!=512){
			clFinish(q);
			clEnqueueReadBuffer(q,px,CL_TRUE,0,512,manor[mans],0,0,0);
			glBegin(GL_POINTS);
			for(int j=0;j<512;j++){
				glColor3ubv(C[manor[mans][j]]);
				glVertex2i(mans,j);
			}
			glEnd();
			if(++mans==512)glFlush();
			else{
				float x=xx+mans*wh;
				clSetKernelArg(k,1,4,&x);
				clEnqueueNDRangeKernel(q,k,1,0,&gws,0,0,0,0);
			}
		}
	}
}