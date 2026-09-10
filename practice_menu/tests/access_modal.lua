local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local UI,PAUSE=0x2032EC0,0x2034490
local function mode() return H.r8(H.PM+H.L.modalMode) end
local function snap()
 local r={}
 for k,v in pairs({menu={0x2000080,0x30},ui={UI,0x3B4},room={H.R,0x38},entities={H.P,0x2C40},
  message={0x2000050,0x20},text={0x2022780,0xA8},bg0={0x2034CB0,0x800},
  font={0x600C000,0x800},map={0x600F800,0x800},pal={0x5000000,0x80},
  nativeCursor={PAUSE,0x18},screen={0x3000F50,0x7C},vram={0x6000000,0x18000},
  fullPalette={0x5000000,0x400},oam={0x7000000,0x400}}) do r[k]=emu:readRange(v[1],v[2]) end
 r.main=emu:readRange(H.M+2,3);r.frames=H.r32(0x30010A0);r.ticks=H.r16(H.M+12)
 -- Native UpdateDisplayControls uploads the already-prepared OAM batch when
 -- OBJ becomes visible again. Compare against that exact batch, not the prior
 -- scanout's stale hardware OAM (notably the animated quest-page icons).
 if H.r8(0x3000000)~=0 and H.r16(0x3000F50)&0x1000~=0 then r.oam=emu:readRange(0x3000020,0x400) end
 return r
end
local function close()
 H.wait(1,H.hotkey())
 for t=1,60 do H.wait(1);if mode()==0 then return end end
 error('modal did not restore')
end
local function roundtrip(name)
 H.wait(5);local before=snap();H.shot(name..'_before')
 local reference=emu:saveStateBuffer();H.wait(1);H.shot(name..'_expected_visible');emu:loadStateBuffer(reference)
 H.wait(12,H.hotkey());H.wait(5)
 H.check(mode()==1 and H.r8(H.PM+13)==1,name..' held hotkey opens modal without close/reopen')
 H.shot(name..'_menu');H.wait(120)
 H.check(emu:readRange(H.M+2,3)==before.main and H.r32(0x30010A0)==before.frames,name..' native task and frame counter paused')
 H.check(H.r16(H.M+12)==before.ticks,name..' native animation clock paused')
 close();local after=snap();H.shot(name..'_restored')
 for k,v in pairs(before) do H.check(after[k]==v,name..' exact restore '..k) end
 H.wait(1);H.shot(name..'_visible');H.wait(19);H.shot(name..'_resumed')
end
local function pauseReady()
 for t=1,220 do
  if H.r8(H.M+4)==7 and H.r8(UI)==2 and H.r8(PAUSE+17)==2 and H.r8(0x3000FD0)==0 then H.wait(3);return end
  H.wait(1)
 end
 error('native pause not ready')
end
H.run(function()
 H.action(14,0) -- give-all fixture makes native map tabs available on this save
 H.press(8);pauseReady()
 H.check(H.r8(UI+2)==1,'real START opens native pause subtask')
 roundtrip('inventory')
 -- Native page switching, no invented subtask or menu state.
 local screens={[H.r8(PAUSE+1)]=true}
 for i=1,3 do H.press(0x100);pauseReady();screens[H.r8(PAUSE+1)]=true;roundtrip('pause_page_'..H.r8(PAUSE+1)) end
 local n=0;for _ in pairs(screens) do n=n+1 end;H.check(n>=3,'controller reached at least three distinct native pause screens')
 -- The nested menu may edit save inventory, not native UI actors or transitions.
 H.menu(15,1);H.check(mode()==1 and H.r8(H.PM+H.L.modalSaveEdits)==1,'native inventory permits save edits')
 local owned=H.item(65);H.press(1);H.check(H.item(65)~=(owned),'element edit applies while native pause is open')
 H.press(1);H.check(H.item(65)==owned,'element edit reversible in nested view')
 H.menu(8,2);local transition=emu:readRange(0x30010A0,0xB0);H.press(1)
 H.check(mode()==1 and H.r8(H.PM+H.L.pendingAction)==0 and emu:readRange(0x30010A0,0xB0)==transition,'nested warp refused without exiting or corrupting native menu')
 close();H.wait(12);H.press(8);H.wait(120)
 H.check(H.r8(H.M+4)==2,'native START still returns to gameplay')
 H.menu(0,0);H.check(mode()==0,'normal gameplay retains existing native Practice subtask');H.close()
 -- Actual title task from a cold ROM reset; no main-state fixture.
 emu:reset();H.wait(180)
 for t=1,800 do if H.r8(H.M+2)==0 and H.r8(H.M+3)==1 and H.r8(0x3000FD0)==0 then break end;H.wait(1) end
 H.check(H.r8(H.M+2)==0,'native title reached after reset')
 roundtrip('logo')
 for t=1,1400 do
  if H.r8(H.M+2)==0 and H.r8(H.M+3)==1 and H.r8(UI+2)==1 and H.r8(0x2000085)==3 and H.r8(0x3000FD0)==0 then break end
  H.wait(1)
 end
 H.check(H.r8(UI+2)==1 and H.r8(0x2000085)==3,'actual animated title reached')
 roundtrip('title')
 H.menu(15,1);local save=emu:readRange(0x2002A40,0x4B4);H.press(1)
 H.check(emu:readRange(0x2002A40,0x4B4)==save and mode()==1,'title view rejects save mutation without active game')
 close();H.boot();H.check(H.r8(H.M+2)==2 and H.r8(H.M+4)==2,'native title/file-select/start still loads supplied save')
end)
