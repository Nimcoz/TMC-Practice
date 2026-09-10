local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local function prepare(kind,id)
 H.menu(29,0);H.option('actorRaw',1);H.option('actorRawKind',kind);H.option('actorRawId',id)
 H.option('actorRawType',0);H.option('actorRawType2',0);H.option('actorRawParent',0);H.option('actorRawFlags',0)
 H.menu(29,9)
end
local function confirm()
 H.press(1);H.press(0x80);H.press(0x301);H.close();H.wait(8)
end
local function status() return emu:readRange(H.PM+H.L.status,28) end
H.run(function()
 emu:write8(H.PM+16,1);H.warp(0,0,504,504)
 for _,v in ipairs({{3,255},{9,0},{4,37},{6,194},{7,128},{8,25}}) do
  local count=H.r8(0x3003DBC);prepare(v[1],v[2]);confirm()
  H.check(status():find('NO NATIVE HANDLER')~=nil and H.r8(0x3003DBC)==count,'native table boundary rejected '..v[1]..'/'..v[2])
 end
 prepare(6,5);local count=H.r8(0x3003DBC);emu:write8(0x3003DBC,64);confirm()
 local e=H.r32(H.PM+H.L.actorSelected)
 H.check(e>=0x30015A0 and e<0x3003BE0 and H.r8(e+8)==6,'raw allocation has no old 64-count reserve limit')
 emu:write8(0x3003DBC,count+1) -- Restore truthful count after the counter-only fixture.
 H.menu(28,0);emu:write32(H.PM+H.L.actorIdentity,H.r32(e+8)~0x100);H.press(1)
 H.check(H.r8(H.PM+H.L.actorMarks+(e-0x30015A0)//136)&1==0,'stale identity cannot freeze a different actor')
 H.menu(28,1);H.press(1);H.check(H.r8(H.PM+H.L.page)==28,'stale identity cannot open delete confirmation')
 emu:write32(H.PM+H.L.actorSelected,0x8000000);H.press(1)
 H.check(H.r8(H.PM+H.L.page)==28,'ROM pointer cannot be dereferenced as an actor')
 H.close()
 -- Temporarily mark unused slots unavailable, without linking bogus entities.
 -- Raw allocator must report exhaustion and never evict a genuine room actor.
 prepare(6,5);local occupied={};local unchanged={}
 for i=0,71 do local p=0x30015A0+i*136
  if H.r32(p)==0 then occupied[#occupied+1]=p;emu:write32(p,0x3003D68)
  else unchanged[p]=H.r32(p+8) end
 end
 confirm();H.check(status():find('POOL FULL')~=nil,'native pool exhaustion reports failure')
 for _,p in ipairs(occupied) do emu:write32(p,0) end
 for p,id in pairs(unchanged) do H.check(H.r32(p+8)==id,'full-pool attempt did not evict actor '..p) end
 H.warp(0,0,504,504);H.shot('guards_complete')
end)
