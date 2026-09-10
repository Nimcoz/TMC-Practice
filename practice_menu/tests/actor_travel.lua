local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local function select(p,row)
 assert(H.r32(p+4)~=0 and H.r32(p)~=0,'selected test actor was deleted before operation')
 H.menu(28,row or 0);emu:write32(H.PM+H.L.actorSelected,p);emu:write32(H.PM+H.L.actorIdentity,H.r32(p+8));H.wait(2)
end
local function freeze(p) select(p,0);H.press(1);H.close() end
local function travel(p,row) select(p,row);H.press(1);H.close() end
local function pose(p) return emu:readRange(p+44,12)..string.char(H.r8(p+56)) end
local function hex(s) return (s:gsub('.',function(c)return string.format('%02X',c:byte())end)) end
local function mark(p)
 if p>=0x2033290 and p<0x2033A90 then return H.PM+H.L.actorMarks+80+(p-0x2033290)//64 end
 if p<H.P+8*136 then return H.PM+H.L.actorMarks+72+(p-H.P)//136 end
 return H.PM+H.L.actorMarks+(p-0x30015A0)//136
end
local function pot()
 H.menu(26,1);H.press(1)
 for k,v in pairs({actorRawKind=6,actorRawId=5,actorRawType=0,actorRawType2=0,actorRawTimer=0,actorRawSubtimer=0,actorRawFlags=0,actorRawParent=0,actorRawLayer=1}) do H.option(k,v) end
 H.menu(29,9);H.press(1);H.press(0x80);H.press(0x301);H.close();H.wait(15)
 local p=H.r32(H.PM+H.L.actorSelected);assert(p>=0x30015A0 and p<0x3003BE0,'pot missing');return p
end
H.run(function()
 emu:write8(H.PM+16,1)
 for _,room in ipairs({{0,0,504,504},{0x22,0x10,88,88},{0x48,0,120,120},{0x80,0,120,120}}) do
  H.warp(table.unpack(room));if room[1]==0 then H.openFloor() end
  local e=pot();freeze(e);freeze(H.P)
  local flags=emu:readRange(0x2002C9C,512)
  local linkPose=pose(H.P);travel(e,8)
  H.check(pose(e)==linkPose and pose(H.P)==linkPose,'Actor to Link copies exact XYZ/layer in '..room[1])
  H.wait(90);H.check(pose(e)==linkPose and H.r8(mark(e))&1~=0,'Actor to Link preserves freeze')
  -- Keep the Pot frozen: its native tile-dependent updater breaks it if
  -- unpaused on a relocated non-pot tile. Set fractional fixture while paused,
  -- then real X-edit input captures that exact pose through existing menu code.
  select(e,2);emu:write32(e+44,H.r32(e+44)+24*65536+123);emu:write32(e+48,H.r32(e+48)+8*65536+456)
  emu:write32(e+52,0xFFFC4321);emu:write8(e+56,2);H.press(0x10);H.close()
  local target=pose(e);local state=emu:readRange(e+8,8)
  select(e,7);H.shot('actor_travel_menu_'..room[1]);H.press(1);H.close()
  H.log('POSE target='..hex(target)..' link='..hex(pose(H.P))..' actor='..hex(pose(e))..' status='..emu:readRange(H.PM+H.L.status,28))
  H.check(pose(H.P)==target and pose(e)==target,'Link to Actor copies fractional XYZ and layer '..room[1])
  H.wait(90)
  H.check(pose(H.P)==target and H.r8(mark(H.P))&1~=0,'Link teleport updates frozen Link snapshot')
  H.check(emu:readRange(e+8,8)==state,'teleport does not reset actor identity/action/timer')
  H.check(H.r8(H.R+4)==room[1] and H.r8(H.R+5)==room[2] and emu:readRange(0x2002C9C,512)==flags,'same room and all story flags unchanged')
  travel(H.P,7);travel(H.P,8);H.check(pose(H.P)==target,'self teleport is harmless')
  H.shot('actor_travel_result_'..room[1]);H.action(26,2)
 end
 H.warp(0x48,0,120,120)
 local manager
 for i=0,31 do local p=0x2033290+i*64;if H.r32(p+4)~=0 and H.r8(p+8)==9 then manager=p;break end end
 assert(manager,'native manager missing')
 select(manager,7);local bytes=emu:readRange(manager,64);H.press(1)
 H.check(H.r8(H.PM+13)==1 and emu:readRange(manager,64)==bytes,'manager without XYZ safely rejected')
 H.shot('manager_travel_guard');H.close()
 H.menu(28,7);emu:write32(H.PM+H.L.actorSelected,0x08000000);H.press(1)
 H.check(H.r8(H.PM+13)==1,'invalid pointer rejected without queued teleport');H.close()
 H.warp(0,0,504,504);H.openFloor();local e=pot();select(e,7)
 emu:write32(H.PM+H.L.actorIdentity,H.r32(e+8)~0x100);H.press(1)
 H.check(H.r8(H.PM+13)==1,'stale actor identity rejected');H.close()
 -- Actual native dialogue remains open; relocation is not a disguised Break Free.
 H.warp(0x22,0x11,64,92);local npc
 for i=0,71 do local p=0x30015A0+i*136;if H.r32(p+4)~=0 and H.r8(p+8)==7 then npc=p;break end end
 assert(npc,'house NPC missing');local house=emu:saveStateBuffer();local spoke=false
 for i=0,71 do local p=0x30015A0+i*136;if H.r32(p+4)~=0 and H.r8(p+8)==7 then
  emu:loadStateBuffer(house);emu:write32(H.P+44,H.r32(p+44));emu:write32(H.P+48,H.r32(p+48)+20*65536)
  emu:write8(H.P+20,0);H.wait(6);H.press(0x100);H.wait(90)
  if H.r8(0x2000050)&0x7F~=0 then npc=p;spoke=true;break end
 end end
 if not spoke then H.warp(3,1,336,272);H.wait(200) end
 assert(H.r8(0x2000050)&0x7F~=0,'native dialogue absent')
 -- Fallback changes rooms; find an actual current NPC, never reuse the old handle.
 if not spoke then
  npc=nil;for i=0,71 do local p=0x30015A0+i*136;if H.r32(p+4)~=0 and H.r8(p+8)==7 then npc=p;break end end
  assert(npc,'native intro NPC absent')
 end
 travel(npc,8);H.check(H.r8(0x2000050)&0x7F~=0,'Actor to Link preserves actual textbox')
 travel(npc,7);H.check(H.r8(0x2000050)&0x7F~=0,'Link to Actor preserves actual textbox')
 H.shot('teleport_preserved_textbox');H.action(16,2);H.wait(60)
 H.check(H.r8(0x2000050)&0x7F==0,'existing Break Free still closes textbox after teleport')
end)
