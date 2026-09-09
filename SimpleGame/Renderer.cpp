#include "stdafx.h"
#include "Renderer.h"
#include "RenderShaders.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
void Quad(){glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(-1,-1);glTexCoord2f(1,0);glVertex2f(1,-1);
 glTexCoord2f(1,1);glVertex2f(1,1);glTexCoord2f(0,1);glVertex2f(-1,1);glEnd();}
}
GLuint Renderer::Compile(const char* vs,const char* fs){
 GLuint p=glCreateProgram();
 for(int i=0;i<2;++i){GLuint s=glCreateShader(i?GL_FRAGMENT_SHADER:GL_VERTEX_SHADER);
 const char* source=i?fs:vs;glShaderSource(s,1,&source,nullptr);glCompileShader(s);
 GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
 if(!ok){char log[4096]={};glGetShaderInfoLog(s,4096,nullptr,log);error=log;glDeleteShader(s);glDeleteProgram(p);return 0;}
 glAttachShader(p,s);glDeleteShader(s);}
 glLinkProgram(p);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);
 if(!ok){char log[4096]={};glGetProgramInfoLog(p,4096,nullptr,log);error=log;glDeleteProgram(p);return 0;}return p;
}
Renderer::Renderer(int w,int h){
 if(!GLEW_VERSION_3_3){error="OpenGL 3.3 compatibility context required";return;}
 worldProgram=Compile(RenderShaders::WorldVS,RenderShaders::WorldFS);
 postProgram=Compile(RenderShaders::PostVS,RenderShaders::PostFS);
 if(!worldProgram||!postProgram)return;
 glUseProgram(worldProgram);glUniform1i(glGetUniformLocation(worldProgram,"materialTex"),0);
 glUniform1i(glGetUniformLocation(worldProgram,"shadowTex"),1);
 materialLocation=glGetUniformLocation(worldProgram,"textured");emissionLocation=glGetUniformLocation(worldProgram,"emission");glUseProgram(0);
 glGenFramebuffers(1,&shadowFbo);glGenTextures(1,&shadowDepth);glBindTexture(GL_TEXTURE_2D,shadowDepth);
 glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,2048,2048,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 glBindFramebuffer(GL_FRAMEBUFFER,shadowFbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,shadowDepth,0);
 glDrawBuffer(GL_NONE);glReadBuffer(GL_NONE);
 if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){error="Shadow framebuffer unavailable";glBindFramebuffer(GL_FRAMEBUFFER,0);return;}
 glBindFramebuffer(GL_FRAMEBUFFER,0);MakeMaterials();
 textDC=CreateCompatibleDC(nullptr);
 font=CreateFontW(-22,0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,HANGEUL_CHARSET,OUT_DEFAULT_PRECIS,
 CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Malgun Gothic");
 if(!textDC||!font){error="Korean font initialization failed";return;}
 previousFont=SelectObject(textDC,font);initialized=true;Resize(w,h);
}
void Renderer::ReleaseTargets(){
 if(sceneFbo)glDeleteFramebuffers(1,&sceneFbo);if(sceneColor)glDeleteTextures(1,&sceneColor);
 if(sceneDepth)glDeleteRenderbuffers(1,&sceneDepth);sceneFbo=sceneColor=sceneDepth=0;
}
Renderer::~Renderer(){
 ReleaseTargets();if(shadowFbo)glDeleteFramebuffers(1,&shadowFbo);if(shadowDepth)glDeleteTextures(1,&shadowDepth);
 if(worldProgram)glDeleteProgram(worldProgram);if(postProgram)glDeleteProgram(postProgram);
 glDeleteTextures(MaterialCount,materials);for(auto& p:labels)glDeleteTextures(1,&p.second.texture);
 if(textDC&&previousFont)SelectObject(textDC,previousFont);if(font)DeleteObject(font);if(textDC)DeleteDC(textDC);
}
void Renderer::Resize(int w,int h){
 w=(std::max)(1,w);h=(std::max)(1,h);if(sceneFbo&&width==w&&height==h)return;
 width=w;height=h;ReleaseTargets();
 glGenTextures(1,&sceneColor);glBindTexture(GL_TEXTURE_2D,sceneColor);
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,width,height,0,GL_RGBA,GL_FLOAT,nullptr);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 glGenRenderbuffers(1,&sceneDepth);glBindRenderbuffer(GL_RENDERBUFFER,sceneDepth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,width,height);
 glGenFramebuffers(1,&sceneFbo);glBindFramebuffer(GL_FRAMEBUFFER,sceneFbo);
 glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,sceneColor,0);
 glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,sceneDepth);
 glDrawBuffer(GL_COLOR_ATTACHMENT0);glReadBuffer(GL_COLOR_ATTACHMENT0);
 if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){initialized=false;error="Scene framebuffer unavailable";}
 glBindFramebuffer(GL_FRAMEBUFFER,0);
}
void Renderer::MakeMaterials(){
 glGenTextures(MaterialCount,materials);
 for(int m=0;m<MaterialCount;++m){std::vector<unsigned char> data(128*128*3);
 for(int y=0;y<128;++y)for(int x=0;x<128;++x){
 unsigned hash=(unsigned(x)*73856093u)^(unsigned(y)*19349663u);float noise=float(hash%31)/255.f;
 float r=.52f,g=.64f,b=.35f;
 if(m==Grass){r+=noise;g+=noise;b+=noise*.5f;}
 if(m==Stone){int row=y/24,xx=(x+(row%2)*16)%32;bool mortar=xx<2||y%24<2;
 float tone=mortar?.28f:.59f+float((x/32+y/24)%3)*.04f+noise;r=tone;g=tone*.94f;b=tone*.79f;}
 if(m==Wood){float grain=.05f*std::sin(x*.8f+std::sin(y*.09f)*2);r=.52f+grain+noise;g=.32f+grain;b=.17f+grain;
 if(x%32<2){r*=.6f;g*=.6f;b*=.6f;}}
 if(m==Roof){bool seam=y%20<2||(x+(y/20%2)*12)%24<2;float v=seam?.6f:1.f;r=(.64f+noise)*v;g=(.32f+noise)*v;b=(.24f+noise)*v;}
 int i=(y*128+x)*3;data[i]=static_cast<unsigned char>(r*255);data[i+1]=static_cast<unsigned char>(g*255);data[i+2]=static_cast<unsigned char>(b*255);}
 glBindTexture(GL_TEXTURE_2D,materials[m]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,128,128,0,GL_RGB,GL_UNSIGNED_BYTE,data.data());glGenerateMipmap(GL_TEXTURE_2D);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);}
 glBindTexture(GL_TEXTURE_2D,0);
}
void Renderer::BeginShadow(float x,float z){
 shadowPass=true;glUseProgram(0);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,0);glActiveTexture(GL_TEXTURE0);
 glBindFramebuffer(GL_FRAMEBUFFER,shadowFbo);glViewport(0,0,2048,2048);glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
 glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glDisable(GL_LIGHTING);glDisable(GL_TEXTURE_2D);
 glClear(GL_DEPTH_BUFFER_BIT);glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(2,4);
 glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(-42,42,-42,42,-100,100);glGetFloatv(GL_PROJECTION_MATRIX,lightProjection);
 glMatrixMode(GL_MODELVIEW);glLoadIdentity();glRotatef(55,1,0,0);glRotatef(-35,0,1,0);glTranslatef(-x,0,-z);glGetFloatv(GL_MODELVIEW_MATRIX,lightView);
}
void Renderer::BeginScene(float x,float z){
 shadowPass=false;glDisable(GL_POLYGON_OFFSET_FILL);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
 glBindFramebuffer(GL_FRAMEBUFFER,sceneFbo);glViewport(0,0,width,height);glClearColor(.50f,.70f,.80f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
 glMatrixMode(GL_PROJECTION);glLoadIdentity();float a=float(width)/height;glOrtho(-11*a,11*a,-11,11,-100,100);
 glMatrixMode(GL_MODELVIEW);glLoadIdentity();glRotatef(35.264f,1,0,0);glRotatef(45,0,1,0);glTranslatef(-x,0,-z);
 // bias * light projection * light view * inverse camera view.
 glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,shadowDepth);glMatrixMode(GL_TEXTURE);glLoadIdentity();
 glTranslatef(.5f,.5f,.5f);glScalef(.5f,.5f,.5f);glMultMatrixf(lightProjection);glMultMatrixf(lightView);
 glTranslatef(x,0,z);glRotatef(-45,0,1,0);glRotatef(-35.264f,1,0,0);
 glActiveTexture(GL_TEXTURE0);glMatrixMode(GL_MODELVIEW);
 // Match the direction used by the light camera's inverse rotation.
 GLfloat sun[]={.329f,.819f,.470f,0};glLightfv(GL_LIGHT0,GL_POSITION,sun);
 glUseProgram(worldProgram);MaterialMode();
}
void Renderer::MaterialMode(int material,float emission){
 if(shadowPass)return;
 glUniform1i(materialLocation,material>=0?1:0);glUniform1f(emissionLocation,emission);
 if(material>=0&&material<MaterialCount){glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,materials[material]);}
}
void Renderer::EndScene(){
 glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);
 glUseProgram(postProgram);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,sceneColor);
 glUniform1i(glGetUniformLocation(postProgram,"scene"),0);glUniform2f(glGetUniformLocation(postProgram,"texel"),1.f/width,1.f/height);
 Quad();glUseProgram(0);glBindTexture(GL_TEXTURE_2D,0);
}
void Renderer::BeginUI(){
 glUseProgram(0);glActiveTexture(GL_TEXTURE0);glDisable(GL_TEXTURE_2D);glDisable(GL_LIGHTING);glDisable(GL_DEPTH_TEST);
 glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,1280,800,0,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
}
void Renderer::Text(float x,float baseline,const std::wstring& value,float r,float g,float b){
 if(value.empty()||!textDC)return;
 auto found=labels.find(value);
 if(found==labels.end()){
  if(labels.size()>160){for(auto& p:labels)glDeleteTextures(1,&p.second.texture);labels.clear();}
  SIZE extent={};GetTextExtentPoint32W(textDC,value.c_str(),int(value.size()),&extent);
  Label label;label.width=(std::max)(1,static_cast<int>(extent.cx)+4);label.height=32;
  BITMAPINFO info={};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=label.width;
  info.bmiHeader.biHeight=-label.height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
  void* bits=nullptr;HBITMAP bitmap=CreateDIBSection(textDC,&info,DIB_RGB_COLORS,&bits,nullptr,0);
  if(!bitmap||!bits)return;HGDIOBJ previous=SelectObject(textDC,bitmap);
  std::fill_n(static_cast<unsigned char*>(bits),label.width*label.height*4,0);
  SetBkMode(textDC,TRANSPARENT);SetTextColor(textDC,RGB(255,255,255));TextOutW(textDC,2,0,value.c_str(),int(value.size()));GdiFlush();
  unsigned char* pixels=static_cast<unsigned char*>(bits);
  for(int i=0;i<label.width*label.height;++i){unsigned char alpha=pixels[i*4];pixels[i*4]=pixels[i*4+1]=pixels[i*4+2]=255;pixels[i*4+3]=alpha;}
  glGenTextures(1,&label.texture);glBindTexture(GL_TEXTURE_2D,label.texture);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,label.width,label.height,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  SelectObject(textDC,previous);DeleteObject(bitmap);found=labels.emplace(value,label).first;
 }
 const Label& l=found->second;glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,l.texture);glColor4f(r,g,b,1);
 glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(x,baseline-23);glTexCoord2f(1,0);glVertex2f(x+l.width,baseline-23);
 glTexCoord2f(1,1);glVertex2f(x+l.width,baseline-23+l.height);glTexCoord2f(0,1);glVertex2f(x,baseline-23+l.height);glEnd();glDisable(GL_TEXTURE_2D);
}
void Renderer::DrawSolidRect(float x,float y,float z,float size,float r,float g,float b,float a){
 glColor4f(r,g,b,a);glBegin(GL_QUADS);glVertex3f(x-size,y-size,z);glVertex3f(x+size,y-size,z);glVertex3f(x+size,y+size,z);glVertex3f(x-size,y+size,z);glEnd();
}
