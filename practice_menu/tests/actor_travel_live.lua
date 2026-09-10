local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local function pose(p) return emu:readRange(p+44,12)..string.char(H.r8(p+56)) end
local function selected(e,row)
 assert(H.r32(e+4)~=0,'live test actor absent');H.menu(28,row)
 emu:write32(H.PM+H.L.actorSelected,e);emu:write32(H.PM+H.L.actorIdentity,H.r32(e+8));H.wait(2)
end
local function atCommit(e,row)
 selected(e,row);local identity=H.r32(e+8);local action=H.r8(e+12);local health=H.r8(e+69)
 H.wait(1,1)
 for t=1,180 do
  H.wait(1)
  if H.r8(H.PM+H.L.pendingAction)==0 and H.r8(H.PM+13)==0 and H.r8(H.M+4)==2 then
   H.check(pose(e)==pose(H.P),'unfrozen teleport exact XYZ/layer on actual commit frame, row '..row)
   H.check(H.r32(e+8)==identity and H.r8(e+12)==action and H.r8(e+69)==health,'teleport preserves actor identity/action/health')
   H.shot('live_commit_'..row);return
  end
 end
 error('teleport action never committed')
end
H.run(function()
 emu:write8(H.PM+16,1);H.warp(0,0,504,504);H.openFloor()
 H.menu(26,1);H.press(1)
 for k,v in pairs({actorRawKind=3,actorRawId=0,actorRawType=0,actorRawType2=0,actorRawTimer=0,actorRawSubtimer=0,actorRawFlags=0,actorRawParent=0,actorRawLayer=1}) do H.option(k,v) end
 H.menu(29,9);H.press(1);H.press(0x80);H.press(0x301);H.close();H.wait(40)
 local e=H.r32(H.PM+H.L.actorSelected);assert(e>=0x30015A0 and e<0x3003BE0,'native Octorok absent')
 local m=H.PM+H.L.actorMarks+(e-0x30015A0)//136
 local flags=emu:readRange(0x2002C9C,512)
 atCommit(e,8);H.check(H.r8(m)&1==0,'Actor to Link does not introduce a freeze')
 local s=emu:readRange(e+12,4)..pose(e);H.wait(150)
 H.check(emu:readRange(e+12,4)..pose(e)~=s,'unfrozen Octorok continues native behavior')
 atCommit(e,7);H.check(H.r8(m)&1==0,'Link to Actor leaves unfrozen target active')
 H.wait(20);local p=pose(H.P);H.wait(12,0x20);H.wait(3)
 H.check(pose(H.P)~=p,'normal controller movement resumes after Link teleport')
 H.check(H.r8(H.R+4)==0 and H.r8(H.R+5)==0 and emu:readRange(0x2002C9C,512)==flags,'both live teleports preserve room and story flags')
 -- A freeze is retained when only the target, not Link, was frozen.
 selected(e,0);H.press(1);H.close();atCommit(e,7)
 H.check(H.r8(m)&1~=0 and H.r8(H.PM+H.L.actorMarks+72)&1==0,'frozen target remains frozen without freezing Link')
 H.action(26,2)
end)
