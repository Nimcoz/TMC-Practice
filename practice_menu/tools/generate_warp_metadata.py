"""Names + native entries from local room definitions; no guessed safe spawns."""
import argparse
import json
import re
from pathlib import Path


def generate(source, output):
    areas = {n:i for i,n in enumerate(re.findall(r'^\s+(AREA_\w+),', (source/'include/area.h').read_text(), re.M))}
    rooms, entries, group, index = {}, {}, None, -1
    for line in (source/'include/roomid.h').read_text().splitlines():
        m = re.search(r'// (AREA_\w+)',line)
        if m: group = m[1]
        m = re.search(r'^\s+(ROOM_\w+)(?:\s*=\s*(\w+))?,',line)
        if not m or group not in areas: continue
        name, explicit = m.groups()
        index = int(explicit,0) if explicit else index+1
        tail = name.removeprefix('ROOM_'+group.removeprefix('AREA_')+'_')
        label = 'UNNAMED ROOM' if re.fullmatch(r'[0-9a-f]+',tail) else tail.replace('_',' ')
        rooms[name] = {'area':areas[group],'room':index,'name':label}
    for m in re.finditer(r'\{\s*WARP_TYPE_(?:AREA|BORDER)\s*,([^{}]+)\}',(source/'src/data/transitions.c').read_text()):
        v=[x.strip() for x in m[1].split(',')]
        if len(v)!=13: continue
        sx,sy,ex,ey,shape,area,room,layer,transition,facing,*_=v
        if area not in areas or room not in rooms or transition!='TRANSITION_TYPE_NORMAL': continue
        try: x,y,z,d=int(ex,0),int(ey,0),int(layer,0),int(facing,0)
        except ValueError: continue
        if x>=0x8000 or y>=0x8000 or z not in (1,2): continue
        entries.setdefault((areas[area],rooms[room]['room']),{'x':x,'y':y,'layer':z,'facing':d,'source':room})
    lines=['/* GENERATED from area.h, roomid.h and native transitions.c. */','static const char* const sAreaNames[0x90] = {']
    for name,i in areas.items():
        if i>=0x90: continue
        label=name.removeprefix('AREA_').replace('_',' ')
        if label.startswith('NULL ') or re.fullmatch(r'[0-9A-F]+',label): label='UNNAMED AREA'
        lines.append(f' [{i}] = {json.dumps(label)},')
    lines+=['};','static const WarpRoomInfo sRoomNames[] = {']
    for r in rooms.values():
        if r['area']>=0x90: continue
        e=entries.get((r['area'],r['room']),{})
        lines.append(' {%d,%d,%d,%d,%d,%d,%s},'%(r['area'],r['room'],e.get('x',0),e.get('y',0),e.get('layer',0),e.get('facing',0),json.dumps(r['name'])))
    output.write_text('\n'.join(lines+['};'])+'\n',encoding='ascii')
    output.with_suffix('.json').write_text(json.dumps({'rooms':list(rooms.values()),'entries':[{'area':k[0],'room':k[1],**v} for k,v in entries.items()]},indent=2)+'\n')
    print(f'Warp metadata: {len(rooms)} room definitions, {len(entries)} native entries; names do not imply safe spawn.')


if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--source',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    generate(a.source,a.output)
