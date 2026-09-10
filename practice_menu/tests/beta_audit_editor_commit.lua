-- Disposable RAM-only investigation of the native unused editor path.
local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 H.warp(0,0,504,504);H.openFloor();emu:write8(H.P+20,0)
 local base=emu:saveStateBuffer()
 local item=0x16;local p=0x2002B32+(item>>2);local shift=(item%4)*2
 emu:write8(p,(H.r8(p)&~(3<<shift))|(1<<shift));emu:write8(0x2002AF4,item);emu:write8(0x2002AF5,0)
 local mx=((H.r32(H.P+44)>>16)-H.r16(H.R+6))>>4
 local my=(((H.r32(H.P+48)>>16)-H.r16(H.R+8))-16)>>4
 local index=(mx&63)|((my&63)<<6)
 local layer=H.r8(H.P+56)==2 and 0x200B650 or 0x2025EB0
 local pos=layer+4+index*2;local original=H.r16(pos)
 local before=emu:readRange(layer+4,8192)
 local save=emu:readRange(0x2002A40,0x4B4)
 H.wait(8,1);H.wait(2,0x21);H.wait(3,1);H.wait(12)
 H.log(string.format('NATIVE EDIT map[%03X] at %08X: %04X -> %04X',index,pos,original,H.r16(pos)))
 local changed={}
 for i=0,4095 do
  local old=before:byte(i*2+1)|(before:byte(i*2+2)<<8)
  if old~=H.r16(layer+4+i*2) then
   changed[#changed+1]={index=i,old=old}
   H.log(string.format('MAP DIFFERENCE %03X: %04X -> %04X',i,old,H.r16(layer+4+i*2)))
  end
 end
 H.log('layer='..H.r8(H.P+56)..' final facing='..H.r8(H.P+20)..' changed cells='..#changed)
 H.check(#changed>0,'native editor commits an actual loaded-map tile change on release')
 H.check(emu:readRange(0x2002A40,0x4B4)==save,'tile edit does not change native save bytes')
 H.shot('committed_tile_editor');H.action(1,2);H.wait(160)
 local restored=#changed>0
 for _,entry in ipairs(changed) do if H.r16(layer+4+entry.index*2)~=entry.old then restored=false end end
 H.check(restored,'native room reload discards the unsaved editor tile change')
 emu:loadStateBuffer(base);H.wait(4);H.check(H.r8(H.M+4)==2,'research state restored')
end)
