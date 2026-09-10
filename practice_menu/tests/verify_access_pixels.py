"""Read-only comparison of real mGBA scanout frames; no image alteration.

Each reference is one native frame from the same in-memory save state, without
opening Practice. The modal's first visible return frame must match it exactly.
This accounts for a native OAM transfer already prepared before opening.
"""
import hashlib
import json
import argparse
from pathlib import Path
from PIL import Image, ImageChops

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--candidate', default='ACCESS')
parser.add_argument('--region',choices=('EU','JP'))
args = parser.parse_args()
prefix = args.candidate.lower()
rom_path=root/f'build/TMC_Practice_USA_{args.candidate}_TEST.gba'
report_path=root/f'build/{prefix}_pixels.json'
if args.region:
    prefix='region_'+args.region.lower()
    rom_path=root/f'build/regions/{args.region}/TMC_Practice_{args.region}.gba'
    report_path=root/f'build/regions/{args.region}/pixels.json'
rom_hash = hashlib.sha256(rom_path.read_bytes()).hexdigest().upper()
cases = {
    'access_modal': ['inventory', 'pause_page_1', 'pause_page_2', 'pause_page_4', 'logo', 'title'],
    'access_sequences': ['story_intro', 'credits', 'credits_end', 'ending_save_prompt'],
    'access_views': ['native_save_prompt', 'dungeon_map', 'figurine_list', 'figurine_description'],
}
checks, menus = [], []

def read(path):
    with Image.open(path) as im:
        assert im.size == (240, 160), path
        return im.convert('RGB')

for case, names in cases.items():
    folder = root/'test_results'/(prefix+'_verified_'+case)
    assert (folder/'done.txt').read_text().strip() == 'GAMEPLAY_CHECKS failures=0', case
    assert hashlib.sha256((folder/'test.gba').read_bytes()).hexdigest().upper() == rom_hash, case
    for name in names:
        menu = read(folder/(name+'_menu.png'))
        resumed = read(folder/(name+'_resumed.png'))
        counts = menu.getcolors(240*160)
        assert counts and len(counts) >= 4, (case, name, 'blank menu')
        text_pixels = sum(n for n, rgb in counts if min(rgb) >= 200)
        assert text_pixels >= 100, (case, name, 'no visible menu text')
        assert ImageChops.difference(menu, resumed).getbbox(), (case, name, 'menu never leaves')
        menus.append(dict(case=case, view=name, text_pixels=text_pixels, passed=True))
        if case != 'access_views':
            expected = read(folder/(name+'_expected_visible.png'))
            actual = read(folder/(name+'_visible.png'))
            assert len(expected.getcolors(240*160)) > 1, (case, name, 'blank native reference')
            diff = ImageChops.difference(expected, actual)
            changed = sum(count for count, rgb in diff.getcolors(240*160) if rgb != (0, 0, 0))
            assert changed == 0, (case, name, changed)
            checks.append(dict(case=case, view=name, changed_pixels=changed, passed=True))

assert len(checks) == 10 and len(menus) == 14
report = dict(passed=True, rom_sha256=rom_hash, paired_native_frames=checks, visible_menu_frames=menus,
              reference='same native state, one normal frame, no Practice opening',
              forced_blank_transfer_frame_excluded=True)
report_path.write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
