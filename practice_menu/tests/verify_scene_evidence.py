"""Read-only audit of scene replay logs, save copies and actual scanout PNGs."""
import hashlib
import json
import re
import argparse
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--region',choices=('EU','JP'))
args=parser.parse_args()
digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest().upper()
prefix='scenes_verified_'
report_path=root/'build/scenes_replay_evidence.json'
if args.region:
    folder=root/'build/regions'/args.region
    rom=digest(folder/f'TMC_Practice_{args.region}.gba')
    save=digest(folder/'fixtures/base.sav')
    prefix='region_'+args.region.lower()+'_verified_'
    report_path=folder/'replay_evidence.json'
else:
    rom = digest(root/'build/TMC_Practice_USA_SCENES_TEST.gba')
    save = digest(root.parent/'work/mgba_test/tmc_final_test.sav')
    assert save == '80909955A162A18D85B6AEC8BCC19EC0F198EE5EA0183B3EA03A3B8E877451F8'
results = []
for name in ('scene_replays', 'scene_full'):
    folder = root/'test_results'/(prefix+name)
    assert (folder/'done.txt').read_text().strip() == 'GAMEPLAY_CHECKS failures=0', name
    assert digest(folder/'test.gba') == rom, name
    assert digest(folder/'test.sav') == save, 'replay wrote EEPROM: '+name
    trace = (folder/'trace.log').read_text()
    assert 'FAIL' not in trace, name
    if name == 'scene_full':
        for message in ('full native replay auto-returns 5', 'full native replay auto-returns 6',
                        'full replay restores every save byte 5', 'full replay restores every save byte 6',
                        'full ending naturally reaches credits', 'Practice opens during replay credits',
                        'modal graphics do not overwrite replay save backup'):
            assert 'PASS '+message in trace, message
        frames = re.findall(r'auto-returns (\d) frames=(\d+)', trace)
    else:
        assert trace.count('PASS all 1204 native save bytes restored') == 2
        assert 'PASS cheat settings restored' in trace
        assert 'PASS ending warp blocked from native inventory' in trace
        frames = []
    results.append(dict(suite=name, passed=True, eeprom_unchanged=True, frames=frames,
                        rom_sha256=rom, save_sha256=save))

images = []
for name, files in {'scene_replays': ['warp_choices_5', 'confirm_5', 'intro_visible',
                                      'ending_visible', 'replay_return_menu_1', 'replay_return_menu_2'],
                    'scene_full': ['full_return_5', 'full_return_6', 'replay_credits_menu']}.items():
    for file in files:
        path = root/'test_results'/(prefix+name)/(file+'.png')
        with Image.open(path) as image:
            assert image.size == (240, 160), path
            colors = image.convert('RGB').getcolors(240*160)
            assert colors and len(colors) >= 4, ('blank frame', path)
            images.append(dict(path=path.relative_to(root).as_posix(), sha256=digest(path), colors=len(colors)))
report = dict(passed=True, rom_sha256=rom, full_save_bytes=1204, suites=results, visible_frames=images,
              normal_ending_guard='Replay only; native ACCESS ending-save suite separately required')
report_path.write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
