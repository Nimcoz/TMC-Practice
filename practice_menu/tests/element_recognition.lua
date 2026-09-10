local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local F,I=0x2002C9C,0x2002B42
local function raw(item)
 H.menu(26,1);H.press(1)
 for k,v in pairs({actorRawKind=6,actorRawId=0,actorRawType=item,actorRawType2=0,actorRawTimer=0,actorRawSubtimer=0,actorRawFlags=0,actorRawParent=0,actorRawLayer=1}) do H.option(k,v) end
 H.menu(29,9);H.press(1);H.press(0x80);H.press(0x301);H.close();H.wait(70)
 local p=H.r32(H.PM+H.L.actorSelected);assert(p>=0x30015A0 and p<0x3003BE0,'native ground item absent')
 return p
end
local function expected(base,bits) return string.char((base:byte(1)&(~0x6C))|bits)..base:sub(2) end
local function clearElements()
 H.log('FIXTURE clear only element ownership byte and four progression bits')
 emu:write8(I,0);emu:write8(F,H.r8(F)&(~0x6C))
end
local function saveGame()
 H.log(string.format('BEFORE_SAVE action=%d ctrl=%d priority=%d pause=%d flags=%08X message=%d inv0=%d',H.r8(H.P+12),H.r8(H.S+139),H.r8(0x3003DC0),H.r8(0x2034490),H.r32(H.S+48),H.r8(0x2000050),H.item(0)))
 H.press(8);H.wait(65);assert(H.r8(H.M+4)==7,'native pause absent')
 emu:write8(0x2000083,16);H.log('FIXTURE native save-button cursor')
 H.wait(5);H.press(1);H.wait(60);H.press(0x40);H.press(1);H.wait(180)
 H.press(1);H.wait(60);H.press(8);H.wait(90)
 if H.r8(H.M+4)==7 then H.press(8);H.wait(90) end
end
H.run(function()
 emu:write8(H.PM+16,1);H.warp(0,0,504,504);H.openFloor();clearElements()
 local flags=emu:readRange(F,512);local bits=0
 for n,bit in ipairs({2,3,5,6}) do
  local id=63+n;local beforeSpawn=emu:readRange(F,512);local e=raw(id)
  H.check(H.item(id)==0 and emu:readRange(F,512)==beforeSpawn,'spawn alone grants no item/progress '..id)
  H.log('FIXTURE position Link on real ground item for native pickup')
  emu:write32(H.P+44,H.r32(e+44));emu:write32(H.P+48,H.r32(e+48))
  -- CreateItemEntity animates an auxiliary Link. Real Link can remain idle
  -- before the message even appears; do not mistake that for completed pickup.
  H.wait(140)
  for t=1,720 do
   if H.r8(0x2000050)&0x7F~=0 and t%12==0 then H.press(1) else H.wait(1) end
   if H.item(id)>0 and H.r8(0x2000050)&0x7F==0 and H.r8(H.P+12)==1 and H.r8(0x2034490)==0 and H.r8(0x3003DC0)==0 then break end
  end
  bits=bits|(1<<bit);H.shot('collected_element_'..id)
  H.check(H.item(id)==1,'native pickup actually grants element '..id)
  H.check(H.r8(0x2000050)&0x7F==0 and H.r8(0x2034490)==0 and H.r8(0x3003DC0)==0,'native item presentation fully completes '..id)
  H.check(emu:readRange(F,512)==expected(flags,bits),'native pickup sets matching progression flag only '..id)
 end
 -- Direct native flag queries remain editable; no every-frame ownership repair.
 emu:write8(F,H.r8(F)&(~8));H.wait(60)
 H.check(H.item(65)==1 and H.r8(F)&8==0,'raw flag override is not continuously rewritten')
 -- Repair a pre-existing owned/missing-flag mismatch with remove/add in menu.
 H.action(15,1);H.action(15,1)
 H.check(H.item(65)==1 and H.r8(F)&8~=0,'element remove/add repairs older inconsistent ownership')
 -- Removing an uncollected Practice spawn must not award its element/flag.
 H.action(15,1);local e=raw(65);local before=emu:readRange(F,512)
 H.menu(28,1);H.press(1);H.press(0x80);H.press(0x301);H.close();H.wait(30)
 H.check(H.item(65)==0 and emu:readRange(F,512)==before,'manual removal does not award element or progress')
 -- Native timed despawn uses doSetFlag=FALSE, not the collection branch.
 e=raw(65);emu:write16(e+0x6C,1);H.wait(10)
 H.log(string.format('TIMEOUT action=%d remaining=%d priority=%d pause=%d ctrl=%d',H.r8(e+12),H.r16(e+0x6C),H.r8(0x3003DC0),H.r8(0x2034490),H.r8(H.S+139)))
 H.check(H.r32(H.PM+H.L.actorSelected)==0 and H.item(65)==0 and emu:readRange(F,512)==before,'native timeout cleanup does not award element/progress')
 H.action(15,1);H.shot('elements_menu')
 local owned=H.r8(I);local progress=H.r8(F)&0x6C
 saveGame();H.shot('elements_native_saved');emu:reset();H.boot()
 H.check(H.r8(I)==owned and H.r8(F)&0x6C==progress,'native save/reset/load preserves element ownership and progression')
 H.menu(15,1);H.shot('elements_after_reload');H.close()
end)
