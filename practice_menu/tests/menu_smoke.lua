local frame = 0
local phase = "boot"
local phase_frame = 0
local gameplay_frames = 0
local cycle = 0
local last_task = -1
local task_frames = 0
local out_dir = "C:/Users/user/Documents/Codex/2026-08-30/files-mentioned-by-the-user-tmc/practice_menu/test_results/black_menu_fix_framebuffer/"
local log_path = out_dir .. "menu_smoke.log"
local done_path = out_dir .. "menu_smoke.done"

local MAIN = 0x03001000
local INPUT = 0x03000FF0
local PRACTICE = 0x0203D000
local UI = 0x02032EC0

local KEY_A = 0x001
local KEY_B = 0x002
local KEY_SELECT = 0x004
local KEY_START = 0x008
local KEY_UP = 0x040
local KEY_DOWN = 0x080
local KEY_R = 0x100
local KEY_L = 0x200
local HOTKEY = KEY_L + KEY_R + KEY_SELECT

local function u32(value)
    if value < 0 then return value + 4294967296 end
    return value
end

local function log(tag)
    local f = io.open(log_path, "a")
    if not f then return end
    f:write(string.format(
        "%s frame=%d phase=%s pc=%08X task=%02X state=%02X sub=%02X held=%04X pressed=%04X magic=%08X menu=%02X page=%02X noclip=%02X explore=%02X ui=%08X dispcnt=%04X\n",
        tag, frame, phase, u32(emu:readRegister("pc")),
        emu:read8(MAIN + 2), emu:read8(MAIN + 3), emu:read8(MAIN + 4),
        emu:read16(INPUT), emu:read16(INPUT + 2), u32(emu:read32(PRACTICE)),
        emu:read8(PRACTICE + 13), emu:read8(PRACTICE + 23),
        emu:read8(PRACTICE + 14), emu:read8(PRACTICE + 15),
        u32(emu:read32(UI)), emu:read16(0x04000000)))
    f:close()
end

local function set_phase(next_phase)
    phase = next_phase
    phase_frame = frame
    log("PHASE")
end

local function finish(result)
    log(result)
    local f = io.open(done_path, "w")
    if f then
        f:write(result .. "\n")
        f:close()
    end
    phase = "done"
    emu:setKeys(0)
end

callbacks:add("frame", function()
    frame = frame + 1
    if frame == 1 then
        os.remove(log_path)
        os.remove(done_path)
        log("SCRIPT_START")
    end

    local task = emu:read8(MAIN + 2)
    local state = emu:read8(MAIN + 3)
    local substate = emu:read8(MAIN + 4)
    local magic = u32(emu:read32(PRACTICE))
    local menu_open = emu:read8(PRACTICE + 13)

    if task ~= last_task then
        last_task = task
        task_frames = 0
    else
        task_frames = task_frames + 1
    end

    if frame % 120 == 0 then log("TICK") end

    local keys = 0
    if phase == "boot" then
        -- The supplied SRAM boots through the normal title/file-select path.
        if task == 0 and frame >= 650 and ((frame - 650) % 120) < 4 then keys = KEY_START end
        if task == 1 then
            if state == 1 and task_frames >= 120 then emu:write8(MAIN + 3, 2) end
            if task_frames >= 90 and ((task_frames - 90) % 120) < 4 then keys = KEY_A end
        end
        if task == 2 and state == 2 and substate == 2 and magic == 0x504D4331 then
            gameplay_frames = gameplay_frames + 1
            if gameplay_frames >= 45 then
                cycle = 1
                set_phase("open_press")
            end
        end
        if frame > 2400 then finish("FAIL_BOOT_TIMEOUT") end
    elseif phase == "open_press" then
        if frame - phase_frame < 4 then keys = HOTKEY else set_phase("wait_root") end
    elseif phase == "wait_root" then
        if task == 2 and state == 2 and substate == 7 and menu_open == 1 then
            set_phase("root_settle")
        elseif frame - phase_frame > 240 then
            finish("FAIL_MENU_OPEN_TIMEOUT")
        end
    elseif phase == "root_settle" then
        if frame - phase_frame >= 45 then
            if cycle == 1 then
                set_phase("close_press")
            else
                emu:screenshot(out_dir .. "02_menu_root_framebuffer.png")
                set_phase("cursor_down_press")
            end
        end
    elseif phase == "cursor_down_press" then
        if frame - phase_frame < 3 then keys = KEY_DOWN else set_phase("cursor_down_settle") end
    elseif phase == "cursor_down_settle" then
        if frame - phase_frame >= 8 then
            emu:screenshot(out_dir .. "03_menu_cursor_down_framebuffer.png")
            set_phase("cursor_up_press")
        end
    elseif phase == "cursor_up_press" then
        if frame - phase_frame < 3 then keys = KEY_UP else set_phase("cursor_up_settle") end
    elseif phase == "cursor_up_settle" then
        if frame - phase_frame >= 8 then set_phase("practice_press") end
    elseif phase == "practice_press" then
        if frame - phase_frame < 3 then keys = KEY_A else set_phase("wait_practice") end
    elseif phase == "wait_practice" then
        if emu:read8(PRACTICE + 23) == 1 then
            set_phase("practice_settle")
        elseif frame - phase_frame > 90 then
            finish("FAIL_PAGE_NAVIGATION")
        end
    elseif phase == "practice_settle" then
        if frame - phase_frame >= 15 then
            emu:screenshot(out_dir .. "04_menu_practice_framebuffer.png")
            set_phase("back_press")
        end
    elseif phase == "back_press" then
        if frame - phase_frame < 3 then keys = KEY_B else set_phase("wait_back") end
    elseif phase == "wait_back" then
        if emu:read8(PRACTICE + 23) == 0 then
            set_phase("close_press")
        elseif frame - phase_frame > 90 then
            finish("FAIL_BACK_NAVIGATION")
        end
    elseif phase == "close_press" then
        if frame - phase_frame < 4 then keys = HOTKEY else set_phase("wait_close") end
    elseif phase == "wait_close" then
        if task == 2 and state == 2 and substate == 2 and menu_open == 0 then
            set_phase("gameplay_restore_settle")
        elseif frame - phase_frame > 240 then
            finish("FAIL_MENU_CLOSE_TIMEOUT")
        end
    elseif phase == "gameplay_restore_settle" then
        if frame - phase_frame >= 45 then
            if cycle == 1 then
                emu:screenshot(out_dir .. "01_gameplay_framebuffer.png")
                cycle = 2
                set_phase("open_press")
            else
                emu:screenshot(out_dir .. "05_gameplay_restored_framebuffer.png")
                finish("PASS")
            end
        end
    end

    emu:setKeys(keys)
end)
