"""Compare actual mGBA scanout frames; never modifies images."""
import json
import sys
from pathlib import Path
from PIL import Image

root = Path(sys.argv[1])
checks = []
def pixels(name):
    with Image.open(root / name) as im:
        assert im.size == (240, 160)
        return list(im.convert('RGB').getdata())

for frame in range(65, 71):
    native = pixels(f'mode_0_frame_{frame}.png')
    for mode in range(1, 4):
        overlay = pixels(f'mode_{mode}_frame_{frame}.png')
        outside = timer = debug = 0
        for i, (a, b) in enumerate(zip(native, overlay)):
            if a == b:
                continue
            x, y = i % 240, i // 240
            if 80 <= x < 152 and y < 8 and mode & 1:
                timer += 1
            elif 160 <= x < 224 and 40 <= y < 80 and mode & 2:
                debug += 1
            else:
                outside += 1
        checks.append(dict(frame=frame, mode=mode, outside_changed_pixels=outside,
                           timer_pixels=timer, debug_pixels=debug,
                           passed=outside == 0 and (not mode & 1 or timer > 50)
                           and (not mode & 2 or debug > 150)))
native = pixels('mode_0_cleared.png')
for mode in range(1, 4):
    changed = sum(a != b for a, b in zip(native, pixels(f'mode_{mode}_cleared.png')))
    checks.append(dict(mode=mode, cleared_changed_pixels=changed, passed=changed == 0))
report = dict(passed=all(c['passed'] for c in checks), checks=checks)
(root / 'pixel_verification.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
sys.exit(0 if report['passed'] else 1)
