local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 H.warp(3,1,336,272)
 local function state(s) H.log(s..' xy='..H.r16(H.P+46)..','..H.r16(H.P+50)..' action='..H.r8(H.P+12)..' ctrl='..H.r8(H.S+139)..' flags='..H.r32(H.S+48)..' message='..H.r8(0x2000050)..' playerprio='..H.r8(H.P+17)..' event='..H.r8(0x3003DC0)..' ent='..H.r8(0x3003DC1)..' frame='..H.r8(H.S+168)..' dir='..H.r8(H.P+21)) end
 state('before');H.action(16,2);state('after')
 for t=1,12 do H.wait(1,0x20);state('input '..t) end
 H.shot('after')
end)
