"""Prepare disposable regional test fixtures; never convert a user's save in place.

The stock USA progress fixture is explicitly translated by named native flags,
regional EEPROM signature and native checksum rules. These are synthetic port
test fixtures, NOT a claim to have received EU/JP saves from the user.
"""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'practice_menu/tools'))
from map_regions import BASE,Mapper,read_roms,u32

def flags(region):
    text=(ROOT/'work/tmc/include/flags.h').read_text()
    text=subprocess.check_output([str(ROOT/'practice_menu/toolchain/bin/arm-none-eabi-gcc.exe'),'-D'+region,'-E','-P','-x','c','-'],input=re.sub(r'^#include.*$','',text,flags=re.M),text=True)
    result={};offsets=[0,0x100,0x200,0x300,0x400,0x500,0x5C0,0x680,0x740,0x800,0x8C0,0x9C0,0xA80]
    for body,group in re.findall(r'typedef enum\s*\{([^}]+)\}\s*(Flag|LocalFlags\d+)\s*;',text):
        bank=0 if group=='Flag' else int(group.removeprefix('LocalFlags'));index=-1
        for entry in body.split(','):
            if not entry.strip():continue
            name,*value=entry.strip().split('=');index=int(value[0].strip(),0) if value else index+1
            result[name.strip()]=offsets[bank]+index
    return result

def endian(raw):return bytearray(b''.join(raw[i:i+8][::-1] for i in range(0,len(raw),8)))
def checksum(data):
    return sum(word^(len(data)-2*i) for i,word in enumerate(struct.unpack('<'+'H'*(len(data)//2),data)))&65535
def status(data):
    mark=b'3ZCM';c=(checksum(mark)+checksum(data))&65535
    return struct.pack('<HH',c,(-c)&65535)+mark

def fixture(source,region,language,dest,oldflags,newflags):
    raw=source.read_bytes();assert len(raw)==8192
    data=endian(raw);changes=[]
    assert data[:32].startswith(b'AGBZELDA:THE MINISH CAP:ZELDA 5')
    for base in (0,0x1000):data[base:base+32]=b'AGBZELDA:THE MINISH CAP:ZELDA 3'.ljust(32,b'\0')
    for offset,check,size in ((0x70,0x20,16),(0x80,0x30,0x500),(0x580,0x40,0x500),(0xA80,0x50,0x500)):
        if data[check+4:check+8]!=b'3ZCM':continue
        original=data[offset:offset+size]
        assert data[check:check+8]==status(original),(source,hex(check),'native checksum')
        modified=bytearray(original)
        if size==16:
            modified[7]=language
        else:
            old=original[0x25C:0x45C];new=bytearray(512)
            names={bit:name for name,bit in oldflags.items()}
            for bit in range(4096):
                if not old[bit//8]&(1<<(bit%8)):continue
                name=names.get(bit);target=newflags.get(name) if name else bit
                changes.append({'slot':(offset-0x80)//0x500,'source_bit':bit,'name':name,'regional_bit':target})
                if target is not None:new[target//8]|=1<<(target%8)
            modified[0x25C:0x45C]=new
            if region=='JP':modified[0x80:0x86]=bytes.fromhex('97 7F DD 00 00 00')
        for mirror in (0,0x1000):
            data[offset+mirror:offset+mirror+size]=modified
            data[check+mirror:check+mirror+8]=status(modified)
    result=endian(data);dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(result)
    return {'source':str(source.relative_to(ROOT)),'source_sha256':hashlib.sha256(raw).hexdigest(),'fixture':str(dest.relative_to(ROOT)),'sha256':hashlib.sha256(result).hexdigest(),'language':language,'flags':changes}

def main():
    roms=read_roms(Path('C:/Users/user/Downloads'));oldflags=flags('USA')
    for region in ('EU','JP'):
        build=ROOT/'practice_menu/build/regions'/region
        staged=build/'tests';staged.mkdir(parents=True,exist_ok=True)
        mapper=Mapper(roms['USA'],roms[region]);mapping={};missing={}
        records=[]
        for path in sorted((ROOT/'practice_menu/tests').glob('*.lua')):
            if path.name.startswith('region_'):continue
            text=path.read_text();changes=[]
            def replace(m):
                a=int(m[0],16)
                if not BASE<a<0x09000000:return m[0]
                if a not in mapping:
                    match=mapper.literal(a) or mapper.code(a)
                    # A range endpoint is not an instruction entry. Translate
                    # only when its exact preceding table has a known length.
                    if not match and a==0x08007DC0:
                        b=mapper.literal(0x08007CAC)
                        match={'address':f'{int(b["address"],16)+0x114:08X}','method':'surface-key-table-end'}
                    if not match:
                        missing.setdefault(f'{a:08X}',[]).append(path.name);return m[0]
                    mapping[a]=int(match['address'],16)
                changes.append({'usa':f'{a:08X}','regional':f'{mapping[a]:08X}'})
                return f'0x{mapping[a]:08X}'
            text=re.sub(r'0x[0-9a-fA-F]+',replace,text)
            if region=='EU':
                # Native EU starts on the extra LANGUAGE row (index 3).
                # Navigate to slot 0 with real UP inputs; do not force states.
                text=text.replace("if H.r8(H.M+2)==1 and i%120<4 then k=1 end", "if H.r8(H.M+2)==1 and i%120<4 then k=H.r8(0x02019EE6)>0 and 0x40 or 1 end")
                text=text.replace("if r8(M+2)==1 and n%120<4 then k=1 end", "if r8(M+2)==1 and n%120<4 then k=r8(0x02019EE6)>0 and 0x40 or 1 end")
            text=text.replace("out..'../../tests/gameplay_harness.lua'",repr((staged/'gameplay_harness.lua').as_posix()))
            (staged/path.name).write_text(text)
            records.append({'file':path.name,'replacements':changes})
        sources={'base':ROOT/'work/mgba_test/tmc_final_test.sav'}
        for family in ('expansion','comfort'):
            for p in (ROOT/f'practice_menu/test_results/{family}_settings_fixtures').glob('*.sav'):
                sources[family+'_'+p.stem]=p
        fixture_reports=[]
        for name,p in sources.items():
            fixture_reports.append(fixture(p,region,2 if region=='EU' else 0,build/'fixtures'/f'{name}.sav',oldflags,flags(region)))
        if region=='EU':
            for language in range(3,7):fixture_reports.append(fixture(sources['base'],region,language,build/'fixtures'/f'language_{language}.sav',oldflags,flags(region)))
        (build/'fixtures/blank.sav').write_bytes(b'\xFF'*8192)
        cases=[]
        for runner in ('run_expansion_regression.ps1','run_actors_extra.ps1'):
            text=(ROOT/'practice_menu/tests'/runner).read_text()
            for name,script,save in re.findall(r"@\('([^']+)','([^']+\.lua)'(?:,'([^']+)')?",text):
                if name=='actor_guards':continue # Expert replacement is enabled.
                fixture_name='base'
                if 'settings_fixtures/' in save:
                    family='comfort' if 'comfort_settings' in save else 'expansion'
                    fixture_name=family+'_'+Path(save).stem
                cases.append({'name':name,'script':str((staged/script).relative_to(ROOT)),
                              'save':str((build/'fixtures'/f'{fixture_name}.sav').relative_to(ROOT)),
                              'rom':str((build/f'TMC_Practice_{region}{"_autotest" if name=="internal" else ""}.gba').relative_to(ROOT))})
        assert len(cases)==53 and len(set(c['name'] for c in cases))==53,len(cases)
        (build/'test_cases.json').write_text(json.dumps(cases,indent=2)+'\n')
        (build/'test_preparation.json').write_text(json.dumps({'region':region,'fixtures':fixture_reports,'test_relocations':records,'unresolved_test_addresses':missing},indent=2)+'\n')
        print(region,'unresolved test addresses',json.dumps(missing),flush=True)

if __name__=='__main__':main()
