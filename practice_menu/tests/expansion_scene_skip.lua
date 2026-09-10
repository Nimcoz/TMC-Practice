local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local baseline=out:find('baseline')~=nil
H.run(function()
 H.warp(3,1,336,272);H.wait(30);H.shot('native_scene_before')
 H.log(string.format('native scene control=%d message=%d',H.r8(H.S+0x8B),H.r8(0x2000050)))
 H.check(H.r8(H.S+0x8B)~=0 or (H.r8(0x2000050)&0x7F)~=0,'real native scripted scene is active')
 -- Both branches pause at the same point through the real menu.
 H.menu(1,10);H.shot('scene_skip_menu')
 if baseline then H.close() else H.press(1);H.close() end
 local elapsed=0
 while elapsed<1800 and (H.r8(H.S+0x8B)~=0 or (H.r8(0x2000050)&0x7F)~=0) do
  if baseline then H.press(1);elapsed=elapsed+6 end
  H.wait(20);elapsed=elapsed+20
 end
 H.log('scene resumed after '..elapsed..' observation frames')
 H.shot('native_scene_after')
 H.check(H.r8(H.S+0x8B)==0 and (H.r8(0x2000050)&0x7F)==0,'native scene finishes and releases control')
 local f=assert(io.open(out..'scene_flags.bin','wb'));f:write(emu:readRange(0x2002C9C,0x200));f:close()
 H.wait(30);local x=H.r32(H.P+44);H.wait(15,0x20);H.wait(8)
 H.check(H.r32(H.P+44)~=x,'normal movement resumes after native scene')
 H.menu(1,10);H.press(1)
 H.check(H.r8(H.PM+13)==1,'skip without a scene does not close menu or advance gameplay')
 H.close()
end)
