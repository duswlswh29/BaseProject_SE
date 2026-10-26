import bpy, math, random
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(root/'Assets/Willowmere/Source/Willowmere_Modular_Library.blend'))
random.seed(108)
col=bpy.data.collections['Nature_Modules']
for o in list(bpy.data.objects):
    if o.name.startswith(('Pine_Crown','Broadleaf_Crown','Grass_Blade')) or o.get('garden_v3'):
        bpy.data.objects.remove(o,do_unlink=True)
def mesh(name,vertices,faces,collection,mat):
    m=bpy.data.meshes.new(name);m.from_pydata(vertices,[],faces);m.update()
    o=bpy.data.objects.new(name,m);collection.objects.link(o);m.materials.append(bpy.data.materials[mat]);o['garden_v3']=True
    return o
def canopy(name,origin,predicate,step=.13):
    cells=set()
    for x in range(-10,11):
      for y in range(-10,11):
       for z in range(5,26):
        if predicate(x*step,y*step,z*step):cells.add((x,y,z))
    vs=[];fs=[]
    sides=[((1,0,0),[(1,0,0),(1,1,0),(1,1,1),(1,0,1)]),((-1,0,0),[(0,1,0),(0,0,0),(0,0,1),(0,1,1)]),((0,1,0),[(1,1,0),(0,1,0),(0,1,1),(1,1,1)]),((0,-1,0),[(0,0,0),(1,0,0),(1,0,1),(0,0,1)]),((0,0,1),[(0,0,1),(1,0,1),(1,1,1),(0,1,1)]),((0,0,-1),[(0,1,0),(1,1,0),(1,0,0),(0,0,0)])]
    for x,y,z in sorted(cells):
      for d,corners in sides:
       if (x+d[0],y+d[1],z+d[2]) not in cells:
        n=len(vs);vs.extend([(origin[0]+(x+a-.5)*step,origin[1]+(y+b-.5)*step,origin[2]+(z+c-.5)*step) for a,b,c in corners]);fs.append(tuple(range(n,n+4)))
    o=mesh(name,vs,fs,col,'leaf');o.data.materials.append(bpy.data.materials['leaf_light'])
    for p in o.data.polygons:p.material_index=1 if p.normal.z>.5 and random.random()<.25 else 0
lobes=[(-.35,-.18,1.85,.65),(.35,.12,1.98,.65),(0,0,2.40,.62),(.08,-.36,2.16,.58)]
canopy('Broadleaf_Crown_Continuous',(7,1,0),lambda x,y,z:any(((x-a)/r)**2+((y-b)/r)**2+((z-c)/(r*.8))**2<1 for a,b,c,r in lobes))
def pine(x,y,z):
    for k in range(6):
        base=.85+k*.32;r=.77-k*.105
        if base<=z<=base+.57 and math.hypot(x,y)<r*(1-(z-base)/.68)*(1+.07*math.sin(math.atan2(y,x)*5+k)):return True
    return False
canopy('Pine_Crown_Continuous',(5,1,0),pine)
# Tapered, bent blade ribbons rather than upright rectangular blocks.
vs=[];fs=[]
for i in range(28):
    x=5.27+random.uniform(-.31,.31);y=-.4+random.uniform(-.24,.24);h=random.uniform(.16,.40);a=random.random()*math.tau;w=.025
    dx,dy=math.cos(a),math.sin(a);n=len(vs)
    vs.extend([(x-dx*w,y-dy*w,0),(x+dx*w,y+dy*w,0),(x+dx*w*.6+dy*.05,y+dy*w*.6-dx*.05,h*.65),(x-dx*w*.6+dy*.05,y-dy*w*.6-dx*.05,h*.65),(x+dy*.12,y-dx*.12,h)])
    fs.extend([(n,n+1,n+2,n+3),(n+3,n+2,n+4)])
mesh('Grass_Blade_Meadow',vs,fs,col,'leaf_light')
arch=bpy.data.collections['Architecture_Assembled_Cottage']
def box(name,p,s,mat):
    bpy.ops.mesh.primitive_cube_add(size=1,location=Vector((-4,2,0))+Vector(p));o=bpy.context.object;o.name=name;o.scale=s
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    for c in list(o.users_collection):c.objects.unlink(o)
    arch.objects.link(o);o.data.materials.append(bpy.data.materials[mat]);o['garden_v3']=True
# Raised side gardens and a back terrace, with uninterrupted front entrance.
for side in [-1,1]:
    box('Garden_Soil',(side*1.43,.25,.13),(.59,2.05,.24),'wood')
    for row in range(2):
      for j in range(8):box('Garden_Stone',(side*1.73,-.67+j*.26,.10+row*.15),(.12,.24,.14),'stone')
    for k in range(3):box('Garden_Hedge',(side*1.43,.21,.38+k*.12),(.43-k*.07,1.90-k*.08,.17),'leaf' if k<2 else 'leaf_light')
    for j in range(10):
      y=-.60+j*.18
      # Overlapping flattened foliage tiers create a continuous hedge silhouette.
      if j%2==0:
       box('Garden_Flower_Stem',(side*1.42,y,.76),(.025,.025,.30),'leaf')
       box('Garden_Flower',(side*1.42,y,.92),(.11,.11,.07),'flower' if j%4 else 'ivory')
box('Terrace_Deck',(0,1.48,.10),(2.60,.72,.18),'wood_light')
for x in [-1.2,1.2]:
 box('Terrace_Post',(x,1.78,.48),(.12,.12,.85),'wood')
for z in [.35,.65]:box('Terrace_Rail',(0,1.78,z),(2.5,.08,.07),'wood')
for i in range(3):box('Garden_Step',(0,1.99+i*.18,.15-i*.04),(.8,.20,.24-i*.08),'stone')
bpy.ops.wm.save_as_mainfile(filepath=str(root/'Assets/Willowmere/Source/Willowmere_Modular_Library.blend'))
scene=bpy.context.scene;scene.cycles.samples=8;scene.render.filepath=str(root/'Assets/Willowmere/Source/Garden_Preview.png');bpy.ops.render.render(write_still=True)
