import bpy,os,re,struct,json,math
from mathutils import Vector,Matrix
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=root/'Assets/Willowmere/Source/Willowmere_Modular_Library.blend'
bpy.ops.wm.open_mainfile(filepath=str(source))
enum=re.search(r'enum class Model\s*\{(.*?)\};',(root/'ModelCache.h').read_text(),re.S).group(1)
ids={n.strip():i for i,n in enumerate(enum.split(',')) if n.strip()}
pack={}; report={}
def export(name,objects,anchor=(0,0,0),scale=(1,1,1),normalize=False,white=False):
    # Transform the actual evaluated Blender meshes, including their armature poses.
    bpy.context.view_layer.update(); graph=bpy.context.evaluated_depsgraph_get()
    transform=Matrix.Diagonal(Vector((*scale,1))) @ Matrix.Translation(-Vector(anchor))
    coords=[]; faces=[]
    for obj in objects:
        if obj.type!='MESH':continue
        ev=obj.evaluated_get(graph); mesh=ev.to_mesh();mesh.calc_loop_triangles()
        matrix=transform @ obj.matrix_world; normalmatrix=matrix.to_3x3().inverted().transposed()
        for triangle in mesh.loop_triangles:
            mat=obj.data.materials[triangle.material_index] if obj.data.materials else None
            color=tuple(mat.diffuse_color) if mat else (1,1,1,1)
            if white:color=(1,1,1,1)
            norm=(normalmatrix@triangle.normal).normalized()
            materialName = mat.name if mat else ''
            tag = 2 if materialName.startswith('wood') else 3 if materialName=='stone' else 4 if materialName.startswith('leaf') else 5 if materialName=='glass' else 6 if materialName=='ivory' else 7 if materialName=='slate' else 0
            if white: tag=0
            for vi in triangle.vertices:
                v=matrix@mesh.vertices[vi].co
                coords.append((v.x,v.z,-v.y));faces.append(((norm.x,norm.z,-norm.y),color,tag))
        ev.to_mesh_clear()
    assert coords,name
    lo=[min(v[i] for v in coords) for i in range(3)];hi=[max(v[i] for v in coords) for i in range(3)]
    if normalize:
        coords=[tuple((v[i]-(lo[i]+hi[i])/2)/(hi[i]-lo[i]) for i in range(3)) for v in coords]
    data=bytearray()
    for v,(n,c,tag) in zip(coords,faces):
        data+=struct.pack('<12f',*v,*n,*c,v[0],-100.0-tag)
    assert len(coords)<150000 and all(abs(x)<10 for v in coords for x in v)
    pack[ids[name]]=bytes(data);report[name]={'vertices':len(coords),'source_objects':[o.name for o in objects],'bounds_before_normalization':[lo,hi]}
def col(name):return [o for o in bpy.data.collections[name].objects if o.type=='MESH']
def family(collection,idle,first):
    objects=col(collection); rig=next(o for o in bpy.data.collections[collection].objects if o.type=='ARMATURE')
    rig.animation_data.action=bpy.data.actions[rig.name+'__Reference'];bpy.context.scene.frame_set(1)
    export(idle,objects,rig.location)
    rig.animation_data.action=bpy.data.actions[rig.name+'__Walk']
    for i in range(8):
        bpy.context.scene.frame_set(1+int(i*30/8));export(first+str(i),objects,rig.location)
    rig.animation_data.action=bpy.data.actions[rig.name+'__Reference'];bpy.context.scene.frame_set(1)
family('Human_Apprentice','HeroIdle','Hero')
family('Human_Merchant','MerchantIdle','Merchant')
family('Human_Healer','HealerIdle','Healer')
family('Animal_Deer','DeerIdle','Deer')
family('Animal_Wildcat','CatIdle','Cat')
family('Animal_Wolf','WolfIdle','Wolf')
for target,src in [('Slime','CatIdle'),('Boar','WolfIdle')]:pack[ids[target]]=pack[ids[src]]; report[target]={'alias':src}
export('House',col('Architecture_Assembled_Cottage'),(-4,2,0),(1.45,1.20,1.25))
export('Garden',[o for o in col('Architecture_Assembled_Cottage') if o.name.startswith('Garden_') and o.location.x < -5],(-5.43,2.25,0))
props=col('Household_Props')
for name,prefix,pivot in [('Table','Table',(-3,-3,0)),('Stool','Stool',(-2,-3,0)),('Chest','Chest',(-1,-3,0)),('Barrel','Barrel',(0,-3,0)),('Lantern','Lantern',(2.4,-3,0))]:
    export(name,[o for o in props if o.name.startswith(prefix)],pivot)
nature=col('Nature_Modules')
for name,prefix,pivot in [('Tree','Pine',(5,1,0)),('Broadleaf','Broadleaf',(7,1,0)),('GrassTuft','Grass_Blade',(5.27,-.4,0)),('Rock','Rock',(7,-.4,0))]:
    export(name,[o for o in nature if o.name.startswith(prefix)],pivot)
export('WoodBlock',[next(o for o in props if o.name.startswith('Table_Top'))],normalize=True,white=True)
export('StoneBlock',[next(o for o in nature if o.name.startswith('Stone_Tile'))],normalize=True,white=True)
export('LeafCluster',[next(o for o in nature if o.name.startswith('Broadleaf_Crown'))],normalize=True,white=True)
export('Crystal',[next(o for o in nature if o.name.startswith('Rock'))],(7,-.4,0),(.3,.3,.65))
export('Orb',[next(o for o in nature if o.name.startswith('Rock'))],(7,-.4,.24),(.3,.3,.3))
export('Potion',[next(o for o in props if o.name.startswith('Pot'))],(1,-3,0),(.7,.7,.7))
export('Camp',[o for o in props if o.name.startswith('Barrel')],(0,-3,0),(.8,.8,.5))
out=root/'Assets'/'Willowmere';out.mkdir(parents=True,exist_ok=True)
with (out/'meshes.bin').open('wb') as f:
    f.write(struct.pack('<4I',0x574D4B31,1,len(pack),48))
    for index,data in sorted(pack.items()):
        checksum=2166136261
        for b in data:checksum=((checksum^b)*16777619)&0xffffffff
        f.write(struct.pack('<3I',index,len(data)//48,checksum));f.write(data)
(out/'mesh_manifest.json').write_text(json.dumps({'source':source.name,'axis_conversion':'Blender XYZ -> game XZ-Y','animation':'8 baked Walk poses, separate Reference pose; skeletal source retained in blend','meshes':report},ensure_ascii=False,indent=2),encoding='utf-8')
print('EXPORTED',len(pack),'meshes',sum(len(d) for d in pack.values()),'bytes')
