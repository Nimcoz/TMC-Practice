local out = assert(os.getenv('TMC_TEST_OUTPUT'))
local f = assert(io.open(out .. 'trace.log', 'w'))
local n, ready, start = 0, 0, nil
local P, S, M, PM = 0x03001160, 0x03003F80, 0x03001000, 0x0203D000
local function log(tag)
    f:write(string.format('%s frame=%d main=%d/%d/%d room=%02X/%02X p=%02X/%02X xy=%d,%d anim=%04X skills=%04X charge=%02X sword=%02X flags=%08X equip=%d,%d active=%02X/%02X/%02X pc=%08X\n', tag,n,
        emu:read8(M+2),emu:read8(M+3),emu:read8(M+4),emu:read8(0x03000BF4),emu:read8(0x03000BF5),
        emu:read8(P+12),emu:read8(P+13),emu:read16(P+46),emu:read16(P+50),emu:read16(S+8),emu:read16(S+172),emu:read8(S+160),emu:read8(S+27),emu:read32(S+48),emu:read8(0x02002AF4),emu:read8(0x02002AF5),
        emu:read8(0x03000B80),emu:read8(0x03000B81),emu:read8(0x03000B82),emu:readRegister('pc')))
    f:flush()
end
local function done(s)
    log(s); f:close(); local d=assert(io.open(out..'done.txt','w')); d:write(s); d:close(); start=-100000; emu:setKeys(0)
end
callbacks:add('frame',function()
    n=n+1; local keys=0
    if not start then
        if emu:read8(M+2)==0 and n>650 and n%120<4 then keys=8 end
        if emu:read8(M+2)==1 and n%120<4 then keys=1 end
        if emu:read8(M+2)==2 and emu:read8(M+3)==2 and emu:read8(M+4)==2 then
            ready=ready+1; if ready>45 then start=n; log('BOOTED_NATIVE'); emu:screenshot(out..'00_before.png') end
        end
        if n>2500 then done('FAIL_BOOT') end
    elseif start>=0 then
        local t=n-start
        if t<4 then keys=0x304 end
        if t==35 then
            if emu:read8(PM+13)~=1 then done('FAIL_MENU') return end
            emu:write8(PM+23,14); emu:write8(PM+24+14,0)
        end
        if t>=40 and t<43 then keys=1 end
        if t==50 then log('GIVE_ALL'); emu:screenshot(out..'01_given.png') end
        if t>=60 and t<64 then keys=0x304 end
        if t>=110 and t<140 then keys=0x10 end
        if t>=160 and t<440 then keys=1 end
        if t>=500 and t<640 and t%24<4 then keys=1 end
        if t>=660 and t<690 then keys=2 end
        if t>=720 and t<750 then keys=0x20 end
        if t==120 or t==180 or t==250 or t==350 or t==460 or t==650 or t==750 then emu:screenshot(out..string.format('%04d.png',t)) end
        if t%10==0 then log('TICK') end
        if t==800 then done('PROBE_COMPLETE') end
    end
    emu:setKeys(keys)
end)
