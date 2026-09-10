local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local UI,MENU,FADE=0x2032EC0,0x2000080,0x3000FD0
local function mode() return H.r8(H.PM+H.L.modalMode) end
local function untilReady(test,limit,label)
 for t=1,limit do if test() then H.wait(3);return end;H.wait(1) end
 error(label..string.format(' main=%d/%d/%d ui=%d/%d menu=%d/%d fade=%d',H.r8(H.M+2),H.r8(H.M+3),H.r8(H.M+4),H.r8(UI),H.r8(UI+2),H.r8(MENU+5),H.r8(MENU+6),H.r8(FADE)))
end
local function exactRoundtrip(name)
 H.wait(4)
 local regs={menu={MENU,0x30},ui={UI,0x3B4},entities={H.P,0x2C40},room={H.R,0x38},
  message={0x2000050,0x20},text={0x2022780,0xA8},bg0={0x2034CB0,0x800},
  font={0x600C000,0x800},map={0x600F800,0x800},pal={0x5000000,0x80},save={0x2002A40,0x4B4},
  screen={0x3000F50,0x7C},vram={0x6000000,0x18000},fullPalette={0x5000000,0x400},oam={0x7000000,0x400}}
 local b={};for k,v in pairs(regs) do b[k]=emu:readRange(v[1],v[2]) end
 local ticks,frames=H.r16(H.M+12),H.r32(0x30010A0)
 if H.r8(0x3000000)~=0 and H.r16(0x3000F50)&0x1000~=0 then b.oam=emu:readRange(0x3000020,0x400) end
 H.shot(name..'_before')
 local reference=emu:saveStateBuffer();H.wait(1);H.shot(name..'_expected_visible');emu:loadStateBuffer(reference)
 H.wait(1,H.hotkey());H.wait(6)
 assert(mode()==1,'modal not opened: '..name);H.check(true,name..' regular hotkey opens');H.shot(name..'_menu')
 H.wait(100);H.check(H.r16(H.M+12)==ticks and H.r32(0x30010A0)==frames,name..' native animation timing paused')
 H.menu(15,1);H.press(1)
 H.check(H.r8(H.PM+H.L.modalSaveEdits)==0 and emu:readRange(0x2002A40,0x4B4)==b.save,name..' save mutation refused outside gameplay')
 H.wait(1,H.hotkey())
 for t=1,30 do H.wait(1);if mode()==0 then break end end
 assert(mode()==0,'modal restore timeout');H.shot(name..'_restored')
 for k,v in pairs(regs) do H.check(emu:readRange(v[1],v[2])==b[k],name..' exact restore '..k) end
 H.check(H.r16(H.M+12)==ticks and H.r32(0x30010A0)==frames,name..' exact timing restore')
 H.wait(1);H.shot(name..'_visible');H.wait(29);H.shot(name..'_resumed')
end
H.run(function()
 -- Re-enter the real opening through its native bedroom RoomInit condition.
 -- Only the copied test save in RAM is changed; no intro PC/state is fabricated.
 H.warp(0x22,0x15,80,80)
 local bank=H.r16(0x2033A94)
 for _,bit in ipairs({0x13,bank+0x46}) do
  local a=0x2002C9C+math.floor(bit/8);emu:write8(a,H.r8(a)&(~(1<<(bit%8))))
 end
 H.log('FIXTURE clear native START/local intro-seen bits; reload bedroom via ordinary room transition')
 emu:write8(0x30010A8,1);emu:write8(0x30010A9,5)
 emu:write8(0x30010AC,0x22);emu:write8(0x30010AD,0x15);emu:write8(0x30010AE,4);emu:write8(0x30010AF,0)
 emu:write16(0x30010B0,80);emu:write16(0x30010B2,80);emu:write8(0x30010B4,1)
 untilReady(function() return H.r8(H.M+4)==7 and H.r8(UI)==2 and H.r8(UI+2)==5 and H.r8(MENU+6)==2 and H.r8(FADE)==0 end,1600,'native story intro missing')
 H.check(true,'native opening story panel reached through room initialization')
 H.wait(65) -- let the original story text blend finish before the visible test
 exactRoundtrip('story_intro')
 -- Advance with the native START skip (existing original-game behavior).
 H.press(8);H.wait(180)
 H.log(string.format('INTRO_CONTINUES ui=%d menu=%d/%d',H.r8(UI+2),H.r8(MENU+5),H.r8(MENU+6)))
 -- Credits fixture enters the real native task at its initialization state;
 -- no credits function, entry pointer or save routine is patched/skipped.
 emu:reset();H.boot()
 H.log('FIXTURE enter native STAFFROLL task/state0 (same bytes as native SetTask)')
 emu:write8(H.M+2,4);emu:write8(H.M+3,0);emu:write8(H.M+4,0)
 untilReady(function() return H.r8(H.M+3)==1 and H.r8(MENU+5)==0 and H.r8(FADE)==0 and H.r8(MENU+26)==1 end,1200,'credits first image missing')
 H.wait(40);exactRoundtrip('credits')
 -- Fast-forward only native waiting timers, never the sequence index. This
 -- executes every normal credits entry, blend and resource load in order.
 local sawEnd=false
 for t=1,9000 do
  local state,kind,overlay=H.r8(H.M+3),H.r8(MENU+5),H.r8(MENU+6)
  if state==2 and overlay==1 and H.r8(FADE)==0 then break end
  if state==1 and kind==2 and overlay==2 and H.r8(FADE)==0 and H.r16(MENU+16)==280 and not sawEnd then
   sawEnd=true;H.log(string.format('END entry=%08X gfx=%d',H.r32(MENU+12),H.r16(MENU+16)));exactRoundtrip('credits_end')
  end
  if state==1 and (kind==0 or kind==3 or (kind==2 and overlay==2)) and H.r16(MENU+8)>2 then emu:write16(MENU+8,2) end
  H.wait(1)
 end
 H.check(sawEnd,'native final Nintendo copyright caption tested before its text fade-out')
 untilReady(function() return H.r8(H.M+3)==2 and H.r8(MENU+6)==1 and H.r8(FADE)==0 end,800,'native end-save prompt missing')
 H.wait(40);exactRoundtrip('ending_save_prompt')
 -- YES exercises the save-write exclusion, on a disposable copied save.
 H.press(1)
 H.check(H.r8(MENU+6)==2,'native ending save actually started')
 H.press(H.hotkey());H.check(mode()==0,'Practice hotkey cannot interrupt native ending EEPROM write')
 H.wait(180);H.boot();H.check(H.r8(H.M+2)==2,'native ending save/reset/load completes after modal access')
end)
