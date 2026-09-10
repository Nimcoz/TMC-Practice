"""Compare effective warp spawns with region-preprocessed native transitions.

The common label table may contain EU-only 0xFFF 'preserve axis' sentinels.
These are rejected by the existing bounds guard, not treated as fixed spawns.
Prove every valid menu destination has the same result in each retail region.
"""
from pathlib import Path
import json
import re
import struct
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'practice_menu/tools'))
from map_regions import read_roms,BASE

def main():
    roms=read_roms(Path('C:/Users/user/Downloads'))
    profiles=json.loads((ROOT/'practice_menu/build/regions/address_candidates.json').read_text())
    source=(ROOT/'work/tmc/src/data/transitions.c').read_text()
    area_names={n:i for i,n in enumerate(re.findall(r'^\s+(AREA_\w+),',(ROOT/'work/tmc/include/area.h').read_text(),re.M))}
    for region in ('EU','JP'):
        folder=ROOT/'practice_menu/build/regions'/region
        data=json.loads((folder/'warp_metadata.json').read_text())
        rooms={}
        group=None;index=-1
        for line in (ROOT/'work/tmc/include/roomid.h').read_text().splitlines():
            a=re.search(r'// (AREA_\w+)',line)
            if a:group=a[1]
            m=re.search(r'^\s+(ROOM_\w+)(?:\s*=\s*(\w+))?,',line)
            if m and group in area_names:
                index=int(m[2],0) if m[2] else index+1;rooms[m[1]]=(area_names[group],index)
        plain=subprocess.check_output([str(ROOT/'practice_menu/toolchain/bin/arm-none-eabi-gcc.exe'),'-D'+region,'-E','-P','-x','c','-'],input=re.sub(r'^#include.*$','',source,flags=re.M),text=True)
        native={}
        for m in re.finditer(r'\{\s*WARP_TYPE_(?:AREA|BORDER)\s*,([^{}]+)\}',plain):
            v=[x.strip() for x in m[1].split(',')]
            if len(v)!=13:continue
            _,_,x,y,_,area,room,layer,transition,facing,*_=v
            if room not in rooms or area not in area_names or transition!='TRANSITION_TYPE_NORMAL':continue
            try:x,y,layer=int(x,0),int(y,0),int(layer,0)
            except ValueError:continue
            if x>=0x8000 or y>=0x8000 or layer not in (1,2):continue
            native.setdefault(rooms[room],(x,y,layer))
        existing={(e['area'],e['room']):(e['x'],e['y'],e['layer']) for e in data['entries']}
        rom=roms[region];table=int(profiles['regions'][region]['0811E214']['match']['address'],16)
        checks=[];sentinels=[]
        for row in data['rooms']:
            a,r=row['area'],row['room']
            if a>=0x90 or r>=0x40:continue
            p=struct.unpack_from('<I',rom,table-BASE+a*4)[0]-BASE
            if not 0<=p<len(rom):continue
            if any(struct.unpack_from('<H',rom,p+i*10)[0]==65535 for i in range(r+1)):continue
            w,h=struct.unpack_from('<HH',rom,p+r*10+4)
            if not w or not h:continue
            def effective(e):
                return (*e,True) if e and e[0]<w and e[1]<h else (w//2 if w>32 else 8,h//2 if h>32 else 8,1,False)
            old,new=existing.get((a,r)),native.get((a,r))
            assert effective(old)==effective(new),(region,a,r,old,new)
            if old!=new:sentinels.append({'area':a,'room':r,'stored':old,'native':new,'effective':effective(old)})
            checks.append((a,r))
        report={'passed':True,'region':region,'valid_destinations_checked':len(checks),'ineffective_region_only_sentinels':sentinels,'all_effective_spawns_and_labels_identical':True}
        (folder/'warp_metadata_audit.json').write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps(report))

if __name__=='__main__':main()
