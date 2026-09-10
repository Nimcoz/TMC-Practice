local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local function select(p)
 H.menu(28,0);emu:write32(H.PM+H.L.actorSelected,p);emu:write32(H.PM+H.L.actorIdentity,H.r32(p+8));H.wait(2)
end
local function attemptSpawn()
 H.menu(29,1);H.option('actorSpawn',0);H.press(1);H.wait(80);H.close()
 return emu:readRange(H.PM+H.L.status,28)
end
local function spawn()
 attemptSpawn();local p=H.r32(H.PM+H.L.actorSelected);assert(p>=0x30015A0 and p<0x3003BE0,'native spawn required');return p
end
H.run(function()
 emu:write8(H.PM+16,1)
 H.warp(0,0,504,504);H.openFloor();local e=spawn()
 select(e);local flags=H.r8(e+16)
 emu:write8(e+16,flags|2);H.press(1)
 H.check((H.r8(H.PM+H.L.actorMarks+(e-0x30015A0)//136)&1)==0,'scripted ordinary enemy cannot be frozen')
 H.menu(28,1);H.press(1);H.check(H.r8(H.PM+H.L.page)==28,'scripted enemy cannot be removed');emu:write8(e+16,flags)
 local ef=H.r8(e+0x6D)
 for _,v in ipairs({1,16,32,64}) do
  emu:write8(e+0x6D,ef|v);H.menu(28,1);H.press(1)
  H.check(H.r8(H.PM+H.L.page)==28,'boss/support/captain/monitored flag protected '..v)
 end
 emu:write8(e+0x6D,ef)
 local parent=H.r32(e+80);emu:write32(e+80,H.P);H.press(1)
 H.check(H.r8(H.PM+H.L.page)==28,'outgoing parent relation protected');emu:write32(e+80,parent)
 local child=H.r32(H.P+84);emu:write32(H.P+84,e);H.press(1)
 H.check(H.r8(H.PM+H.L.page)==28,'incoming actor relation protected');emu:write32(H.P+84,child)
 local id=H.r8(e+9);emu:write8(e+9,0x13);emu:write32(H.PM+H.L.actorIdentity,H.r32(e+8));H.press(1)
 H.check(H.r8(H.PM+H.L.page)==28,'boss ID rejected independently of flags')
 emu:write8(e+9,id);emu:write32(H.PM+H.L.actorIdentity,H.r32(e+8))
 emu:write32(H.PM+H.L.actorSelected,0x08000000);H.press(1)
 H.check(H.r8(H.PM+H.L.page)==28,'out-of-pool selected pointer rejected without dereference');select(e)
 emu:write32(H.PM+H.L.actorIdentity,H.r32(e+8)~0x100);H.menu(28,1);H.press(1)
 H.check(H.r8(H.PM+H.L.page)==28,'mismatched actor identity rejected');select(e)
 H.press(1);H.close();H.wait(15)
 H.menu(26,2);H.press(1);H.close()
 H.check((H.r8(H.PM+H.L.actorMarks+(e-0x30015A0)//136)&1)==0,'unfreeze individuals clears only individual freeze')
 H.warp(0,0,504,504);H.openFloor()
 -- Capacity guard fixture: do not actually fill the pool or trigger native eviction.
 H.menu(29,1);local count=H.r8(0x3003DBC);emu:write8(0x3003DBC,64);H.press(1);H.wait(75)
 H.check(emu:readRange(H.PM+H.L.status,28):find('RESERVE FULL')~=nil,'capacity guard refuses before native allocation')
 emu:write8(0x3003DBC,count);H.close()
 -- Face out of bounds on ordinary ground. This is a test position, not a production teleport.
 emu:write32(H.P+44,(H.r16(H.R+6)+8)*65536);emu:write8(H.P+20,6)
 local status=attemptSpawn();H.check(status:find('CLEAR FLOOR')~=nil,'spawn rejects out-of-room target')
 H.warp(0,0,504,504);H.openFloor()
 -- A real native intro/dialog, not synthetic script flags.
 H.warp(3,1,336,272);local message=emu:readRange(0x2000050,32)
 H.menu(29,1);H.press(1);H.shot('scene_spawn_protected')
 H.check(H.r8(H.PM+13)==1 and H.r8(H.PM+H.L.pendingAction)==0,'real scene blocks spawn without closing menu')
 H.close();H.check(H.r8(H.S+139)~=0 or H.r8(0x2000050)~=0,'real native scene remains active after inspect-only menu')
 -- Native reset loses runtime actor flags and never restores a confirmation.
 emu:reset();H.boot();H.check(H.r32(H.PM+H.L.actorSelected)==0 and emu:readRange(H.PM+H.L.actorMarks,72)==string.rep('\0',72),'reset discards actor handles and marks')
 H.menu(27,0);H.option('actorFilter',6);H.wait(2);H.press(1);H.shot('manager_readonly')
 if H.r8(H.PM+H.L.page)==28 then
  H.press(1);H.menu(28,1);H.press(1);H.check(H.r8(H.PM+H.L.page)==28,'native manager inspect only')
 end
 H.close()
end)
