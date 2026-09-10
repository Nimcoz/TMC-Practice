local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local UI,PAUSE,MENU=0x2032EC0,0x2034490,0x2000080
local function mode() return H.r8(H.PM+H.L.modalMode) end
local function modal(name)
 H.wait(5);local ui=emu:readRange(UI,0x3B4);local menu=emu:readRange(MENU,0x30)
 local pal=emu:readRange(0x5000000,0x80);local font=emu:readRange(0x600C000,0x800);local map=emu:readRange(0x600F800,0x800)
 H.shot(name..'_before');H.wait(12,H.hotkey());H.wait(6)
 assert(mode()==1,'cannot open modal: '..name);H.check(true,name..' opens and stays open with held hotkey');H.shot(name..'_menu')
 H.wait(30);H.wait(1,H.hotkey())
 for i=1,30 do H.wait(1,H.hotkey());if mode()==0 then break end end
 H.check(mode()==0 and emu:readRange(UI,0x3B4)==ui and emu:readRange(MENU,0x30)==menu,name..' native UI and cursor restored')
 H.check(emu:readRange(0x5000000,0x80)==pal and emu:readRange(0x600C000,0x800)==font and emu:readRange(0x600F800,0x800)==map,name..' native graphics restored exactly')
 H.wait(12,H.hotkey());H.check(mode()==0,name..' held closing hotkey does not reopen');H.wait(12);H.shot(name..'_resumed')
end
H.run(function()
 H.action(14,0);H.press(8);H.wait(65)
 emu:write8(MENU+3,16);H.log('FIXTURE native pause cursor selects SAVE button');H.wait(5);H.press(1);H.wait(60)
 H.check(H.r8(PAUSE+1)==11 and H.r8(MENU+5)==1,'native save confirmation reached by A button')
 modal('native_save_prompt')
 H.press(0x40);H.press(1)
 H.check(H.r8(MENU+5)==2,'native pause save write started');H.press(H.hotkey())
 H.check(mode()==0,'native pause EEPROM write cannot be interrupted');H.wait(180)
 H.shot('native_save_finished');H.press(1);H.wait(60);H.press(8);H.wait(90)
 if H.r8(H.M+4)==7 then H.press(8);H.wait(90) end
 H.check(H.r8(H.M+4)==2,'native save confirmation still returns to gameplay')
 H.warp(0x48,0,120,120);H.press(8);H.wait(70)
 local found=false
 for i=1,5 do
  if H.r8(PAUSE+1)==5 then found=true;break end
  H.press(0x100);H.wait(55)
 end
 H.check(found,'native dungeon map reached with shoulder buttons');assert(found,'dungeon map not available')
 modal('dungeon_map');H.press(8);H.wait(90)
 H.warp(0x23,7,120,120);H.menu(16,9);H.press(1);H.press(0x40);H.press(1);H.close()
 local viewer
 for i=0,71 do local e=H.P+0x440+i*0x88
  if H.r32(e+4)~=0 and H.r8(e+8)==6 and H.r8(e+9)==0x22 and H.r8(e+10)==0 then viewer=e end
 end
 assert(viewer,'native figurine viewer missing');H.log('FIXTURE position below actual figurine viewer')
 emu:write32(H.P+44,H.r32(viewer+44));emu:write32(H.P+48,H.r32(viewer+48)+20*65536);emu:write8(H.P+20,0)
 H.wait(8);H.press(0x100);H.wait(80)
 H.check(H.r8(H.M+4)==7 and H.r8(UI+2)==7,'native figurine list entered by interaction');modal('figurine_list')
 H.press(0x80);H.press(0x10);H.wait(15);modal('figurine_description')
 H.press(2);H.wait(80);H.check(H.r8(H.M+4)==2,'figurine viewer closes back to same gameplay')
 emu:reset();H.boot();H.check(H.item(6)==1,'supplied save copy still reloads with explicit native inventory save')
end)
