"""Build one explicitly selected EU/JP port from the unchanged USA sources.

Native references are relocated in a disposable build tree, with a complete
replacement manifest. This is not a universal ROM converter: only the two
exact verified retail hashes are supported. The USA build/deliveries remain
unchanged. Generated ROMs are local verification artifacts, not distribution.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import re
import struct
import subprocess
import sys
import shutil

from map_regions import BASE, ROOT, ROMS, Mapper, read_roms, u16, u32, occurrences
import build_rom as baseline
from generate_actor_names import generate as actor_names
from generate_warp_metadata import generate as warp_metadata
from generate_completion_flags import generate as completion_flags

PROJECT=ROOT/'practice_menu'

def sha(data): return hashlib.sha256(data).hexdigest().upper()

def bl_target(data,p):
    a,b=u16(data,p),u16(data,p+2)
    assert a&0xF800==0xF000 and b&0xF800==0xF800
    d=((a&2047)<<12)|((b&2047)<<1)
    if d&0x400000: d-=0x800000
    return BASE+p+4+d

def build(region,downloads,toolchain=None):
    roms=read_roms(downloads)
    clean=roms[region];usa=roms['USA'];mapper=Mapper(usa,clean)
    candidates=json.loads((PROJECT/'build/regions/address_candidates.json').read_text())
    entries=candidates['regions'][region]
    assert all(e['match'] for e in entries.values())
    addresses={int(a,16):int(e['match']['address'],16) for a,e in entries.items()}
    assert all((a&1)==(b&1) for a,b in addresses.items()),'Thumb state must be preserved'
    # Explicitly derived members of known native tables / instruction ranges.
    for start,size in ((0x08100CBC,24),(0x080B2248,40),(0x08109A30,12)):
        for delta in range(size):
            addresses.setdefault(start+delta,addresses[start]+delta)
    for addr in (0x0805F9E8,0x0805F9F0,0x0805F9F8,0x0810D504,0x0810D50C,
                 0x0805E7BC,0x0805E900,0x080A1270,0x08081404,0x080526A0,0x08074BF8,0x08074200):
        for delta in range(8): addresses.setdefault(addr+delta,addresses[addr]+delta)

    def address(a):
        if a==BASE or not BASE<a<0x09000000: return a
        if a not in addresses: raise ValueError(f'unmapped native reference {a:08X}')
        return addresses[a]

    def native(a):
        if a in addresses: return addresses[a]
        hit=mapper.literal(a) or mapper.code(a)
        if not hit: raise ValueError(f'unresolved native call/literal {a:08X}')
        return int(hit['address'],16)

    builddir=PROJECT/'build/regions'/region
    builddir.mkdir(parents=True,exist_ok=True)
    tool=toolchain or PROJECT/'toolchain/bin'
    if not (tool/'arm-none-eabi-gcc.exe').exists():
        compiler=shutil.which('arm-none-eabi-gcc')
        if not compiler: raise ValueError('GNU Arm compiler missing; pass --toolchain BIN_DIRECTORY')
        tool=Path(compiler).parent
    gcc=tool/'arm-none-eabi-gcc.exe'
    report={'region':region,'status':'BUILD_ONLY_NOT_RELEASE_VERIFIED',
            'input_sha1':hashlib.sha1(clean).hexdigest().upper(),'input_sha256':sha(clean),
            'source_relocations':[],'hook_preimages':[],'patch_writes':[],'exploration_relocations':[]}
    # Re-check all native hook preimages semantically against the USA baseline.
    # Branch immediates are regional; their decoded targets must independently
    # identify the same native function. PC literal operands must refer to the
    # same RAM location or the independently matched regional ROM object.
    def validate_span(a,count):
        p=a-BASE;q=address(a)-BASE
        i=0
        while i<count:
            if i+4<=count and (p+i)%4==0 and BASE<u32(usa,p+i)<0x09000000:
                old=u32(usa,p+i);new=u32(clean,q+i)
                assert new==native(old),(hex(a+i),'pointer',hex(old),hex(new))
                i+=4;continue
            x,y=u16(usa,p+i),u16(clean,q+i)
            if i+4<=count and x&0xF800==0xF000 and u16(usa,p+i+2)&0xF800==0xF800:
                old=bl_target(usa,p+i);new=bl_target(clean,q+i)
                assert new==(native(old|1)&~1),(hex(a+i),'BL',hex(old),hex(new))
                i+=4;continue
            if x&0xF800==0x4800:
                assert x&0xFF00==y&0xFF00
                v=u32(usa,((p+i+4)&~3)+4*(x&255))
                w=u32(clean,((q+i+4)&~3)+4*(y&255))
                assert w==(native(v) if BASE<v<0x09000000 else v),(hex(a+i),'LDR',hex(v),hex(w))
            else:
                assert x==y,(hex(a+i),'instruction',hex(x),hex(y))
            i+=2

    # Three displaced prologues and pickup branch target used by inline asm.
    for a,n in ((0x0805E7BC,10),(0x0805E900,10),(0x080A1270,8),(0x08081404,24)):
        validate_span(a,n)
    # Main/game/subtask/entity tables are native, never borrowed from USA.
    for start,count in ((0x08100CBC,6),(0x080B2248,10)):
        for i in range(count):
            v=u32(usa,start-BASE+4*i);w=u32(clean,address(start)-BASE+4*i)
            assert w==native(v),(hex(start+4*i),'table')
    old_sub=u32(usa,0xA753C);new_sub=u32(clean,address(0x080A753C)-BASE)
    for i in range(11):
        assert u32(clean,new_sub-BASE+4*i)==native(u32(usa,old_sub-BASE+4*i))
    # All native sound tracks lie below the separately linked modal scratch.
    audio=mapper.literal(0x08A11C3C)
    assert audio,'native audio table'
    cursor=0x02036BC0;tracks=[]
    for i in range(32):
        _,start,count=struct.unpack_from('<IIB',clean,int(audio['address'],16)-BASE+12*i)
        assert start==cursor and count
        cursor+=count*0x50;tracks.append(count)
    assert cursor==0x02038560 and sum(tracks)==82
    report['native_audio']={'table':audio['address'],'tracks':sum(tracks),'ewram_end':f'{cursor:08X}'}
    # Reserved veneers are in disabled DebugTask code, not arbitrary zero data.
    init=addresses[0x0810D504]
    incoming=[]
    for p in range(0,0xB0000-4,2):
        if u16(clean,p)&0xF800==0xF000 and u16(clean,p+2)&0xF800==0xF800:
            if init<=bl_target(clean,p)<init+16: incoming.append(f'{p+BASE:08X}')
    assert not incoming,'debug init has live direct callers'
    refs=[p+BASE for p in occurrences(clean,struct.pack('<I',init|1)) if p%4==0]
    assert refs==[addresses[0x08109A30]],refs
    report['veneer_reservation']={'start':f'{init:08X}','bytes':16,'only_pointer_reference':f'{refs[0]:08X}','direct_callers':incoming}

    # Staging is a deterministic mechanical rewrite, never edits source files.
    for directory in ('src','include'):
        for path in sorted((PROJECT/directory).rglob('*')):
            if not path.is_file() or path.suffix not in ('.c','.h'): continue
            dest=builddir/'staged'/path.relative_to(PROJECT)
            dest.parent.mkdir(parents=True,exist_ok=True)
            source=path.read_text()
            changes=[]
            def replace(m):
                a=int(m[0],16);b=address(a)
                if a!=b: changes.append({'usa':f'{a:08X}','regional':f'{b:08X}'})
                return f'0x{b:08X}'
            staged=re.sub(r'0x08[0-9a-fA-F]{6}',replace,source)
            staged=staged.replace('VERIFIED USA ROM LEFTOVERS',f'VERIFIED {region} ROM LEFTOVERS')
            dest.write_text(staged)
            report['source_relocations'].append({'file':path.relative_to(PROJECT).as_posix(),'source_sha256':sha(path.read_bytes()),'staged_sha256':sha(dest.read_bytes()),'replacements':changes})
    # Generate the identical ID names and native region-specific quest flags.
    # Reuse the established generator CLI/output contract.
    subprocess.run([sys.executable,str(PROJECT/'tools/generate_actor_names.py'),'--source',str(ROOT/'work/tmc'),'--output',str(builddir/'actor_names.h')],check=True)
    warp_metadata(ROOT/'work/tmc',builddir/'warp_metadata.h')
    completion_flags(ROOT/'work/tmc',gcc,builddir/'completion_flags.h',region)
    flags=json.loads((builddir/'completion_flags.json').read_text())
    for name,bit in (('SHOP07_TANA',0x25E),('SHOP07_COMPLETE',0x25F)):
        assert next(x['bit'] for x in flags if x['name']==name)==bit
    common=['-mthumb','-mcpu=arm7tdmi','-Os','-std=c11','-ffreestanding','-fno-builtin','-ffunction-sections','-fdata-sections','-fomit-frame-pointer','-Wall','-Wextra','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast','-I',str(builddir/'staged/include'),'-I',str(builddir)]
    objects=[]
    for path in sorted((builddir/'staged/src').rglob('*.c')):
        obj=builddir/('_'.join(path.relative_to(builddir/'staged/src').with_suffix('.o').parts))
        subprocess.run([str(gcc),*common,'-c',str(path),'-o',str(obj)],check=True)
        objects.append(str(obj))
    elf=builddir/'practice_menu.elf';module=builddir/'practice_menu.bin';symbols=builddir/'practice_menu.sym'
    subprocess.run([str(gcc),'-mthumb','-mcpu=arm7tdmi','-nostdlib',f'-Wl,-T,{PROJECT/"linker.ld"},-Map,{builddir/"practice_menu.map"},--gc-sections',*objects,'-lgcc','-o',str(elf)],check=True)
    subprocess.run([str(tool/'arm-none-eabi-objcopy.exe'),'-O','binary',str(elf),str(module)],check=True)
    symbols.write_bytes(subprocess.check_output([str(tool/'arm-none-eabi-nm.exe'),'-n',str(elf)]))
    payload=bytearray((ROOT/'KNOWN_GOOD_FINAL_EXPLORATION/outputs/FINAL_verified_respawn_guard_payload.bin').read_bytes())
    assert sha(payload)==baseline.EXPLORATION_SHA256
    for off in (0x324,0x328,0x32C,0x338,0x33C,0x340):
        a=u32(payload,off);b=address(a);struct.pack_into('<I',payload,off,b)
        report['exploration_relocations'].append({'offset':f'{off:04X}','usa':f'{a:08X}','regional':f'{b:08X}'})
    payload_path=builddir/'exploration.bin';payload_path.write_bytes(payload)

    # Execute the established hook definition with address/preimage adapters.
    # All emitted writes are logged, including unchanged 16 MiB original data.
    def expect(rom,a,expected_hex):
        expected=bytes.fromhex(expected_hex)
        assert usa[a-BASE:a-BASE+len(expected)]==expected,(hex(a),'USA baseline preimage')
        # The replaced DebugTask remainder can end midway through a BL; its
        # entire function and table were independently matched above.
        if a not in (0x0805F9F0,0x0805F9F8): validate_span(a,len(expected))
        p=address(a)-BASE;regional=clean[p:p+len(expected)]
        assert bytes(rom[p:p+len(expected)])==regional,(hex(address(a)),'regional preimage')
        report['hook_preimages'].append({'usa_address':f'{a:08X}','address':f'{address(a):08X}','usa':expected.hex(),'regional':regional.hex()})
    def write(rom,a,data):
        p=address(a)-BASE;rom[p:p+len(data)]=data
        report['patch_writes'].append({'address':f'{address(a):08X}','data':data.hex()})
    def write_bl(rom,source,target):
        s,t=address(source),address(target);d=t-(s+4)
        assert not d&1 and -(1<<22)<=d<(1<<22)
        write(rom,source,struct.pack('<HH',0xF000|((d>>12)&2047),0xF800|((d>>1)&2047)))
    baseline.CLEAN_SHA1=ROMS[region][1].upper()
    baseline.EXPLORATION_SHA256=sha(payload)
    baseline.expect=expect
    baseline.write16=lambda rom,a,v:write(rom,a,struct.pack('<H',v&65535))
    baseline.write32=lambda rom,a,v:write(rom,a,struct.pack('<I',v&0xFFFFFFFF))
    baseline.write_thumb_bl=write_bl
    for auto in (False,True):
        output=builddir/f'TMC_Practice_{region}{"_autotest" if auto else ""}.gba'
        sys.argv=['build_rom.py','--rom',str(downloads/ROMS[region][0]),'--module',str(module),'--symbols',str(symbols),'--exploration',str(payload_path),'--output',str(output)]+(['--autotest'] if auto else [])
        report['patch_writes']=[];report['hook_preimages']=[]
        baseline.main()
        report['autotest' if auto else 'normal']={'name':output.name,'sha256':sha(output.read_bytes()),'writes':report['patch_writes'].copy()}
    report['module_sha256']=sha(module.read_bytes())
    report['exploration_sha256']=sha(payload)
    (builddir/'port_build.json').write_text(json.dumps(report,indent=2)+'\n')
    print(region,'BUILD COMPLETE; emulator verification pending',flush=True)

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--region',choices=('EU','JP'),required=True)
    p.add_argument('--downloads',type=Path,default=Path('C:/Users/user/Downloads'))
    p.add_argument('--toolchain',type=Path)
    a=p.parse_args();build(a.region,a.downloads,a.toolchain)
