"""Independently reconstruct both ports without importing the port builder.

The existing independent hook specification is address-parameterized. Neither
write logs nor claimed hashes are accepted as proof of the resulting ROM.
"""
from pathlib import Path
import hashlib
import json
import re
import struct
import argparse

ROOT=Path(__file__).resolve().parents[2]
PROJECT=ROOT/'practice_menu'
BASE=0x08000000
def sha(data):return hashlib.sha256(data).hexdigest().upper()

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--downloads',type=Path,default=Path('C:/Users/user/Downloads'))
    args=parser.parse_args()
    names={'EU':'Legend of Zelda, The - The Minish Cap (Europe) (En,Fr,De,Es,It).gba',
           'JP':'Zelda no Densetsu - Fushigi no Boushi (Japan).gba'}
    profiles=json.loads((PROJECT/'build/regions/address_candidates.json').read_text())
    verifier=(PROJECT/'tools/verify_release.py').read_text()
    baseline=(ROOT/'KNOWN_GOOD_FINAL_EXPLORATION/outputs/FINAL_verified_respawn_guard_payload.bin').read_bytes()
    assert sha(baseline)=='A284A537A5A174769E355572B287A6A1172C025F5C50EB52569A9933DEC12361'
    reports=[]
    for region in ('EU','JP'):
        folder=PROJECT/'build/regions'/region
        clean=(args.downloads/names[region]).read_bytes()
        assert hashlib.sha1(clean).hexdigest()==profiles['inputs'][region]['sha1']
        m={int(a,16):int(e['match']['address'],16) for a,e in profiles['regions'][region].items()}
        text=re.sub(r'0x08[0-9A-Fa-f]{6}',lambda x:f'0x{m[int(x[0],16)]:08X}' if int(x[0],16) in m else x[0],verifier)
        ns={'__name__':'regional_verifier_library'}
        exec(compile(text,'regional_verifier_library','exec'),ns)
        symbols=ns['parse_symbols'](folder/'practice_menu.sym')
        module=(folder/'practice_menu.bin').read_bytes()
        payload=(folder/'exploration.bin').read_bytes()
        expected=bytearray(baseline)
        for p in (0x324,0x328,0x32C,0x338,0x33C,0x340):
            struct.pack_into('<I',expected,p,m[struct.unpack_from('<I',baseline,p)[0]])
        assert payload==expected,'Exploration changes escape six native literal words'
        normal=(folder/f'TMC_Practice_{region}.gba').read_bytes()
        auto=(folder/f'TMC_Practice_{region}_autotest.gba').read_bytes()
        assert normal==ns['expected_rom'](clean,module,payload,symbols,False),'normal ROM mismatch'
        assert auto==ns['expected_rom'](clean,module,payload,symbols,True),'autotest ROM mismatch'
        diffs=[i for i,(a,b) in enumerate(zip(normal,auto)) if a!=b]
        assert diffs and all(m[0x08100CBC]-BASE<=i<m[0x08100CBC]-BASE+12 for i in diffs)
        assert normal[:0xC0]==clean[:0xC0],'regional header altered'
        assert 0x02038560<symbols['__modal_start__']<symbols['__modal_end__']<=0x0203D000
        assert symbols['gPracticeState']==0x0203D000 and symbols['__bss_end__']<=0x0203F000
        source_report=json.loads((folder/'port_build.json').read_text())
        for entry in source_report['source_relocations']:
            src=(PROJECT/entry['file']).read_bytes()
            staged=(folder/'staged'/entry['file']).read_bytes()
            assert sha(src)==entry['source_sha256'] and sha(staged)==entry['staged_sha256']
            for replacement in entry['replacements']:
                assert m[int(replacement['usa'],16)]==int(replacement['regional'],16)
        report={'result':'PASS','region':region,'clean_sha1':hashlib.sha1(clean).hexdigest().upper(),
                'clean_sha256':sha(clean),'normal_sha256':sha(normal),'autotest_sha256':sha(auto),
                'independent_reconstruction_bytes':len(normal),'header_unchanged':True,
                'native_address_entries':len(m),'module_bytes':len(module),'module_sha256':sha(module),
                'source_files_checked':len(source_report['source_relocations']),
                'exploration_only_six_literals_changed':True,'normal_autotest_differences':diffs,
                'modal_end':f'{symbols["__modal_end__"]:08X}','practice_end':f'{symbols["__bss_end__"]:08X}',
                'native_audio':source_report['native_audio']}
        (folder/'static_verification.json').write_text(json.dumps(report,indent=2)+'\n')
        reports.append(report)
    print(json.dumps(reports,indent=2))

if __name__=='__main__':main()
