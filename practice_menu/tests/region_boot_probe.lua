local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local f=assert(io.open(out..'trace.log','w'));local n,last,done=0,'',false
callbacks:add('frame',function()
 if done then return end;n=n+1
 local t,s,b=emu:read8(0x3001002),emu:read8(0x3001003),emu:read8(0x3001004)
 local state=string.format('%d/%d/%d fade=%d menu=%d/%d/%d lang=%d',t,s,b,emu:read8(0x3000FD0),emu:read8(0x2000081),emu:read8(0x2000085),emu:read8(0x2000086),emu:read8(0x2000007))
 if state~=last then f:write(n..' '..state..'\n');f:flush();last=state end
 if n==800 or n==1600 or n==2590 then emu:screenshot(out..'boot_'..n..'.png') end
 local k=0
 if t==0 and n>650 and n%120<4 then k=8 end
 if t==1 and n%120<4 then k=(emu:read8(0x080000AF)==80 and emu:read8(0x02019EE6)>0) and 0x40 or 1 end
 emu:setKeys(k)
 if n>=2600 or (t==2 and s==2 and b==2) then
  f:close();emu:screenshot(out..'boot_final.png');local d=assert(io.open(out..'done.txt','w'));d:write('BOOT '..state);d:close();done=true
 end
end)
