local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local function mark(p)
 if p>=0x2033290 and p<0x2033A90 then return H.PM+H.L.actorMarks+80+(p-0x2033290)//64 end
 if p<0x30015A0 then return H.PM+H.L.actorMarks+72+(p-H.P)//136 end
 return H.PM+H.L.actorMarks+(p-0x30015A0)//136
end
local function select(p,row)
 H.menu(28,row or 0);emu:write32(H.PM+H.L.actorSelected,p);emu:write32(H.PM+H.L.actorIdentity,H.r32(p+8));H.wait(2)
end
local function raw(k,id,t,t2)
 H.menu(26,1);H.press(1)
 H.check(H.r8(H.PM+H.L.actorRaw)==1,'native input opens raw spawner')
 for key,val in pairs({actorRawKind=k,actorRawId=id,actorRawType=t or 0,actorRawType2=t2 or 0,actorRawParent=0}) do H.option(key,val) end
 H.menu(29,9);H.shot('raw_'..k..'_'..id);H.press(1)
 H.check(H.r8(H.PM+H.L.page)==30,'raw spawn requires confirmation')
 H.press(0x80);H.press(1);H.check(H.r8(H.PM+13)==1,'plain A cannot confirm raw spawn')
 H.press(0x301);H.close();H.wait(12)
 return H.r32(H.PM+H.L.actorSelected)
end
local function freeze(p)
 select(p);H.press(1);H.close();H.wait(6)
 H.check((H.r8(mark(p))&1)==1,'individual freeze mark '..string.format('%08X',p))
 local snapshot=emu:readRange(p+12,4)
 local xyz=H.r8(p+8)~=9 and emu:readRange(p+44,12) or nil
 H.wait(90);H.check(emu:readRange(p+12,4)==snapshot and (not xyz or xyz==emu:readRange(p+44,12)),'native action/timer/pose held for 90 frames')
 H.shot('frozen_'..string.format('%08X',p))
 if xyz then
  select(p,2);local x=H.r32(p+44);H.press(0x10)
  H.check(H.r32(p+44)==x+65536,'move frozen actor X through native menu input')
  H.close();H.wait(30);H.check(H.r32(p+44)==x+65536,'edited frozen pose persists')
 end
end
local function remove(p)
 select(p,1);H.press(1);H.check(H.r8(H.PM+H.L.page)==30,'all-kind removal reaches confirmation')
 H.press(0x80);H.press(0x301);H.close();H.wait(10)
 H.check(H.r32(H.PM+H.L.actorSelected)==0 and H.r8(mark(p))==0,'native deletion clears handle and freeze')
end
local function move_unfrozen(p)
 select(p,2);local x=H.r32(p+44);H.press(0x10);H.close();H.wait(10)
 H.check(H.r32(p+44)==x+65536,'unfrozen actor position edit survives menu close '..string.format('%08X',p))
end
H.run(function()
 emu:write8(H.PM+16,1)
 H.warp(0,0,504,504);H.openFloor()
 freeze(H.P);select(H.P);H.press(1);H.close()
 H.check((H.r8(mark(H.P))&1)==0,'Link can be unfrozen using menu while frozen')
 move_unfrozen(H.P)
 local e=raw(3,0,0,0);assert(e~=0,'raw enemy did not allocate');freeze(e);remove(e)
 e=raw(6,5,0,0);assert(e~=0,'raw pot did not allocate');H.check(H.r8(e+9)==5,'non-curated Pot ID initialized');move_unfrozen(e);freeze(e);remove(e)
 e=raw(6,0,0x54,0);assert(e~=0,'raw pickup did not allocate');freeze(e);remove(e)
 -- Genuine loaded NPC and manager, not fabricated kind bytes on an enemy.
 H.warp(0x22,0x11,64,92)
 local npc,manager
 for i=0,71 do local p=0x30015A0+i*136;if H.r32(p+4)~=0 and H.r8(p+8)==7 then npc=p;break end end
 for i=0,31 do local p=0x2033290+i*64;if H.r32(p+4)~=0 and H.r8(p+8)==9 then manager=p;break end end
 H.check(npc~=nil,'native house NPC loaded');if npc then freeze(npc);remove(npc) end
 if not manager then
  H.warp(0x48,0,120,120)
  for i=0,31 do local p=0x2033290+i*64;if H.r32(p+4)~=0 and H.r8(p+8)==9 then manager=p;break end end
 end
 H.check(manager~=nil,'native room manager loaded');if manager then freeze(manager);remove(manager) end
 H.warp(0,0,504,504)
 H.check(emu:readRange(H.PM+H.L.actorMarks,112)==string.rep('\0',112),'room reload clears all 112 marks')
 e=raw(9,54,0,0);H.check(e>=0x2033290 and e<0x2033A90,'raw native manager uses manager pool');assert(e~=0);freeze(e);remove(e)
 -- Reject out-of-pool pointers and stale identities without dereferencing them.
 H.menu(28,1);emu:write32(H.PM+H.L.actorSelected,0x08000000);H.press(1)
 H.check(H.r8(H.PM+H.L.page)==28,'invalid pointer remains rejected')
 H.close();H.shot('expert_complete')
end)
