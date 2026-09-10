"""Compile an explicit USA completion allow-list; never set every save bit.

The native header is preprocessed by the same ARM compiler as the module so
regional enum insertions cannot silently shift selected flags.
"""
import argparse
import json
import re
import subprocess
from pathlib import Path

STORY = '''LV1_CLEAR LV2_CLEAR LV3_CLEAR LV4_CLEAR LV5_CLEAR
MACHI_SET_1 MACHI_SET_2 MACHI_SET_4 MACHI_SET_5 START EZERO_1ST TABIDACHI
LV1TARU_OPEN WATERBEAN_PUT INLOCK DASHBOOTS LEFT_DOOR_OPEN HAKA_KEY_FOUND
KUMOTATSUMAKI MIZUKAKI_START KAKERA_COMPLETE CHIKATSURO_SHUTTER OUTDOOR
ANJU_HEART WARP_1ST WARP_MONUMENT GAMECLEAR WHITE_SWORD_END WARP_EVENT_END
FIGURE_ALLCOMP AKINDO_BOTTLE_SELL TINGLE_TALK1ST ENTRANCE_0 ENTRANCE_1
ENTRANCE_2 MIZUKAKI_NECHAN MAZE_CLEAR TINY_ENTRANCE
LV1_CLEAR_MES LV2_CLEAR_MES LV3_CLEAR_MES LV4_CLEAR_MES LV5_CLEAR_MES
SOUGEN_08_TORITSUKI SOUGEN_05_BOMB_00 DAIGORON_SHIELD
SHOP07_TANA SHOP07_COMPLETE SORA_ELDER_RECOVER SORA_CHIEF_TALK
SORA_ELDER_TALK1ST SORA_ELDER_TALK2ND SORA_KIDS_MOVE
MIZUKAKI_BOOK1_FALL MIZUKAKI_BOOK2_FALL MIZUKAKI_BOOK3_FALL
MIZUKAKI_BOOK_ALLBACK MIZUKAKI_STAIR MIZUKAKI_STAIR_WARP_OK
OYAKATA_DEMO YAMAKOBITO_OPEN M_PRIEST_TALK M_ELDER_TALK1ST
M_PRIEST_MOVE M_ELDER_TALK2ND KOBITO_MORI_1ST KOBITO_YAMA_ENTER
IZUMI_00_FAIRY IZUMI_01_FAIRY IZUMI_02_FAIRY SHOP00_BOMBBAG
OUBO_KAKERA SEIIKI_STAINED_GLASS SEIIKI_ENTER SEIIKI_SWORD_1ST
SEIIKI_SWORD_2ND SEIIKI_SWORD_3RD SEIIKI_BUNSHIN
LV3_16_BOSSDIE LV5_BOSSDIE'''.split()

def generate(source, gcc, output, region='USA'):
    header = source / 'include/flags.h'
    # Preprocessing needs no type definitions: remove includes, retain every
    # regional conditional. The source package needs only this native header.
    native = header.read_text()
    plain = subprocess.check_output([str(gcc), '-D'+region, '-E', '-P', '-x', 'c', '-'],
        input=re.sub(r'^#include.*$', '', native, flags=re.M), text=True)
    bank_offsets = [0, 0x100, 0x200, 0x300, 0x400, 0x500, 0x5C0, 0x680, 0x740, 0x800, 0x8C0, 0x9C0, 0xA80]
    flags = {}
    for body, group in re.findall(r'typedef enum\s*\{([^}]+)\}\s*(Flag|LocalFlags\d+)\s*;', plain):
        bank = 0 if group == 'Flag' else int(group.removeprefix('LocalFlags'))
        index = -1
        for entry in body.split(','):
            entry = entry.strip()
            if not entry: continue
            name, *value = entry.split('=')
            index = int(value[0].strip(), 0) if value else index + 1
            flags[name.strip()] = {'bank': bank, 'index': index, 'bit': bank_offsets[bank]+index}
    hearts = re.findall(r'^\s*(\w+),\s*/\*\*< Obtained Heart (?:Piece|Container)[^*]*', header.read_text(), re.M)
    hearts.remove('URO_1F_H0')  # Unused room pickup is NOT part of normal 100%.
    assert len(hearts) == 48, 'Review heart pickup allow-list if native source changes'
    chosen = list(dict.fromkeys(STORY + hearts))
    # EU Stockwell has no SHOP00_BOMBBAG flag (native npc/stockwell.c).
    # Inventory_GiveAll already selects the fully upgraded native bag.
    if region == 'EU': chosen.remove('SHOP00_BOMBBAG')
    rows = [{'name': n, **flags[n]} for n in chosen]
    # Native Simon reward: roomInit.c sub_StateChange_SimonsSimulation_Main,
    # bank3/C6 marks first reward selection; MAROYA_TAKARA marks its chest.
    rows += [{'name':'SIMON_FIRST_REWARD_SELECTED', 'bank':3, 'index':0xC6, 'bit':0x3C6},
             {'name':'MAROYA_TAKARA', **flags['MAROYA_TAKARA']}]
    if region == 'USA': assert flags['SHOP07_COMPLETE']['bit'] == 0x25F
    assert flags['SEIIKI_STAINED_GLASS']['bank'] == 3
    output.write_text('/* Generated '+region+' completion allow-list. */\nstatic const u16 sCompletionFlags[] = {\n' +
        ''.join(f" 0x{r['bit']:03X}, /* {r['name']} */\n" for r in rows) + '};\n', encoding='ascii')
    output.with_suffix('.json').write_text(json.dumps(rows,indent=2)+'\n')
    print(f'Completion allow-list: {len(rows)} named/explicit bits, 48 native heart pickups (unused pickup excluded).')

if __name__ == '__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--source',type=Path,required=True)
    p.add_argument('--gcc',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--region',choices=('USA','EU','JP'),default='USA')
    a=p.parse_args();generate(a.source,a.gcc,a.output,a.region)
