import bpy,math,random
from mathutils import Vector
from pathlib import Path
root=Path(__file__).resolve().parents[1]
path=root/'Assets/Willowmere/Source/Willowmere_Modular_Library.blend'
bpy.ops.wm.open_mainfile(filepath=str(path))
random.seed(47)
for obj in list(bpy.data.objects):
    if obj.get('art_detail_v2'): bpy.data.objects.remove(obj,do_unlink=True)
def cube(name,p,size,mat,col):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.name=name;o.scale=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    for c in list(o.users_collection):c.objects.unlink(o)
    col.objects.link(o);o.data.materials.append(bpy.data.materials[mat]);o['art_detail_v2']=True
    return o
def material(name,color):
    m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
    m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(*color,1)
for name,c in [('wood',(.25,.12,.045)),('wood_light',(.40,.23,.09)),('stone',(.38,.40,.34)),('slate',(.19,.24,.18)),('leaf',(.085,.21,.065)),('leaf_light',(.22,.36,.09)),('ivory',(.73,.68,.51)),('flower',(.61,.12,.09))]:material(name,c)
col=bpy.data.collections['Architecture_Assembled_Cottage'];origin=Vector((-4,2,0))
def hc(n,p,s,m):return cube(n,origin+Vector(p),s,m,col)
# Physical roof shingle courses and end-grain eaves, still a single baked draw.
for side in [-1,1]:
    for row in range(8):
        x=side*(.08+row*.147);z=2.64-abs(x)*.70
        for tile in range(11):
            y=-1.18+tile*.232
            o=hc('Roof_Shingle',(x,y,z),(.19,.224,.055),'slate')
            o.rotation_euler.y=side*math.radians(35)
    hc('Eave_Beam',(side*1.12,0,1.73),(.13,2.48,.17),'wood')
    for y in [-.87,-.43,0,.43,.87]:
        hc('Rafter_Tail',(side*1.08,y,1.68),(.26,.08,.12),'wood_light')
    # Side masonry courses; staggered, muted stone colors and recessed mortar.
    for row in range(4):
        for i in range(7):
            hc('Foundation_Block',(side*1.06,-.88+i*.29,.12+row*.12),(.12,.275,.108),'stone')
    hc('Wall_Rail',(side*1.08,0,.62),(.14,2.06,.10),'wood')
    hc('Wall_Rail',(side*1.08,0,1.74),(.14,2.06,.12),'wood')
    for y in [-.53,.53]:
        for split in [-.11,.11]:hc('Window_Mullion',(side*1.16,y+split,1.22),(.045,.025,.45),'wood')
        hc('Window_Sill',(side*1.19,y,.94),(.23,.59,.08),'wood_light')
        hc('Flower_Box',(side*1.20,y,.83),(.23,.52,.16),'wood')
        for i in range(5):
            hc('Window_Herb',(side*1.22,y-.20+i*.10,.97+random.random()*.10),(.16,.10,.16),'leaf_light')
            if i%2==0:hc('Window_Blossom',(side*1.24,y-.20+i*.10,1.10),(.06,.06,.06),'flower')
    # Rear corner ivy, attached to walls and kept out of doorways.
    for i in range(32):
        z=.35+i*.042; y=.73+math.sin(i*.7)*.13
        hc('Ivy_Leaf',(side*1.11,y,z),(.10,.16,.12),'leaf' if i%2 else 'leaf_light')
for i in range(8):hc('Door_Plank',(-.30+i*.087,-1.093,.82),(.079,.025,1.26),'wood_light')
for x in [-.41,.41]:hc('Door_Jamb',(x,-1.09,.89),(.12,.15,1.48),'wood')
hc('Door_Transom',(0,-1.09,1.58),(.92,.17,.13),'wood')
for x in [-.78,.78]:
    hc('Front_Stud',(x,-1.08,1.05),(.09,.10,1.45),'wood')
for i in range(5):hc('Front_Stone',(-.87+i*.43,-1.07,.22),(.40,.10,.28),'stone')
for y in [-1.08,1.08]:
    hc('Gable_Stud',(0,y,2.07),(.10,.10,.83),'wood')
    hc('Gable_Rail',(0,y,1.78),(2.12,.12,.12),'wood')
for i in range(6):
    hc('Chimney_Course',(.60,.50,1.96+i*.14),(.36,.36,.13),'stone')
# Actual light-colored emissive surfaces (material tag in game export).
col=bpy.data.collections['Nature_Modules']
for obj in list(col.objects):
    if obj.name.startswith(('Pine_Crown','Broadleaf_Crown')) and not obj.get('art_detail_v2'):
        bpy.data.objects.remove(obj,do_unlink=True)
for tier in range(5):
    z=1.05+tier*.38;radius=.73-tier*.12
    for j in range(12):
        angle=j*math.tau/12+tier*.37;r=radius*(.4+.6*random.random())
        cube('Pine_Crown_Block',(5+math.cos(angle)*r,1+math.sin(angle)*r,z+random.uniform(-.10,.12)),
             (.28,.28,.22),'leaf' if j%3 else 'leaf_light',col)
for i in range(58):
    angle=random.random()*math.tau;r=math.sqrt(random.random())*.82
    cube('Broadleaf_Crown_Block',(7+math.cos(angle)*r,1+math.sin(angle)*r,1.75+random.random()*.8),
         (.33,.32,.28),'leaf' if i%3 else 'leaf_light',col)
for i in range(22):
    x=random.uniform(-.08,.58);y=random.uniform(-.22,.22)
    o=cube('Grass_Blade_Detail',(5+x,-.4+y,.12+random.random()*.09),(.035,.018,.25+random.random()*.12),'leaf_light',col)
    o.rotation_euler.y=random.uniform(-.4,.4)
# Coherent preview of the new source; the game uses its own real-time shader.
scene=bpy.context.scene;scene.cycles.samples=32
bpy.ops.wm.save_as_mainfile(filepath=str(path))
scene.render.filepath=str(root/'Assets/Willowmere/Source/Enhanced_Assets_Preview.png')
bpy.ops.render.render(write_still=True)
print('ENRICHED_SOURCE_SAVED')
