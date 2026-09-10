"""Check compiled ROM tables against native-label metadata and independent spot checks."""
import hashlib
import json
import struct
import sys
from pathlib import Path

root=Path(__file__).resolve().parents[1]
build=Path(sys.argv[2]) if len(sys.argv)>2 else root/'build'
report=json.loads((build/'actor_names.json').read_text())
binary=(build/'practice_menu.bin').read_bytes()
symbols={parts[2]:int(parts[0],16) for line in (build/'practice_menu.sym').read_text().splitlines()
         if len(parts:=line.split())==3}
def u32(p): return struct.unpack_from('<I',binary,p-0x9020000)[0]
def string(p):
    offset=p-0x9020000
    assert 0<=offset<len(binary)
    return binary[offset:binary.index(0,offset)].decode('ascii')
actual={}
for k,expected in report['kinds'].items():
    k=int(k)
    count=struct.unpack_from('<H',binary,symbols['actorNameCounts']-0x9020000+k*2)[0]
    assert count==len(expected)
    table=u32(symbols['actorNames']+4*k)
    actual[k]=[string(u32(table+i*4)) for i in range(count)]
    assert actual[k]==expected
    for name in actual[k]:
        assert 0<len(name)<=52
        if len(name)>26:
            split=next((i for i in range(26,10,-1) if name[i]==' ' and len(name)-i-1<=26),26)
            assert len(name[split:].lstrip())<=26
items=[string(u32(symbols['groundItemNames']+4*i)) for i in range(len(report['ground_items']))]
assert items==report['ground_items']
assert {k:len(v) for k,v in actual.items()}=={1:1,3:103,4:37,6:194,7:128,8:25,9:58}
for k,id,name in [(1,0,'LINK'),(3,0,'OCTOROK'),(3,5,'DARKNUT'),(3,0x24,'GLEEROK'),
                  (3,0x36,'MAZAAL HEAD'),(6,5,'POT'),(6,0x23,'EYE SWITCH'),
                  (7,0x45,'ANJU'),(7,5,'UNUSED ZELDA FOLLOWER'),(9,54,'REPEATED SOUND')]:
    assert actual[k][id]==name
assert items[0x54:0x57]==['GREEN RUPEE','BLUE RUPEE','RED RUPEE']
result=dict(passed=True,native_slots=sum(map(len,actual.values())),ground_item_types=len(items),
            unknown_labels=sum(s.startswith('UNKNOWN ') for row in actual.values() for s in row),
            module_sha256=hashlib.sha256(binary).hexdigest().upper(),sources_sha256=report['sources_sha256'])
(Path(sys.argv[1]) if len(sys.argv)>1 else root/'build/named_labels_verification.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
