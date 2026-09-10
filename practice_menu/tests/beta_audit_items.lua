-- Research fixtures only: no release ROM edits, no native save operation.
local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local function equip(id)
 local p=0x2002B32+(id>>2);local shift=(id%4)*2
 emu:write8(p,(H.r8(p)&~(3<<shift))|(1<<shift))
 emu:write8(0x2002AF4,id);emu:write8(0x2002AF5,0)
 H.log(string.format('FIXTURE inventory ownership and A slot = %02X',id));H.wait(4)
end
local function entity(id)
 for e=0x30011E8,0x3003B58,0x88 do
  if H.r32(e+4)~=0 and H.r8(e+8)==8 and H.r8(e+9)==id then return e end
 end
end
H.run(function()
 H.warp(0,0,504,504);H.openFloor()
 local base=emu:saveStateBuffer()
 equip(0x1C);emu:write8(0x2002AF6,0x21)
 emu:write8(0x2002AEB,80);emu:write8(0x2002AEA,8);emu:write8(H.P+69,8)
 H.wait(5);H.shot('butter_equipped');H.wait(15,1);H.shot('butter_used');H.wait(230)
 H.log('BUTTER health='..H.r8(0x2002AEA)..' content='..H.r8(0x2002AF6))
 H.check(H.r8(0x2002AEA)==80 and H.r8(0x2002AF6)==0x20,'butter is consumed and heals to full health')
 H.shot('butter_healed');local x=H.r32(H.P+44);H.wait(12,0x10);H.wait(5)
 H.check(H.r32(H.P+44)~=x,'movement works after consuming butter')
 for _,id in ipairs({0x16,0x18,0x19,0x1A}) do
  emu:loadStateBuffer(base);H.wait(3);equip(id)
  local save=emu:readRange(0x2002A40,0x4B4)
  H.wait(8,1)
  local e=entity(id==0x16 and 0x0D or (id==0x1A and 0x18 or (id==0x18 and 0x17 or 7)))
  H.log(string.format('ITEM %02X live entity=%s',id,e and string.format('%08X',e) or 'NONE'))
  if id==0x16 or id==0x1A then
   H.check(e~=nil,string.format('item%02X starts native tile-editor entity',id))
   if e then
    local tile=H.r16(e+0x6C)
    H.wait(2,0x21);H.wait(3,1)
    H.log(string.format('EDITOR %02X tile %04X -> %04X',id,tile,H.r16(e+0x6C)))
    H.check(H.r16(e+0x6C)==(tile+1)%65536,'native editor increments selected tile via A+LEFT')
    H.shot(string.format('editor_%02X_held',id))
   end
  else H.check(e==nil,string.format('orb%02X has no persistent player-item entity',id)) end
  H.wait(12);H.shot(string.format('item_%02X_released',id))
  H.check(not entity(id==0x16 and 0x0D or 0x18) and (H.r8(H.S+0x1A)&0x80)==0,'item release clears editor entity and mobility lock')
  H.check(emu:readRange(0x2002A40,0x4B4)==save,'item use leaves native save data unchanged')
 end
 emu:loadStateBuffer(base);H.wait(3)
 -- The map asset labelled Unused10 is at actual area62 room10, not room05.
 local headers=H.r32(0x811E214+0x62*4);local header=headers+0x10*10
 H.log(string.format('ROOM62/10 header=%08X width=%d height=%d',header,H.r16(header+4),H.r16(header+6)))
 if H.r16(header)~=0xFFFF and H.r16(header+4)>0 and H.r16(header+6)>0 then
  H.warp(0x62,0x10,120,104);H.shot('map_62_10_candidate')
  H.check(H.r8(H.R+4)==0x62 and H.r8(H.R+5)==0x10,'additional Unused10-labelled room loads through native transition')
 end
 emu:loadStateBuffer(base);H.wait(4)
 H.check(H.r8(H.M+4)==2,'research fixture returns to original playable state')
end)
