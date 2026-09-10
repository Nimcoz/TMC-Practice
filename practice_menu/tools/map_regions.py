"""Derive candidate retail-region addresses from the user's clean ROMs.

This is an analysis tool, NOT a patcher. Unique instruction signatures and
PC-relative literal references are recorded for review before any port build.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import struct

BASE = 0x08000000
CODE_END = 0xB3000
ROOT = Path(__file__).resolve().parents[2]
ROMS = {
    "USA": ("Legend of Zelda, The - The Minish Cap (USA).gba", "b4bd50e4131b027c334547b4524e2dbbd4227130"),
    "EU": ("Legend of Zelda, The - The Minish Cap (Europe) (En,Fr,De,Es,It).gba", "cff199b36ff173fb6faf152653d1bccf87c26fb7"),
    "JP": ("Zelda no Densetsu - Fushigi no Boushi (Japan).gba", "6c5404a1effb17f481f352181d0f1c61a2765c5d"),
}

def read_roms(downloads):
    result = {}
    for region, (name, sha) in ROMS.items():
        data = (downloads / name).read_bytes()
        assert hashlib.sha1(data).hexdigest() == sha, region
        result[region] = data
    return result

def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]

def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]

def normalize(data):
    out = bytearray(data[:CODE_END])
    p = 0
    while p < len(out)-4:
        a, b = u16(data, p), u16(data, p+2)
        if not p & 3 and BASE <= u32(data,p) < 0x09000000:
            out[p:p+4] = b'\x00\x00\x00\x08'
            p += 4
        elif a & 0xF800 == 0xF000 and b & 0xF800 == 0xF800:
            out[p:p+4] = b'\x00\xf0\x00\xf8'
            p += 4
        else:
            if a & 0xF800 in (0x4800,0xA000):
                out[p] = 0
            p += 2
    return bytes(out)

def occurrences(data, needle):
    start = 0
    while True:
        p = data.find(needle, start)
        if p < 0:
            break
        yield p
        start = p+1

def collect():
    uses = {}
    paths = list((ROOT/'practice_menu/src').rglob('*.c')) + list((ROOT/'practice_menu/include').rglob('*.h'))
    paths += [ROOT/'practice_menu/tools/build_rom.py']
    for path in paths:
        for lineno,line in enumerate(path.read_text().splitlines(),1):
            for m in re.finditer(r'0x(08[0-9a-fA-F]{6})',line):
                a = int(m[1],16)
                if a != BASE:
                    uses.setdefault(a,[]).append(f'{path.relative_to(ROOT).as_posix()}:{lineno}')
    payload = (ROOT/'KNOWN_GOOD_FINAL_EXPLORATION/outputs/FINAL_verified_respawn_guard_payload.bin').read_bytes()
    # Only literal-pool words, not instruction pairs that resemble pointers.
    for p in range(0x300,len(payload)-3,4):
        a = u32(payload,p)
        if BASE < a < 0x09000000:
            uses.setdefault(a,[]).append(f'exploration+{p:04X}')
    return uses

class Mapper:
    def __init__(self, source, target):
        self.source, self.target = source,target
        self.ns,self.nt = normalize(source),normalize(target)
        self.cache = {}
        self.literal_refs = {}
        for p in range(0,CODE_END-4,2):
            op = u16(source,p)
            if op & 0xF800 == 0x4800:
                pool = ((p+4)&~3)+4*(op&255)
                value = u32(source,pool)
                if BASE < value < 0x09000000:
                    self.literal_refs.setdefault(value,[]).append((p,pool))

    def code(self, address):
        p = (address & ~1)-BASE
        if not 0 <= p < CODE_END:
            return None
        if p in self.cache:
            found=self.cache[p]
            return dict(found,address=f"{int(found['address'],16)+(address&1):08X}") if found else None
        evidence = []
        for size in (96,64,48,32,24):
            for delta in (0,-size//2, -size+2):
                left = p+delta
                if left<0:
                    continue
                needle = self.ns[left:left+size]
                matches = list(occurrences(self.nt,needle))
                if len(matches)==1 and not matches[0]&1:
                    q=matches[0]-delta
                    evidence.append((q,left,size))
            if evidence:
                break
        candidates = sorted(set(q for q,l,n in evidence))
        result = None
        if len(candidates)==1:
            q=candidates[0]
            result={"address":f'{q+BASE:08X}', "method":"unique-normalized-code", "evidence":[{"source_start":f'{l+BASE:08X}',"target_start":f'{q+l-p+BASE:08X}',"bytes":n} for _,l,n in evidence]}
        self.cache[p]=result
        return dict(result,address=f"{int(result['address'],16)+(address&1):08X}") if result else None

    def literal(self, address):
        evidence=[]
        for p,pool in self.literal_refs.get(address,[]):
            match=self.code(BASE+p)
            if not match:
                continue
            q=int(match['address'],16)-BASE
            op=u16(self.target,q)
            if op & 0xFF00 != u16(self.source,p)&0xFF00:
                continue
            target_pool=((q+4)&~3)+4*(op&255)
            value=u32(self.target,target_pool)
            if BASE < value < 0x09000000:
                evidence.append((value,p,q,pool,target_pool))
        candidates=sorted(set(e[0] for e in evidence))
        if len(candidates)==1:
            return {"address":f'{candidates[0]:08X}',"method":"pc-relative-literal", "evidence":[{"source_ldr":f'{p+BASE:08X}',"target_ldr":f'{q+BASE:08X}',"source_pool":f'{pool+BASE:08X}',"target_pool":f'{tp+BASE:08X}'} for _,p,q,pool,tp in evidence]}
        return None

    def table_slot(self,address):
        value=u32(self.source,address-BASE)
        mapped=self.literal(value) or self.code(value)
        if not mapped:
            return None
        matches=[p for p in occurrences(self.target,struct.pack('<I',int(mapped['address'],16))) if p%4==0 and p>CODE_END]
        original=[p for p in occurrences(self.source,struct.pack('<I',value)) if p%4==0 and p>CODE_END]
        if original==[address-BASE] and len(matches)==1:
            return {"address":f'{matches[0]+BASE:08X}',"method":"unique-function-table-slot","evidence":{"original_pointer":f'{value:08X}',"regional_pointer":mapped['address']}}
        return None

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--downloads',type=Path,default=Path('C:/Users/user/Downloads'))
    parser.add_argument('--output',type=Path,default=ROOT/'practice_menu/build/regions/address_candidates.json')
    args=parser.parse_args()
    roms=read_roms(args.downloads)
    uses=collect()
    report={"status":"CANDIDATES_REQUIRING_REVIEW", "inputs":{k:{"size":len(v),"sha1":hashlib.sha1(v).hexdigest(),"sha256":hashlib.sha256(v).hexdigest(),"game_code":v[0xAC:0xB0].decode()} for k,v in roms.items()},"regions":{}}
    for region in ('EU','JP'):
        mapper=Mapper(roms['USA'],roms[region])
        entries={}
        for addr,refs in sorted(uses.items()):
            match=mapper.literal(addr) or mapper.code(addr)
            if not match and addr>=BASE+CODE_END and addr%4==0:
                match=mapper.table_slot(addr)
            entries[f'{addr:08X}']={"uses":refs,"match":match}
        # Retail main task table is six pointers in every region. Assert each
        # slot against the separately matched native function, not an offset.
        table=int(entries['08100CBC']['match']['address'],16)
        for delta in (4,8,16,20):
            source_addr=0x08100CBC+delta
            ptr=u32(roms['USA'],source_addr-BASE)
            mapped=mapper.literal(ptr) or mapper.code(ptr)
            assert mapped and u32(roms[region],table+delta-BASE)==int(mapped['address'],16)
            entries[f'{source_addr:08X}']['match']={"address":f'{table+delta:08X}',"method":"verified-main-task-slot","evidence":{"pointer":mapped['address']}}
        # The native DebugTask is replaced. Its init routine is referenced
        # only by that now-unreachable table, so reserve its first 16 bytes
        # instead of borrowing a region-specific data/padding area.
        debug_table=int(entries['08109A30']['match']['address'],16)
        debug_init=u32(roms[region],debug_table-BASE)&~1
        mapped=mapper.code(0x0805FA05)
        assert mapped and int(mapped['address'],16)==debug_init+1
        for i,addr in enumerate((0x0810D504,0x0810D50C)):
            entries[f'{addr:08X}']['match']={"address":f'{debug_init+8*i:08X}',"method":"reserved-unreachable-debug-init","evidence":{"native_debug_table":f'{debug_table:08X}',"reason":"Native DebugTask/table replaced; first 16 bytes of its unreachable init routine."}}
        report['regions'][region]=entries
        missing=[k for k,v in entries.items() if not v['match']]
        print(region, 'matched',len(entries)-len(missing),'/',len(entries),'missing',','.join(missing),flush=True)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')

if __name__=='__main__':
    main()
