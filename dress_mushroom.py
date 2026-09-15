import json
import math
from pathlib import Path

root=Path('game/assets/models/mushroom_dungeon')
prefab_path=Path('game/assets/prefabs/mushroom_dungeon.prefab.json')
palette=json.loads((root/'decks.json').read_text())['materials']
palette += [dict(palette[1],albedo=[.25,.13,.09]),dict(palette[1],albedo=[.78,.54,.23]),
            dict(palette[2],albedo=[.19,.12,.30])]

def write(p,data):
    p.write_bytes((json.dumps(data,indent=4)+'\n').replace('\n','\r\n').encode())
def shape(mesh,p,s,m=1,r=(0,0,0)):
    return dict(meshType=mesh,position=list(p),scale=list(s),rotation=list(r),color=[1,1,1,1],materialIndex=m)
def model(name,parts,lights=None):
    d=dict(name='mushroom_dungeon_'+name,materials=palette,shapes=parts)
    if lights: d['lights']=lights
    write(root/(name+'.json'),d)
def radial(r,a,y): return [r*math.sin(a),y,r*math.cos(a)]
def beam(a,b,thickness=.45,height=1.4,material=1):
    v=[b[i]-a[i] for i in range(3)]
    length=math.sqrt(sum(x*x for x in v))
    # Local +Y follows the segment; local Z supplies the near-vertical barrier height.
    rotation=[math.acos(v[1]/length),math.atan2(v[0],v[2]),0]
    return shape('Cube',[(a[i]+b[i])*.5 for i in range(3)],[thickness,length+.02,height],material,rotation)

prefab=json.loads(prefab_path.read_text())
for o in prefab['objects']:
    if Path(o['asset']).stem=='arena_beacon':
        o['asset']='../models/mushroom_dungeon/roof_lantern.json'
new_names={'joinery','bench','waystone','nursery','alchemy_table','scroll_shelf','hanging_spores',
           'root_portal','herald_banner','royal_throne','arena_mandala','growth_shelf','arena_beacon','floor_roots'}
objects=[o for o in prefab['objects'] if Path(o['asset']).stem not in new_names]
def place(name,p=(0,0,0),scale=(1,1,1),yaw=0,collision=False,lights=False):
    objects.append(dict(asset='../models/mushroom_dungeon/'+name+'.json',position=list(p),scale=list(scale),
                        rotation=[0,yaw,0],generateColliders=collision,generateLights=lights))

# Continuous rails follow the exact path endpoints, including slope and inward drift.
joins=[]
def railing(parts,points,bar_height=1.4,post_spacing=1):
    for a,b in zip(points,points[1:]):
        parts.append(beam([a[0],a[1]+bar_height*.5,a[2]],[b[0],b[1]+bar_height*.5,b[2]],.4,bar_height))
    for p in points[::post_spacing]:
        joins.append(shape('Cylinder',[p[0],p[1]+bar_height*.5,p[2]],[.85,bar_height+.3,.85],6))
        joins.append(shape('IcoSphere',[p[0],p[1]+bar_height+.25,p[2]],[1,.6,1],7))

rails=[]
for level in range(4):
    y=24*level
    gate=math.pi+level*math.pi*1.5
    # One continuous polygonal run, leaving only the intentional entrance gap.
    points=[radial(29.5,gate+.19+i*(math.tau-.38)/32,y) for i in range(33)]
    railing(rails,points,1.4,2)
for level in range(3):
    for r in [31,39]:
        points=[radial(r,math.pi+(level+step/120)*math.pi*1.5,level*24+step*.2) for step in range(8,113,4)]
        railing(rails,points,1.7,3)
model('railings',rails)

roof_rails=[]
for z in [-3.7,3.7]:
    railing(roof_rails,[[-35-step*.5,72+step*18/98,z] for step in range(4,95,4)],1.4,3)
for side in [-1,1]:
    points=[]
    for step in range(8,213,4):
        t=step/220
        points.append(radial(84-44*t+side*3.9,1.5*math.pi+t*1.5*math.pi,90+step*.2))
    railing(roof_rails,points,1.4,3)
model('roof_railings',roof_rails)

arena_rails=[]
railing(arena_rails,[radial(37,math.pi+.17+i*(math.tau-.34)/40,134) for i in range(41)],2,2)
model('roof_balustrade',arena_rails)

# The shrinking roof spiral needs its true tangent, not the angle of a circular stair.
stairs=json.loads((root/'roof_stairs.json').read_text())
for i,part in enumerate(stairs['shapes'][98:]):
    def point(t): return radial(84-44*t,1.5*math.pi+t*1.5*math.pi,0)
    a,b=point(i/220),point((i+1)/220)
    part['rotation'][1]=-math.atan2(b[2]-a[2],b[0]-a[0])
write(root/'roof_stairs.json',stairs)

# Round seam battens close the stalk-panel joints without coplanar overlapping faces.
for i in range(24):
    a=(i+.5)*math.tau/24
    if i in [10,11,12,13]:
        bottom,height=16,74
    elif i in [17,18]:
        bottom,height=0,72
        joins.append(shape('Cylinder',radial(42,a,88),[1.4,4,1.4],6))
    else:
        bottom,height=0,90
    joins.append(shape('Cylinder',radial(42,a,bottom+height*.5),[1.4,height,1.4],6))
model('joinery',joins)
place('joinery')

# Distinct reusable room furnishings, kept away from the main passages.
bench=[shape('Cube',[0,1,0],[5,.5,1.6],6),shape('Cube',[0,2,.65],[5,1.5,.3],6)]
for x in [-1.8,1.8]: bench.append(shape('Cube',[x,.4,0],[.45,.8,1.3],1))
model('bench',bench)
model('waystone',[shape('Cube',[0,.4,0],[2.2,.8,2.2],5),shape('Crystal',[0,2,0],[1.6,3.2,1.6],3),
                  shape('Sphere',[0,.9,0],[2.5,.5,2.5],7)])
nursery=[shape('Cube',[0,.6,0],[5,1.2,3],6),shape('Cube',[0,1.23,0],[4.5,.06,2.5],5)]
for x,z,h in [(-1.5,-.5,1),(0,.6,1.8),(1.4,-.4,1.3)]:
    nursery += [shape('Cylinder',[x,1.3+h*.5,z],[.35,h,.35],1),
                shape('Sphere',[x,1.3+h,z],[h*1.3,.6,h*1.3],2),
                shape('Sphere',[x,1.15+h,z],[h*1.2,.15,h*1.2],3)]
model('nursery',nursery)
table=[shape('Cube',[0,1.8,0],[5,.35,2.5],6)]
for x in [-2,2]:
    for z in [-.8,.8]: table.append(shape('Cube',[x,.85,z],[.4,1.7,.4],1))
for x,h,m in [(-1.5,1.2,3),(0,.8,4),(1.5,1.5,8)]:
    table += [shape('Cylinder',[x,2+h*.5,0],[.8,h,.8],m),shape('Sphere',[x,2+h,0],[.8,.4,.8],m),
              shape('Cylinder',[x,2+h+.25,0],[.4,.4,.4],7)]
model('alchemy_table',table)
shelf=[shape('Cube',[-2,3,0],[.4,6,1.5],6),shape('Cube',[2,3,0],[.4,6,1.5],6),
       shape('Cube',[0,3,.65],[4,6,.2],6)]
for y in [.4,2.3,4.2,6]:
    shelf.append(shape('Cube',[0,y,0],[4,.25,1.5],7))
    if y<6:
        for x in [-1.3,0,1.3]: shelf.append(shape('Cube',[x,y+.75,0],[.65,1.2,.9],1 if x==0 else 8,[0,0,.12 if x>0 else -.07]))
model('scroll_shelf',shelf)
hanging=[]
for x,z,h in [(-1,0,3),(0,1,2),(1,-.4,4)]:
    hanging += [shape('Cube',[x,-h*.5,z],[.12,h,.12],6),shape('Sphere',[x,-h,z],[1.8,1.1,1.8],3)]
model('hanging_spores',hanging)
portal=[]
for side in [-1,1]:
    points=[[side*4,0,0],[side*4.3,4,0],[side*3.5,7,0],[0,8.2,0]]
    portal += [beam(a,b,1.3,1.5,1) for a,b in zip(points,points[1:])]
    portal.append(shape('IcoSphere',[side*3.5,7,0],[2,1.5,2],7))
portal.append(shape('Crystal',[0,7.8,0],[.8,1.2,.8],3))
model('root_portal',portal)
model('herald_banner',[shape('Cylinder',[0,3.5,0],[.22,7,.22],7),shape('Cube',[1.2,4.7,0],[2.4,3,.12],8),
                       shape('Crystal',[0,7.3,0],[.5,1,.5],3),shape('Sphere',[1.2,4.7,-.1],[1.4,.9,.12],7)])
throne=[shape('Cube',[0,.7,0],[9,1.4,5],1),shape('Cube',[0,2,0],[6,1.2,3],8),
        shape('Sphere',[0,6,1.6],[8,9,2.2],2),shape('Sphere',[0,11,1],[15,5,10],2),
        shape('Sphere',[0,9.8,1],[14,.5,9],3)]
for x in [-5,5]:
    throne += [shape('Cylinder',[x,4,1],[1.3,8,1.3],1),shape('Crystal',[x,9,1],[1.4,4,1.4],7)]
model('royal_throne',throne)
growth=[shape('Sphere',[0,1,0],[9,2,5],2),shape('Sphere',[0,.3,-.1],[8.5,.4,4.8],3),
        shape('Sphere',[2,-1,.5],[5,1.5,3],1)]
model('growth_shelf',growth)

beacon=[shape('Cylinder',[0,.7,0],[3,1.4,3],5),shape('Cylinder',[0,3,0],[1.1,5,1.1],1),
        shape('Sphere',[0,5.7,0],[5,2,5],2),shape('Sphere',[0,5.1,0],[4.6,.35,4.6],3),
        shape('Crystal',[0,7,0],[1.4,3,1.4],7)]
light=json.loads((root/'roof_lantern.json').read_text())['lights']
light[0]['position']=[0,5,0]
model('arena_beacon',beacon,light)
for o in objects:
    if Path(o['asset']).stem=='roof_lantern': o['asset']='../models/mushroom_dungeon/arena_beacon.json'

# Reception hall: seating and waystones. Nursery: beds and hanging spores.
for side in [-1,1]:
    place('bench',(side*18,0,-16),yaw=side*math.pi/2,collision=True)
    place('waystone',(side*19,0,18))
    for z in [-17,17]: place('nursery',(side*20,24,z),yaw=side*.12)
    place('alchemy_table',(side*22,48,13),yaw=side*math.pi/2)
    place('scroll_shelf',(side*17,48,-18),yaw=math.pi)
    place('herald_banner',(side*15,72,17),yaw=side*.3)
    place('bench',(side*20,72,-17),yaw=math.pi,collision=True)
for p in [(-17,22.8,-15),(17,22.8,15),(-18,46.8,14),(18,46.8,-14),(-18,70.8,15)]: place('hanging_spores',p)
for level,z in [(0,-25),(1,-22),(2,-24),(3,18)]: place('root_portal',(0,level*24,z))
place('root_portal',(0,134,-32))
place('royal_throne',(0,134,32))
for x in [-8,8]: place('herald_banner',(x,134,31),scale=(1.3,1.3,1.3))
for i,(a,y) in enumerate([(0.5,18),(2,35),(3.6,52),(5.5,30),(.1,70),(2.5,62)]):
    place('growth_shelf',radial(43,a,y),scale=(1+(i%3)*.2,1,1),yaw=a)

# A ritual floor with a central mushroom sigil and radial paths, not a plain square outline.
objects=[o for o in objects if not (Path(o['asset']).stem=='inlay' and o['position'][1]==134)]
mandala=[shape('Disc',[0,.025,0],[22,1,22],8),shape('Sphere',[0,.065,1],[12,.08,6],7),
         shape('Cube',[0,.065,-3],[1.4,.08,5],7)]
for i in range(24):
    a=i*math.tau/24
    mandala.append(shape('Cube',radial(22,a,.05),[.3,.06,3 if i%3 else 5],3,[0,a,0]))
for i in range(12):
    a=i*math.tau/12
    mandala.append(shape('Disc',radial(15,a,.04),[1.3,1,1.3],7))
model('arena_mandala',mandala)
place('arena_mandala',(0,134,0))

# Distinct floor borders give each deck a readable shape without changing its collision surface.
floor_roots=[]
for side in [-1,1]:
    floor_roots.append(shape('Cube',[side*6,.03,0],[.18,.06,30],7))
model('floor_roots',floor_roots)
for level in range(4): place('floor_roots',(0,level*24,0),yaw=level%2*math.pi/2)
prefab['objects']=objects
write(prefab_path,prefab)
print(f'{len(objects)} placed objects, themed furnishings and joined/tangent-aligned structures written.')
