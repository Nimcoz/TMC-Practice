"""Deterministic labels from the local decomp's native ID enums (not a spawn allowlist)."""
import argparse
import hashlib
import json
import re
from pathlib import Path

SPECS = {3: ('enemy.h', 'OCTOROK', 103), 4: ('projectile.h', 'DARK_NUT_SWORD_SLASH', 37),
         6: ('object.h', 'GROUND_ITEM', 194), 7: ('npc.h', 'NPC_NONE_0', 128),
         8: ('playeritem.h', 'PLAYER_ITEM_NONE', 25)}
ALIASES = {'TPWNSPERSON': 'TOWNSPERSON', 'GOBDO': 'GIBDO', 'DARK_NUT': 'DARKNUT',
           'BOBOMB': 'BOB-OMB', 'PACCI_CANE': 'CANE OF PACCI', 'CHUCHU_BOSS': 'BIG CHUCHU',
           'NPC_UNK_5': 'UNUSED ZELDA FOLLOWER',
           'RUPEE1': 'GREEN RUPEE', 'RUPEE5': 'BLUE RUPEE', 'RUPEE20': 'RED RUPEE',
           'SHELLS30': 'SHELL PICKUP', 'NONE2': 'UNUSED MOLE MITTS', 'NONE3': 'UNUSED ROCS CAPE'}

def enums(path, first, stop_at_gap=False):
    src = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(encoding='utf-8'), flags=re.S)
    for body in re.findall(r'\benum(?:\s+\w+)?\s*\{([^}]+)\}', src):
        tokens = [s.strip() for s in body.split(',') if s.strip()]
        if tokens[0].split('=')[0].strip() != first:
            continue
        names = []
        for token in tokens:
            match = re.fullmatch(r'(\w+)(?:\s*=\s*(0x[\da-fA-F]+|\d+))?', token)
            if not match:
                raise ValueError(f'Unsupported enum token: {token}')
            if match[2] is not None and int(match[2], 0) != len(names):
                if stop_at_gap:
                    break  # Item FC..FF are drop commands, not direct item IDs.
                raise ValueError('Non-contiguous native ID enum')
            names.append(match[1])
        return names
    raise ValueError(f'Missing enum {first}')

def label(symbol):
    original = symbol
    symbol = re.sub(r'^(PLAYER_ITEM_|ITEM_)', '', symbol)
    if symbol in ALIASES:
        return ALIASES[symbol].replace('_', ' ')
    if re.fullmatch(r'NPC_NONE_\d+', symbol) or symbol in ('NONE', 'NULLED', 'NULLED2'):
        return 'UNUSED / NO ACTOR'
    if re.fullmatch(r'(?:NPC_UNK_|OBJECT_|ENEMY_|PROJECTILE_)[0-9A-F]+', symbol):
        return 'UNKNOWN ' + symbol.replace('_UNK_', ' ').replace('_', ' ')
    symbol = ALIASES.get(symbol, symbol)
    symbol = symbol.replace('DARK_NUT', 'DARKNUT').replace('PACCI_CANE', 'CANE OF PACCI')
    result = symbol.replace('_', ' ')
    if len(result) > 52:
        raise ValueError(f'Label needs explicit shortening: {original}')
    return result

def generate(source):
    names = {1: ['LINK']}
    provenance = {}
    for kind, (file, first, count) in SPECS.items():
        path = source / 'include' / file
        symbols = enums(path, first)
        assert len(symbols) == count, (file, len(symbols), count)
        names[kind] = [label(s) for s in symbols]
        provenance[f'include/{file}'] = hashlib.sha256(path.read_bytes()).hexdigest()
    path = source / 'src/manager.c'
    body = re.search(r'gMiscManagerunctions\[\]\)\(\)\s*=\s*\{(.*?)\};', path.read_text(), re.S)
    if body is None:
        body = re.search(r'gMiscManagerunctions\[\].*?=\s*\{(.*?)\};', path.read_text(), re.S)
    symbols = [s.strip() for s in body[1].split(',') if s.strip()]
    assert len(symbols) == 58
    names[9] = ['NO NATIVE HANDLER'] + [re.sub(r'(?<=[a-z0-9])(?=[A-Z])', ' ',
                    s.removesuffix('_Main').removesuffix('Manager')).upper() for s in symbols[1:]]
    names[9][41] = 'UNKNOWN MANAGER 29'
    assert all(0 < len(s) <= 52 for row in names.values() for s in row)
    provenance['src/manager.c'] = hashlib.sha256(path.read_bytes()).hexdigest()
    path = source / 'include/item.h'
    items = [label(s) for s in enums(path, 'ITEM_NONE', stop_at_gap=True)]
    provenance['include/item.h'] = hashlib.sha256(path.read_bytes()).hexdigest()
    return names, items, provenance

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    names, items, provenance = generate(a.source)
    lines = ['/* Generated native labels; unknown/unused entries are not invented names. */']
    for kind, values in sorted(names.items()):
        lines.append(f'static const char* const actorNames{kind}[] = {{')
        lines += ['    '+json.dumps(s)+',' for s in values]
        lines.append('};')
    lines.append('static const char* const groundItemNames[] = {')
    lines += ['    '+json.dumps(s)+',' for s in items]
    lines += ['};', 'static const char* const* const actorNames[10] = {']
    lines += [f'    [{k}] = actorNames{k},' for k in sorted(names)]
    lines += ['};', 'static const u16 actorNameCounts[10] = {']
    lines += [f'    [{k}] = {len(v)},' for k,v in sorted(names.items())]
    lines.append('};')
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text('\n'.join(lines)+'\n', encoding='ascii')
    report = dict(kinds=names, ground_items=items, sources_sha256=provenance)
    a.output.with_suffix('.json').write_text(json.dumps(report, indent=2)+'\n')
    print(f'Actor labels: {sum(map(len,names.values()))} native slots, {len(items)} ground-item types.')

if __name__ == '__main__':
    main()
