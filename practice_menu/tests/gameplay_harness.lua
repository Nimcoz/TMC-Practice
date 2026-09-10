local H={};H.out=assert(os.getenv('TMC_TEST_OUTPUT'))
H.L=assert(loadfile(H.out..'layout.lua'))()
H.PM,H.P,H.S,H.M,H.R=0x203D000,0x3001160,0x3003F80,0x3001000,0x3000BF0
local f=assert(io.open(H.out..'trace.log','w'));local n,ready,co,done,fail=0,0,nil,false,0
function H.log(s) f:write(string.format('%05d %s\n',n,s));f:flush() end
function H.r8(a) return emu:read8(a) end
function H.r16(a) return emu:read16(a) end
function H.r32(a) return emu:read32(a) end
function H.wait(count,k) for i=1,count do coroutine.yield(k or 0) end end
function H.press(k) H.wait(2,k);H.wait(4) end
function H.check(ok,s) H.log((ok and 'PASS ' or 'FAIL ')..s);if not ok then fail=fail+1 end;return ok end
function H.shot(s) emu:screenshot(H.out..s..'.png') end
function H.option(name,v) emu:write8(H.PM+assert(H.L[name],name),v) end
function H.hotkey() return H.L.menuHotkey and H.r16(H.PM+H.L.menuHotkey) or 0x304 end
function H.menu(page,row)
 if H.r8(H.PM+13)==0 then H.press(H.hotkey());H.wait(30) end
 assert(H.r8(H.PM+13)==1,'menu failed to open')
 H.option('page',page);emu:write8(H.PM+H.L.cursor+page,row or 0);H.wait(2)
end
function H.close()
 if H.r8(H.PM+13)==1 then H.press(H.hotkey()) end
 for i=1,150 do
  if H.r8(H.M+3)==2 and H.r8(H.M+4)==2 and H.r8(0x3000FD0)==0 then H.wait(4);return end
  H.wait(1)
 end
 error('menu failed to return to gameplay')
end
function H.action(page,row) H.menu(page,row);H.press(1);H.close() end
function H.warp(a,r,x,y,settle)
 H.log(string.format('FIXTURE native transition %02X/%02X local %d,%d',a,r,x,y))
 emu:write8(0x30010A8,1);emu:write8(0x30010A9,5)
 emu:write8(0x30010AC,a);emu:write8(0x30010AD,r);emu:write8(0x30010AE,4);emu:write8(0x30010AF,0)
 emu:write16(0x30010B0,x);emu:write16(0x30010B2,y);emu:write8(0x30010B4,1)
 H.wait(settle or 170);H.shot('warp_'..a..'_'..r);assert(H.r8(H.R+4)==a and H.r8(H.R+5)==r,string.format('fixture room did not load; actual=%02X/%02X main=%d/%d/%d fade=%d progress=%d',H.r8(H.R+4),H.r8(H.R+5),H.r8(H.M+2),H.r8(H.M+3),H.r8(H.M+4),H.r8(0x3000FD0),H.r8(0x2002A48)))
end
function H.item(i) return (H.r8(0x2002B32+math.floor(i/4)) >> ((i%4)*2))&3 end
function H.surface(act)
 for p=0x8007CAC,0x8007DC0,4 do
  local key=H.r16(p);if key==0 then return 0 end
  if key==act then return H.r16(p+2) end
 end
 return 0
end
function H.openFloor()
 local cp,ap=H.r32(0x800024C),H.r32(0x800027C)
 local ox,oy,w,h=H.r16(H.R+6),H.r16(H.R+8),H.r16(H.R+30),H.r16(H.R+32)
 for ty=6,math.floor(h/16)-7 do for tx=5,math.floor(w/16)-13 do
  local clear=true
  for dx=0,10 do for dy=-1,0 do local j=(ty+dy)*64+tx+dx;if H.r8(cp+j)~=0 or H.surface(H.r8(ap+j))~=0 then clear=false end end end
  if clear then
   emu:write32(H.P+44,(ox+tx*16+8)*65536);emu:write32(H.P+48,(oy+ty*16+8)*65536)
   H.log('FIXTURE position at verified open native floor');H.wait(35);return
  end
 end end
 error('no open floor fixture')
end
function H.boot()
 local good=0
 for i=1,2600 do
  local k=0
  if H.r8(H.M+2)==0 and i>650 and i%120<4 then k=8 end
  if H.r8(H.M+2)==1 and i%120<4 then k=1 end
  if H.r8(H.M+2)==2 and H.r8(H.M+3)==2 and H.r8(H.M+4)==2 then good=good+1;if good>45 then return end end
  H.wait(1,k)
 end
 error('native boot timeout')
end
function H.run(fn)
 co=coroutine.create(function() H.boot();H.log('NATIVE_BOOT with supplied save');fn();H.log('RESULT failures='..fail) end)
 callbacks:add('frame',function()
  if done then return end;n=n+1
  local ok,k=coroutine.resume(co)
  if not ok then fail=fail+1;H.log('FAIL HARNESS '..tostring(k)) end
  if not ok or coroutine.status(co)=='dead' then
   done=true;f:close();local d=assert(io.open(H.out..'done.txt','w'));d:write('GAMEPLAY_CHECKS failures='..fail);d:close();emu:setKeys(0)
  else emu:setKeys(k or 0) end
 end)
end
return H
