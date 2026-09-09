#include "stdafx.h"
#include "TutorialLevel.h"
#include "Renderer.h"
#include "Dependencies/freeglut.h"
#include <memory>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
namespace Tutorial {
struct P { float x,z; };
struct C { float r,g,b; };
struct NPC { float x,z; const wchar_t* name; const wchar_t* line; C color; };
static const NPC villagers[]={
 {0,0,L"미라 교수",L"숲의 빛씨앗을 찾아오렴. 과제는 먹으면 안 된단다.",{.5f,.35f,.7f}},
 {-2,4,L"제빵사 핍",L"빵이 또 날아갔어. 오늘도 매출보다 빵이 잘 오르네.",{.8f,.45f,.3f}},
 {2,6,L"학생 리오",L"WASD로 걸어 봐! 나는 첫날 마을을 움직이려 했어.",{.3f,.5f,.7f}},
 {-2,8,L"정원사 페른",L"마을 북서쪽 숲에서 금빛 씨앗을 찾아보렴.",{.3f,.6f,.35f}},
 {3,-2,L"낚시꾼 노리",L"호수가 밤새 노래했어. 물고기들은 음치라던데.",{.3f,.65f,.6f}},
 {-5,-3,L"배달부 베아",L"씨앗을 구하면 호숫가 돌 옆에서 스페이스를 눌러 봐.",{.7f,.45f,.65f}},
 {-3,13,L"대장장이 로엔",L"마법 지팡이도 수리합니다. 빗자루 주차는 옆집이에요.",{.46f,.38f,.31f}},
 {3,15,L"약초상 세라",L"토끼를 따라가도 좋아. 다만 약초는 나눠 먹어야 해.",{.46f,.65f,.46f}},
 {-10,6,L"목수 오린",L"숲 너머까지 길을 냈지. 호수에는 들어가지 말게.",{.61f,.45f,.31f}},
 {11,6,L"견습생 루나",L"등불을 켜는 마법부터 배우래. 용 소환은 다음 학기래!",{.65f,.48f,.78f}},
 {-10,12,L"상인 마르",L"오늘의 특가! 전설의 용 비늘... 모양의 비스킷!",{.78f,.51f,.32f}},
 {8,15,L"순찰대원 도윤",L"사슴과 토끼가 살고 있어. 여우도 보이면 조용히 지켜봐.",{.32f,.46f,.61f}}
};
static const int npcCount=sizeof(villagers)/sizeof(villagers[0]);
static const P houses[]={{-5,3},{5,4},{-5,8},{5,9},{-6,15},{6,19},{-13,8},{13,11}};
static const P campfires[]={{-3,10},{-12,-9},{14,5}};
struct Animal { P position,home; int kind; float phase,heading; };
static std::vector<Animal> animals;
static std::unique_ptr<Renderer> renderer;
static bool moving=false;
static float facing=0;
static const P seed={-8,-6}, stone={4,-4};
static P player={0,3},camera={0,3};
static std::vector<P> trees;
static bool keys[256]={},paused=false;
// 0 meet, 1 collect, 2 cast, 3 befriend, 4 report, 5 complete.
static int stage=0,w=1280,h=800,last=0;
static float elapsed=0,time=0,spell=0,walk=0;
static std::wstring dialog;
static const wchar_t* speaker=L"";
float Dist(P a,P b){float x=a.x-b.x,z=a.z-b.z;return std::sqrt(x*x+z*z);}
void Color(C c){glColor3f(c.r,c.g,c.b);}
void Box(float x,float y,float z,float a,float b,float c,C color){
 Color(color);glPushMatrix();glTranslatef(x,y,z);glScalef(a,b,c);glutSolidCube(1);glPopMatrix();}
void Ball(float x,float y,float z,float r,C c){
 Color(c);glPushMatrix();glTranslatef(x,y,z);glutSolidSphere(r,10,8);glPopMatrix();}
void Cone(float x,float y,float z,float r,float height,C c){
 Color(c);glPushMatrix();glTranslatef(x,y,z);glRotatef(-90,1,0,0);glutSolidCone(r,height,8,1);glPopMatrix();}
void Disk(float x,float y,float z,float rx,float rz,C c){
 Color(c);glBegin(GL_TRIANGLE_FAN);glNormal3f(0,1,0);glVertex3f(x,y,z);
 for(int i=0;i<=48;++i){float a=6.2831853f*i/48;glVertex3f(x+rx*std::cos(a),y,z+rz*std::sin(a));}glEnd();}
void Text(float x,float y,const std::wstring& value,C c={.94f,.94f,.86f}){
 renderer->Text(x,y,value,c.r,c.g,c.b);
}
void Panel(float x,float y,float width,float height){
 glColor4f(.075f,.13f,.18f,.93f);glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+width,y);
 glVertex2f(x+width,y+height);glVertex2f(x,y+height);glEnd();}
void Say(const wchar_t* name,const wchar_t* line){speaker=name;dialog=line;}
bool Blocked(P p);
void Reset(){
 player={0,3};camera=player;stage=0;elapsed=time=spell=walk=0;paused=false;dialog.clear();
 std::fill(keys,keys+256,false);trees.clear();
 for(int x=-27;x<=27;x+=3)for(int z=-25;z<=25;z+=3){P p={float(x),float(z)};
 bool clear=true;for(P house:houses)if(Dist(p,house)<3.6f)clear=false;
 if((x<-16||x>18||z<-9||z>22||(x<-7&&z<0))&&clear&&Dist(p,seed)>2.3f&&Dist(p,{9,-4})>6.f)trees.push_back(p);}
 animals.clear();
 for(int i=0;i<12;++i){
  P p={float(-14+(i%4)*7),float(-15+(i/4)*2)};
  for(int attempt=0;attempt<8&&Blocked(p);++attempt)p.x+=.85f;
  if(!Blocked(p))animals.push_back({p,p,i%3,float(i),0});
 }
 moving=false;facing=0;
 last=glutGet(GLUT_ELAPSED_TIME);
}
bool Blocked(P p){
 if(p.x<-28||p.x>28||p.z<-26||p.z>26)return true;
 float x=(p.x-9)/4.5f,z=(p.z+4)/4.1f;if(x*x+z*z<1)return true;
 for(P b:houses)if(std::fabs(p.x-b.x)<1.9f&&std::fabs(p.z-b.z)<1.7f)return true;
 for(P t:trees)if(Dist(p,t)<.65f)return true;
 for(P f:campfires)if(Dist(p,f)<.7f)return true;return false;
}
P Target(){if(stage==1)return seed;if(stage==2||stage==3)return stone;return {0,0};}
const wchar_t* Objective(){static const wchar_t* lines[]={
 L"01 / 마을의 미라 교수와 대화하기 [E]",
 L"02 / 숲의 빛씨앗 채집하기 [E]",
 L"03 / 호숫가 돌 옆에서 빛 마법 사용하기 [스페이스]",
 L"04 / 깨어난 작은 생물과 교감하기 [E]",
 L"05 / 미라 교수에게 돌아가 보고하기 [E]",
 L"완료 / 새로운 친구, 새로운 수수께끼"};return lines[stage];}
void Interact(){
 if(!dialog.empty()){dialog.clear();return;}
 if(stage==1&&Dist(player,seed)<1.8f){stage=2;Say(L"빛씨앗",L"햇살처럼 따뜻하다! 호숫가 돌 옆에서 스페이스로 빛 마법을 사용하자.");return;}
 if(stage==3&&Dist(player,stone)<2){stage=4;Say(L"작은 생물",L"뀨! 빛씨앗을 꿀꺽 삼킨 생물이 당신의 뒤를 졸졸 따라온다.");return;}
 int nearest=-1;float best=1.9f;
 for(int i=0;i<npcCount;++i){float d=Dist(player,{villagers[i].x,villagers[i].z});if(d<best){best=d;nearest=i;}}
 if(nearest<0)return;const NPC& n=villagers[nearest];
 if(nearest==0&&stage==0){stage=1;Say(n.name,n.line);}
 else if(nearest==0&&stage==4){stage=5;Say(n.name,L"용이라고? 그럴 리가… 입학을 환영한다. 그 도마뱀도 데려오렴.");}
 else Say(n.name,n.line);
}
void KeyDown(unsigned char k,int,int){
 if(k>='A'&&k<='Z')k=static_cast<unsigned char>(k-'A'+'a');
 if(k==27){paused=!paused;std::fill(keys,keys+256,false);return;}
 if(k=='r'){Reset();return;}if(paused||keys[k])return;keys[k]=true;
 if(k=='e')Interact();
 if(k==' '&&dialog.empty()){spell=1;if(stage==2&&Dist(player,stone)<2){stage=3;Say(L"호숫가 돌",L"빛에 응답한 문양이 깨어났다. 돌 아래에서 작은 울음소리가 들린다…");}}
}
void KeyUp(unsigned char k,int,int){if(k>='A'&&k<='Z')k=static_cast<unsigned char>(k-'A'+'a');keys[k]=false;}
void Visibility(int state){if(state!=GLUT_VISIBLE){paused=true;std::fill(keys,keys+256,false);}}
void Tick(int){
 int now=glutGet(GLUT_ELAPSED_TIME);float dt=(std::min)((now-last)/1000.f,.05f);last=now;
 if(!paused){time+=dt;spell=(std::max)(0.f,spell-dt);if(stage<5)elapsed+=dt;
 moving=false;
 if(dialog.empty()){
 float sx=float(keys['d'])-float(keys['a']),sy=float(keys['w'])-float(keys['s']);float len=std::sqrt(sx*sx+sy*sy);
 if(len>0){float dx=(sx-sy)*.70710678f/len*3.2f*dt,dz=(-sx-sy)*.70710678f/len*3.2f*dt;
 moving=true;facing=std::atan2(dx,dz)*180.f/3.14159265f;
 P next={player.x+dx,player.z};if(!Blocked(next))player=next;
 next={player.x,player.z+dz};if(!Blocked(next))player=next;walk+=dt*10;}}
 for(Animal& animal:animals){
 animal.phase+=dt*3;
 P desired={animal.home.x+std::sin(time*.3f+animal.home.x)*2,animal.home.z+std::cos(time*.27f+animal.home.z)*2};
 float ax=desired.x-animal.position.x,az=desired.z-animal.position.z;
 if(Dist(animal.position,player)<3){ax=animal.position.x-player.x;az=animal.position.z-player.z;}
 float length=std::sqrt(ax*ax+az*az);
 if(length>.05f){float speed=animal.kind==1?1.5f:1.f;P next={animal.position.x+ax/length*speed*dt,animal.position.z+az/length*speed*dt};
 if(!Blocked(next)){animal.position=next;animal.heading=std::atan2(ax,az)*180.f/3.14159265f;}}
 }
 float b=1-std::exp(-5*dt);camera.x+=(player.x-camera.x)*b;camera.z+=(player.z-camera.z)*b;}
 glutPostRedisplay();glutTimerFunc(16,Tick,0);
}
void Resize(int width,int height){w=(std::max)(width,1);h=(std::max)(height,1);if(renderer)renderer->Resize(w,h);}

void Ground(float x,float z,float sx,float sz,int material){
 renderer->MaterialMode(material);Color({1,1,1});glBegin(GL_QUADS);glNormal3f(0,1,0);
 float y=material==Renderer::Grass?-.035f:.025f;
 glTexCoord2f(0,0);glVertex3f(x-sx/2,y,z-sz/2);
 glTexCoord2f(0,sz/2);glVertex3f(x-sx/2,y,z+sz/2);
 glTexCoord2f(sx/2,sz/2);glVertex3f(x+sx/2,y,z+sz/2);
 glTexCoord2f(sx/2,0);glVertex3f(x+sx/2,y,z-sz/2);glEnd();renderer->MaterialMode();
}
void Roof(P b){
 renderer->MaterialMode(Renderer::Roof);Color({1,1,1});
 glBegin(GL_QUADS);
 glNormal3f(-.70f,.71f,0);
 glTexCoord2f(0,0);glVertex3f(b.x-1.85f,1.85f,b.z-1.65f);
 glTexCoord2f(0,2);glVertex3f(b.x-1.85f,1.85f,b.z+1.65f);
 glTexCoord2f(2,2);glVertex3f(b.x,3.6f,b.z+1.65f);
 glTexCoord2f(2,0);glVertex3f(b.x,3.6f,b.z-1.65f);
 glNormal3f(.70f,.71f,0);
 glTexCoord2f(0,0);glVertex3f(b.x,3.6f,b.z-1.65f);
 glTexCoord2f(0,2);glVertex3f(b.x,3.6f,b.z+1.65f);
 glTexCoord2f(2,2);glVertex3f(b.x+1.85f,1.85f,b.z+1.65f);
 glTexCoord2f(2,0);glVertex3f(b.x+1.85f,1.85f,b.z-1.65f);glEnd();
 renderer->MaterialMode();Color({.85f,.73f,.54f});
 glBegin(GL_TRIANGLES);glNormal3f(0,0,1);glVertex3f(b.x-1.5f,1.8f,b.z+1.3f);glVertex3f(b.x+1.5f,1.8f,b.z+1.3f);glVertex3f(b.x,3.4f,b.z+1.3f);
 glNormal3f(0,0,-1);glVertex3f(b.x+1.5f,1.8f,b.z-1.3f);glVertex3f(b.x-1.5f,1.8f,b.z-1.3f);glVertex3f(b.x,3.4f,b.z-1.3f);glEnd();
}
void WoodenDoor(P b){
 renderer->MaterialMode(Renderer::Wood);Color({1,1,1});
 glBegin(GL_QUADS);glNormal3f(0,0,1);
 glTexCoord2f(0,0);glVertex3f(b.x-.32f,.03f,b.z+1.40f);
 glTexCoord2f(1,0);glVertex3f(b.x+.32f,.03f,b.z+1.40f);
 glTexCoord2f(1,1);glVertex3f(b.x+.32f,1.17f,b.z+1.40f);
 glTexCoord2f(0,1);glVertex3f(b.x-.32f,1.17f,b.z+1.40f);
 glEnd();renderer->MaterialMode();
}
void Person(float x,float z,C coat,bool student=false){
 float cycle=student?(moving?walk:0):std::sin(time+x)*.15f;
 glPushMatrix();glTranslatef(x,0,z);glRotatef(student?facing:float(int(x*9)%70),0,1,0);
 for(int side=-1;side<=1;side+=2){
  glPushMatrix();glTranslatef(side*.14f,.6f,0);glRotatef(std::sin(cycle)*24*side,1,0,0);
  Box(0,-.23f,0,.18f,.46f,.20f,{.22f,.22f,.30f});Box(0,-.48f,.065f,.22f,.16f,.34f,{.23f,.15f,.11f});glPopMatrix();
  glPushMatrix();glTranslatef(side*.32f,1.12f,0);glRotatef(-std::sin(cycle)*23*side,1,0,0);
  Box(0,-.17f,0,.18f,.39f,.22f,coat);Ball(0,-.41f,0,.105f,{.96f,.73f,.51f});glPopMatrix();
 }
 Box(0,.88f,0,.52f,.61f,.34f,coat);Box(0,.68f,.02f,.56f,.085f,.38f,{.33f,.21f,.12f});
 Box(0,.7f,.22f,.09f,.10f,.04f,{.95f,.75f,.30f});
 Ball(0,1.4f,0,.255f,{.98f,.78f,.56f});Ball(0,1.54f,-.055f,.23f,{.29f,.18f,.13f});
 Ball(-.09f,1.43f,.22f,.035f,{.13f,.16f,.19f});Ball(.09f,1.43f,.22f,.035f,{.13f,.16f,.19f});
 Ball(0,1.35f,.24f,.048f,{.89f,.57f,.40f});
 if(student){
  Cone(0,1.61f,0,.38f,.61f,{.34f,.27f,.57f});
  Box(0,1.13f,-.22f,.50f,.62f,.07f,{.30f,.26f,.49f});
  Box(.44f,.77f,.08f,.045f,.82f,.045f,{.43f,.25f,.13f});
  renderer->MaterialMode(-1,.8f);Ball(.44f,1.21f,.08f,.08f,{1,.80f,.30f});renderer->MaterialMode();
 }else{
  Cone(0,1.59f,0,.30f,.14f,coat);
  Box(.21f,.8f,-.18f,.18f,.22f,.16f,{.42f,.27f,.16f});
 }
 glPopMatrix();
}
void Creature(float x,float z){
 Ball(x,.42f,z,.25f,{.55f,.77f,.85f});Ball(x,.66f,z-.15f,.20f,{.69f,.87f,.91f});
 for(int side=-1;side<=1;side+=2){glPushMatrix();glTranslatef(x+side*.2f,.5f,z);glRotatef(side*(20+std::sin(time*6)*18),0,0,1);
 Cone(0,0,0,.17f,.4f,{.67f,.52f,.82f});glPopMatrix();}
 Ball(x-.08f,.7f,z-.32f,.035f,{.12f,.19f,.25f});Ball(x+.08f,.7f,z-.32f,.035f,{.12f,.19f,.25f});
}
void Wildlife(const Animal& a){
 glPushMatrix();glTranslatef(a.position.x,0,a.position.z);glRotatef(a.heading,0,1,0);
 float scale=a.kind==0?1.f:(a.kind==1?.5f:.72f);glScalef(scale,scale,scale);
 C fur=a.kind==0?C{.59f,.40f,.23f}:(a.kind==1?C{.82f,.79f,.67f}:C{.83f,.39f,.16f});
 glPushMatrix();glTranslatef(0,.62f,0);glScalef(.7f,.8f,1.35f);Ball(0,0,0,.52f,fur);glPopMatrix();
 for(int i=0;i<4;++i){float side=i%2?1.f:-1.f,front=i/2?1.f:-1.f;
 Box(side*.23f,.27f,front*.34f+std::sin(a.phase+float(i))* .06f,.105f,.5f,.12f,fur);}
 Ball(0,.88f,.54f,.25f,fur);Ball(0,.81f,.77f,.13f,a.kind==1?C{.93f,.87f,.79f}:fur);
 for(int side=-1;side<=1;side+=2){
 Cone(side*.14f,1.04f,.5f,.08f,a.kind==1?.48f:.22f,fur);Ball(side*.16f,.94f,.70f,.035f,{.10f,.12f,.12f});
 if(a.kind==0){Box(side*.19f,1.32f,.42f,.05f,.5f,.05f,{.39f,.29f,.18f});Box(side*.26f,1.46f,.42f,.17f,.05f,.05f,{.39f,.29f,.18f});}}
 if(a.kind==2){glPushMatrix();glTranslatef(0,.53f,-.5f);glRotatef(-45,1,0,0);Cone(0,0,0,.18f,.7f,fur);glPopMatrix();}
 glPopMatrix();
}
void OpaqueWorld(){
 for(P b:houses){
  Box(b.x,.9f,b.z,3,1.8f,2.6f,{.94f,.84f,.65f});Roof(b);
  for(int side=-1;side<=1;side+=2){
   Box(b.x+side*1.43f,.93f,b.z+1.34f,.13f,1.86f,.12f,{.31f,.21f,.14f});
   Box(b.x+side*.87f,1.1f,b.z+1.34f,.61f,.68f,.08f,{.31f,.21f,.14f});
   renderer->MaterialMode(-1,.8f);Box(b.x+side*.87f,1.1f,b.z+1.39f,.46f,.53f,.025f,{1,.69f,.26f});renderer->MaterialMode();
   Box(b.x+side*.87f,1.1f,b.z+1.42f,.05f,.55f,.03f,{.33f,.22f,.14f});
  }
  Box(b.x,1.76f,b.z+1.36f,3,.12f,.13f,{.31f,.21f,.14f});
  Box(b.x,.59f,b.z+1.34f,.68f,1.18f,.09f,{.39f,.25f,.13f});
  WoodenDoor(b);
  Ball(b.x+.22f,.62f,b.z+1.42f,.045f,{.88f,.66f,.25f});
  Box(b.x,0,b.z+1.6f,1.1f,.16f,.7f,{.53f,.53f,.45f});
  Box(b.x+1,2.6f,b.z-.5f,.42f,1.5f,.45f,{.55f,.48f,.40f});
 }
 for(P t:trees){
  Box(t.x,1,t.z,.37f,2,.37f,{.36f,.25f,.13f});
  Cone(t.x,1,t.z,1.13f,2.2f,{.23f,.44f,.28f});Cone(t.x,2.1f,t.z,.87f,1.9f,{.34f,.56f,.31f});
 }
 for(const NPC& n:villagers)Person(n.x,n.z,n.color);
 for(const Animal& a:animals)Wildlife(a);
 for(P f:campfires){
  for(int i=0;i<8;++i){float a=i*6.2831853f/8;Ball(f.x+std::cos(a)*.47f,.15f,f.z+std::sin(a)*.47f,.17f,{.43f,.43f,.40f});}
  Box(f.x,.14f,f.z,.7f,.14f,.17f,{.25f,.16f,.10f});
  Box(f.x,.2f,f.z,.16f,.14f,.7f,{.25f,.16f,.10f});
 }
 // Timber dock ends above the near shoreline; it is decorative, not a bridge.
 renderer->MaterialMode();
 for(int i=0;i<6;++i)Box(5.1f+i*.24f,.12f,-1.8f,.22f,.12f,1.2f,{.50f,.34f,.18f});
 Box(stone.x,.38f,stone.z,.75f,.76f,.65f,{.57f,.64f,.63f});
 renderer->MaterialMode(-1,stage>=3?1.5f:0);
 Ball(stone.x,.88f,stone.z,.18f,stage>=3?C{.61f,.95f,.96f}:C{.54f,.48f,.74f});
 if(stage<=1){renderer->MaterialMode(-1,1.6f);Ball(seed.x,.6f+std::sin(time*3)*.12f,seed.z,.25f,{1,.83f,.35f});}
 renderer->MaterialMode();
 if(stage==3)Creature(stone.x-.7f,stone.z);if(stage>=4)Creature(player.x-.65f,player.z+.5f);
 Person(player.x,player.z,{.43f,.34f,.72f},true);
}
void Landscape(){
 Ground(0,0,58,54,Renderer::Grass);
 // One non-overlapping road mesh avoids flicker at the intersections.
 renderer->MaterialMode(Renderer::Stone);Color({1,1,1});
 glBegin(GL_QUADS);glNormal3f(0,1,0);
 for(int ix=-56;ix<56;++ix)for(int iz=-52;iz<52;++iz){
  float x=ix*.5f,z=iz*.5f,cx=x+.25f,cz=z+.25f;
  bool road=(std::fabs(cx)<1.5f&&cz>-4&&cz<22)
   ||(cx>-8.5f&&cx<.5f&&cz>-4&&cz<-2)
   ||(cx>-9&&cx<-7&&cz>-7&&cz<-2)
   ||(cx>-8.5f&&cx<5.5f&&std::fabs(cz+6)<.75f)
   ||(cx>3&&cx<5&&cz>-6.5f&&cz<-1.5f)
   ||(cx>0&&cx<4&&std::fabs(cz+2)<.75f)
   ||(std::fabs(cx)<12.5f&&std::fabs(cz-12)<1);
  if(!road)continue;
  glTexCoord2f(x*.5f,z*.5f);glVertex3f(x,.025f,z);
  glTexCoord2f(x*.5f,(z+.5f)*.5f);glVertex3f(x,.025f,z+.5f);
  glTexCoord2f((x+.5f)*.5f,(z+.5f)*.5f);glVertex3f(x+.5f,.025f,z+.5f);
  glTexCoord2f((x+.5f)*.5f,z*.5f);glVertex3f(x+.5f,.025f,z);
 }glEnd();renderer->MaterialMode();
 Disk(9,.035f,-4,4.6f,4.2f,{.77f,.72f,.50f});
 for(int i=0;i<180;++i){float x=float((i*17)%550)/10-27.5f,z=float((i*31)%510)/10-25.5f;
 if(std::fabs(x)>2&&!Blocked({x,z})){
  Cone(x,.03f,z,.09f,.3f,{.37f,.53f,.25f});Ball(x,.3f,z,.06f,i%2?C{1,.84f,.48f}:C{.79f,.61f,.85f});}}
}
void Effects(){
 // Animated water surface with geometric ripples, changing normals and foam.
 renderer->MaterialMode(-1,.18f);
 const int rings=18,segments=64;
 auto waterVertex=[](float radius,float angle){
  float x=9+std::cos(angle)*radius*4.25f,z=-4+std::sin(angle)*radius*3.85f;
  float wave=std::sin(x*2+time*1.4f)*std::cos(z*2.3f+time)*.025f;
  glNormal3f(-.05f*std::cos(x*2+time*1.4f),1,-.05f*std::sin(z*2.3f+time));
  Color({.15f+radius*.12f,.43f+radius*.16f+wave,.58f+radius*.13f});
  glVertex3f(x,.075f+wave,z);
 };
 for(int ring=0;ring<rings;++ring){glBegin(GL_TRIANGLE_STRIP);
  for(int j=0;j<=segments;++j){float a=j*6.2831853f/segments;waterVertex(float(ring)/rings,a);waterVertex(float(ring+1)/rings,a);}glEnd();}
 renderer->MaterialMode(-1,.5f);
 for(int i=0;i<30;++i){float a=i*2.4f+time*.07f,r=1.2f+(i%6)*.44f;Disk(9+std::cos(a)*r,.115f,-4+std::sin(a)*r,.11f,.025f,{.61f,.86f,.86f});}
 glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glDepthMask(GL_FALSE);
 // Flame ribbons and rising embers. Emissive values feed the bloom pass.
 for(P f:campfires){
  renderer->MaterialMode(-1,2.1f);
  for(int i=0;i<9;++i){float life=std::fmod(time*(.65f+i*.015f)+i*.113f,1.f);
   float x=f.x+std::sin(time*4+i)*.12f,z=f.z+std::cos(time*3+i)*.12f;
   Ball(x,.25f+life*.95f,z,(1-life)*.16f+.015f,{1,.28f+life*.3f,.055f});}
  for(int i=0;i<7;++i){float life=std::fmod(time*.4f+i*.14f,1.f);
   Ball(f.x+std::sin(i+time)*life*.45f,.6f+life*1.8f,f.z,.025f*(1-life),{1,.64f,.12f});}
 }
 if(stage<5){P t=Target();renderer->MaterialMode(-1,1.4f);
  Ball(t.x,2.4f+std::sin(time*3)*.12f,t.z,.12f,{1,.85f,.30f});
  for(int i=1;i<=6&&Dist(player,t)>2;++i){float f=float(i)/7;Disk(player.x+(t.x-player.x)*f,.10f,player.z+(t.z-player.z)*f,.065f,.065f,{1,.8f,.37f});}}
 if(spell>0){renderer->MaterialMode(-1,2.3f);
  for(int i=0;i<36;++i){float a=i*6.2831853f/36,r=(1-spell)*2.5f;Ball(player.x+std::cos(a)*r,.35f+spell,player.z+std::sin(a)*r,.055f,{1,.8f,.4f});}}
 glDepthMask(GL_TRUE);glDisable(GL_BLEND);renderer->MaterialMode();
}
void HUD(){
 renderer->BeginUI();
 Panel(24,22,775,116);Text(44,52,L"윌로미어 · 작은 시작",{1,.86f,.55f});Text(44,86,Objective());
 Text(44,119,L"경과 "+std::to_wstring(int(elapsed))+L"초  ·  목표 3~5분  ·  시간 제한 없음",{.67f,.80f,.80f});
 Panel(24,744,1232,38);Text(42,772,L"WASD 이동    E 대화·채집·계속    스페이스 빛 마법    ESC 일시정지    R 다시 시작");
 Panel(1040,22,216,195);Text(1055,50,L"주변 지도",{1,.86f,.55f});Text(1055,78,L"숲          호수");
 auto dot=[](P p,C c){Color(c);glPointSize(6);glBegin(GL_POINTS);glVertex2f(1147+p.x*3.3f,131+p.z*2.2f);glEnd();};
 for(const NPC& n:villagers)dot({n.x,n.z},{.85f,.65f,.4f});
 dot(seed,{.53f,.85f,.48f});dot(stone,{.4f,.79f,1});dot(player,{1,1,1});Text(1055,204,L"흰 점: 내 위치");
 bool isNearby=Dist(player,Target())<1.9f;
 for(const NPC& n:villagers)isNearby=isNearby||Dist(player,{n.x,n.z})<1.9f;
 if(isNearby&&dialog.empty()&&stage<5){Panel(410,677,460,42);Text(430,706,stage==2&&Dist(player,stone)<2?L"[스페이스] 돌의 문양 깨우기":L"[E] 가까운 대상과 상호작용");}
 if(!dialog.empty()){Panel(24,602,1232,127);Text(44,635,speaker,{1,.86f,.55f});Text(44,675,dialog);Text(44,713,L"[E] 계속");}
 if(stage==5&&dialog.empty()){Panel(290,265,700,180);Text(330,312,L"튜토리얼 완료",{1,.86f,.55f});Text(330,350,L"역사책이 잊어버린 작은 친구를 만났습니다.");
  Text(330,387,L"보상: 마법학교 초대장과 배고픈 동행자");Text(330,424,L"계속 탐험하거나 R로 다시 시작하세요.");}
 if(paused){Panel(430,300,420,130);Text(465,350,L"일시정지");Text(465,396,L"ESC 계속하기    R 다시 시작");}
}
bool Initialize(int width,int height){
 renderer.reset(new Renderer(width,height));
 if(!renderer->IsInitialized())OutputDebugStringA(renderer->Error().c_str());
 return renderer->IsInitialized();
}
void Shutdown(){renderer.reset();}
void Render(){
 if(!renderer||!renderer->IsInitialized())return;
 renderer->BeginShadow(camera.x,camera.z);OpaqueWorld();
 renderer->BeginScene(camera.x,camera.z);Landscape();OpaqueWorld();Effects();
 renderer->EndScene();HUD();glutSwapBuffers();
}
}
