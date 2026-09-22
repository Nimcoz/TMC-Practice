-- Read-only invariant probes around the EU native ARM entity traversal.
-- Only the explicitly labelled photo-coordinate fixture writes player X/Y;
-- breakpoints never repair, skip, or mutate native traversal or its context.
local H=assert(loadfile(assert(os.getenv('TMC_HARDWARE_HARNESS'))))()
local D,CTX=0x0203F000,0x03003DD0
local enabled=false
local stats,lastCall,tag
local guarded=H.r32(0x080B177C)==0xE51FF004
local iteratorGuarded=H.r32(0x080B172C)==0xE51FF004
local entrySite=guarded and 0x03005F90 or 0x03005F94
local pendingEntry,pendingReturn
local pendingIteratorEntry,pendingIteratorExit,iteratorFrame
local function reg(n) return emu:readRegister(n)&0xFFFFFFFF end
local function machine()
 local m={sp=reg('sp'),lr=reg('lr'),cpsr=reg('cpsr'),r={}}
 for n=0,12 do m.r[n]=reg('r'..n) end
 return m
end
local function contextSnapshot()
 local c={};for n=0,3 do c[n]=H.r32(CTX+n*4) end;return c
end
local function slot(a)
 if a>=0x03001160 and a<0x03003BE0 then return (a-0x03001160)%0x88==0 end
 if a>=0x02033290 and a<0x02033A90 then return (a-0x02033290)%0x40==0 end
 return false
end
local function sentinel(a) return a>=0x03003D70 and a<=0x03003DB0 and (a-0x03003D70)%8==0 end
local function node(a) return slot(a) or sentinel(a) end
local function state()
 local s={r0=reg('r0'),r4=reg('r4'),r7=reg('r7'),r8=reg('r8'),r9=reg('r9'),r10=reg('r10'),r11=reg('r11'),sp=reg('sp'),lr=reg('lr')}
 s.table=H.r32(CTX);s.head=H.r32(CTX+4);s.actor=H.r32(CTX+8);s.astk=H.r32(CTX+12)
 s.next=node(s.r0) and H.r32(s.r0+4) or 0xBADC0DE
 return s
end
local function violation(site,reason,s)
 stats.bad=stats.bad+1
 if stats.bad<=12 then
  H.log(string.format('VIOLATION %s %s %s r0=%08X next=%08X r4=%08X r7=%08X r8=%08X r9=%08X r10=%08X r11=%08X sp=%08X lr=%08X context=%08X/%08X/%08X/%08X room=%02X/%02X xy=%d,%d stage=%X',
   tag,site,reason,s.r0,s.next,s.r4,s.r7,s.r8,s.r9,s.r10,s.r11,s.sp,s.lr,s.table,s.head,s.actor,s.astk,H.r8(H.R+4),H.r8(H.R+5),H.r16(H.P+46),H.r16(H.P+50),H.r32(D+8)))
 end
end
local function check(ok,site,reason,s) if not ok then violation(site,reason,s) end end
local function preserved(saved,site,expectedLR)
 if not saved then return end
 local actual=machine();local s=state()
 for n=0,12 do check(actual.r[n]==saved.r[n],site,'shim preserves r'..n,s) end
 check(actual.sp==saved.sp,site,'shim restores complete64-byte stack frame',s)
 check(actual.lr==expectedLR,site,'shim preserves/replays LR',s)
 check(actual.cpsr==saved.cpsr,site,'shim preserves complete CPSR including flags',s)
end
local function common(site,s)
 check(s.r7==0x020000B0,site,'enemy-target register',s)
 check(s.r11==CTX and s.r10==0x03005FBC,site,'context/table register',s)
 check(s.table==0x0800274C or s.table==0x0800275C,site,'native traversal descriptor',s)
 local bound=s.table==0x0800275C and 0x03003DB8 or 0x03003DB0
 check(s.r9==bound and sentinel(s.r8) and s.r8<s.r9,site,'native list bounds',s)
 check(s.head==s.r8,site,'context list head equals r8',s)
 check(s.sp==s.astk and (s.sp&3)==0 and s.sp>=0x03006C14 and s.sp<0x03007F00,site,'native SP equals saved context SP',s)
end

emu:setBreakpoint(function()
 if not enabled then return end
 stats.calls=stats.calls+1
 local s=state();common('call',s)
 check(slot(s.r4) and s.r0==s.r4 and s.actor==s.r4,'call','actor argument/current actor',s)
 if not guarded then check(s.lr==0x03005F98,'call','ARM callback return address',s) end
 if slot(s.r4) then
  local kind=H.r8(s.r4+8)
  stats.kinds[kind]=(stats.kinds[kind] or 0)+1
 end
 lastCall=s
 -- D4 entry is before displaced MOV lr,pc; independently sample all registers
 -- on the first128 and each256th event to bound debugger overhead.
 if guarded then pendingEntry=(stats.calls<=128 or stats.calls%256==0) and machine() or nil end
end,entrySite)

emu:setBreakpoint(function()
 if not enabled then return end
 stats.returns=stats.returns+1
 local s=state();common('return',s)
 check(slot(s.r4),'return','preserved actor slot',s)
 if lastCall then
  check(s.r4==lastCall.r4 and s.sp==lastCall.sp,'return','callback preserves actor and SP',s)
 end
 if guarded then pendingReturn=(stats.returns<=128 or stats.returns%256==0) and machine() or nil end
end,0x03005F98)

if guarded then
 local entryShim=H.r32(0x080B1780)
 local returnShim=H.r32(0x080B1788)
 -- Check the assembled preimages before relying on the post-restore offsets.
 assert(H.r32(entryShim+0x38)==0xE12FFF11,'D4 entry post-restore BX r1')
 assert(H.r32(returnShim+0x34)==0xE59B0008,'D4 return post-restore LDR r0,[r11,#8]')
 emu:setBreakpoint(function()
  if not enabled then return end
  if pendingEntry then stats.shimEntries=stats.shimEntries+1;preserved(pendingEntry,'entry-shim',0x03005F98);pendingEntry=nil end
 end,entryShim+0x38)
 emu:setBreakpoint(function()
  if not enabled then return end
  if pendingReturn then stats.shimReturns=stats.shimReturns+1;preserved(pendingReturn,'return-shim',pendingReturn.lr);pendingReturn=nil end
 end,returnShim+0x34)
end

if iteratorGuarded then
 local iteratorEntryShim=H.r32(0x080B1730)
 local iteratorExitShim=H.r32(0x080B179C)
 -- Stop after LDMIA restores all64 bytes and before replay changes r1 or r0.
 assert(H.r32(iteratorEntryShim+0x30)==0xE8BD5FFF,'D5 entry complete register restore')
 assert(H.r32(iteratorEntryShim+0x34)==0xE59F1008,'D5 entry post-restore descriptor LDR')
 assert(H.r32(iteratorExitShim+0x30)==0xE8BD5FFF,'D5 exit complete register restore')
 assert(H.r32(iteratorExitShim+0x34)==0xE3A00000,'D5 exit post-restore MOV r0,#0')
 local function snapshotIterator()
  local saved=machine();saved.context=contextSnapshot();return saved
 end
 local function preservedIterator(saved,site)
  if not saved then return end
  preserved(saved,site,saved.lr)
  local actual=contextSnapshot();local s=state()
  for n=0,3 do check(actual[n]==saved.context[n],site,'observer leaves native context word'..n..' untouched',s) end
 end
 emu:setBreakpoint(function()
  if not enabled then return end
  stats.iteratorEntries=stats.iteratorEntries+1
  local s=state()
  check(s.r0<=1,'iterator-entry','native traversal mode is0 or1',s)
  check(not iteratorFrame,'iterator-entry','no independent nested traversal',s)
  iteratorFrame={sp=s.sp,mode=s.r0,lr=s.lr}
  pendingIteratorEntry=(stats.iteratorEntries<=128 or stats.iteratorEntries%256==0) and snapshotIterator() or nil
 end,0x03005F40)
 emu:setBreakpoint(function()
  if not enabled then return end
  stats.iteratorExits=stats.iteratorExits+1
  if iteratorFrame then
   local s=state()
   check(s.sp+36==iteratorFrame.sp,'iterator-exit','native nine-register frame matches entry SP including delete unwinds',s)
  end
  iteratorFrame=nil
  pendingIteratorExit=(stats.iteratorExits<=128 or stats.iteratorExits%256==0) and snapshotIterator() or nil
 end,0x03005FAC)
 emu:setBreakpoint(function()
  if not enabled then return end
  if pendingIteratorEntry then
   stats.iteratorShimEntries=stats.iteratorShimEntries+1
   preservedIterator(pendingIteratorEntry,'iterator-entry-shim');pendingIteratorEntry=nil
  end
 end,iteratorEntryShim+0x34)
 emu:setBreakpoint(function()
  if not enabled then return end
  if pendingIteratorExit then
   stats.iteratorShimExits=stats.iteratorShimExits+1
   preservedIterator(pendingIteratorExit,'iterator-exit-shim');pendingIteratorExit=nil
  end
 end,iteratorExitShim+0x34)
end

emu:setBreakpoint(function()
 if not enabled then return end
 stats.nexts=stats.nexts+1
 local s=state();common('next',s)
 check(node(s.r0) and s.r0==s.actor,'next','valid current/predecessor node',s)
 check(node(s.next),'next','next pointer is allocated slot or list sentinel',s)
end,0x03005FA4)

H.run(function()
 H.log(string.format('PROBES guarded=%s native entry=%08X return=03005F98 next=03005FA4',tostring(guarded),entrySite))
 H.log(string.format('ITERATOR PROBES guarded=%s entry=03005F40 exit=03005FAC',tostring(iteratorGuarded)))
 local base=emu:saveStateBuffer()
 local totalDeleteUnwinds=0
 local cases={
  {name='south_off',on=false,key=0x80},
  {name='south_on',on=true,key=0x80},
  {name='southwest_on',on=true,key=0xA0},
  {name='southeast_on',on=true,key=0x90},
  {name='west_wall_on',on=true,key=0x20},
  {name='east_wall_on',on=true,key=0x10},
  {name='photo_position_idle',on=true,key=0,photo=true},
  {name='photo_position_southwest',on=true,key=0xA0,photo=true},
 }
 for _,case in ipairs(cases) do
  enabled=false;emu:loadStateBuffer(base);emu:setKeys(0);H.wait(3)
  tag=case.name;stats={bad=0,calls=0,returns=0,nexts=0,kinds={},shimEntries=0,shimReturns=0,iteratorEntries=0,iteratorExits=0,iteratorShimEntries=0,iteratorShimExits=0};lastCall=nil;pendingEntry=nil;pendingReturn=nil
  pendingIteratorEntry=nil;pendingIteratorExit=nil;iteratorFrame=nil;enabled=true
  H.menu(9,0);if case.on then H.press(1) end;H.close()
  if case.photo then
   -- Explicit synthetic coordinate setup, not a claim of a controller-only path.
   H.log('FIXTURE setting exact photographed room 03/06 X=1989 Y=965; no actor/context edits')
   H.check(H.r8(H.R+4)==3 and H.r8(H.R+5)==6,tag..' uses original castle-entrance room')
   emu:write32(H.P+0x2C,1989*65536);emu:write32(H.P+0x30,965*65536)
   H.wait(1);H.shot(tag..'_start')
  end
  for n=1,600 do
   H.wait(1,case.key)
   if n%120==0 then
    H.log(string.format('STEP %s n=%d room=%02X/%02X xy=%d,%d callbacks=%d violations=%d trip=%d',tag,n,H.r8(H.R+4),H.r8(H.R+5),H.r16(H.P+46),H.r16(H.P+50),stats.calls,stats.bad,H.r32(D+24)))
   end
   if stats.bad>0 or H.r32(D+24)~=0 then break end
  end
  H.wait(120);enabled=false
  local kinds={};for k=0,9 do if stats.kinds[k] then kinds[#kinds+1]=k..':'..stats.kinds[k] end end
  H.log(string.format('SUMMARY %s calls=%d returns=%d nexts=%d violations=%d kinds=%s room=%02X/%02X xy=%d,%d',tag,stats.calls,stats.returns,stats.nexts,stats.bad,table.concat(kinds,','),H.r8(H.R+4),H.r8(H.R+5),H.r16(H.P+46),H.r16(H.P+50)))
  H.check(stats.calls>0 and stats.returns>0 and stats.nexts>0,tag..' all three native probes observed')
  if guarded then
   H.log(string.format('SHIM PRESERVATION %s entries=%d returns=%d full r0-r12/SP/LR/CPSR snapshots',tag,stats.shimEntries,stats.shimReturns))
   H.check(stats.shimEntries>0 and stats.shimReturns>0,tag..' both 64-byte shims independently checked')
  end
  if iteratorGuarded then
   local deleteUnwinds=stats.calls-stats.returns
   totalDeleteUnwinds=totalDeleteUnwinds+deleteUnwinds
   H.log(string.format('ITERATOR PRESERVATION %s entries=%d exits=%d shimEntries=%d shimExits=%d deleteUnwinds=%d full r0-r12/SP/LR/CPSR/context snapshots error=%d',tag,stats.iteratorEntries,stats.iteratorExits,stats.iteratorShimEntries,stats.iteratorShimExits,deleteUnwinds,H.r32(0x0203F310)))
   H.check(stats.iteratorEntries>0 and stats.iteratorExits>0 and stats.iteratorShimEntries>0 and stats.iteratorShimExits>0,tag..' both full-iterator shims independently checked')
   H.check(H.r32(0x0203F310)==0,tag..' iterator trace has no terminal error')
  end
  H.check(stats.bad==0,tag..' native actor/list/register/SP invariants')
  H.check(H.r32(D+24)==0,tag..' no watchdog capture')
  H.shot(tag..'_end')
 end
 if iteratorGuarded then H.check(totalDeleteUnwinds>0,'suite exercises native nonreturning deletion with valid iterator exits') end
end)
