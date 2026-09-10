"""Independent EEPROM decoding/CRC, save isolation and unchanged scene outcome."""
import binascii
import hashlib
import json
import struct
import sys
from pathlib import Path

root=Path(__file__).resolve().parents[1]
prefix=sys.argv[1] if len(sys.argv)>1 else 'comfort'
pages=int(sys.argv[2]) if len(sys.argv)>2 else 26
before_path=Path(sys.argv[3]) if len(sys.argv)>3 else root.parent/'work/mgba_test/tmc_final_test.sav'
before=before_path.read_bytes()
records=[]
for case in ('settings_figures','comfort'):
    after=(root/f'test_results/{prefix}_verified_{case}/test.sav').read_bytes()
    assert len(before)==len(after)==8192
    changed=[i for i,(a,b) in enumerate(zip(before,after)) if a!=b]
    assert changed and all(0xFC0<=i<0x1000 or 0x1FC0<=i<0x2000 for i in changed)
    copies=[]
    for offset in (0xFC0,0x1FC0):
        r=b''.join(after[i:i+8][::-1] for i in range(offset,offset+64,8))
        assert struct.unpack_from('<I',r)[0]==0x54455350
        assert struct.unpack_from('<H',r,4)[0]==2
        assert struct.unpack_from('<H',r,6)[0]==binascii.crc_hqx(r[8:],0xFFFF)
        assert r[11]==pages
        if case=='comfort':
            assert struct.unpack_from('<HH',r,44)==(0x204,9)
            assert r[48:50]==b'\x03\x01' and all(r[i]!=255 for i in range(48,64,2))
        else: assert r[8]==2
        copies.append(r)
    assert copies[0]==copies[1]
    records.append(dict(case=case,changed_bytes=len(changed),outside_settings_changed_bytes=0,
                        version=2,crc16=hex(struct.unpack_from('<H',copies[0],6)[0]),copies_identical=True))
baseline=(root/f'test_results/{prefix}_verified_scene_baseline/scene_flags.bin').read_bytes()
skip=(root/f'test_results/{prefix}_verified_scene/scene_flags.bin').read_bytes()
assert len(baseline)==512 and baseline==skip
report=dict(passed=True,settings_records=records,scene_flags_identical=True,
            scene_flags_sha256=hashlib.sha256(skip).hexdigest().upper())
(root/f'build/{prefix}_evidence.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
