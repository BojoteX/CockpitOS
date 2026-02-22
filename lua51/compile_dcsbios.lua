--[[
  CockpitOS — DCS-BIOS JSON Compiler

  Compiles aircraft LUA modules into JSON documentation files
  using the bundled Lua 5.1 interpreter (no DCS World needed).

  Usage:  lua5.1.exe compile_dcsbios.lua <bios_root> <output_dir>

  bios_root:   Path to DCS-BIOS directory (contains BIOS.lua)
  output_dir:  Path to write aircraft JSON files

  Outputs one line per module: OK <name> or FAIL <name> <error>
  Final line: DONE <compiled> <failed>
]]

local bios_root = arg[1]
local output_dir = arg[2]

if not bios_root or not output_dir then
    io.stderr:write("Usage: lua5.1.exe compile_dcsbios.lua <bios_root> <output_dir>\n")
    os.exit(1)
end

-- Normalize path separators
bios_root = bios_root:gsub("\\", "/"):gsub("/$", "")
output_dir = output_dir:gsub("\\", "/"):gsub("/$", "")

-- Custom loader: remap Scripts.DCS-BIOS.X -> bios_root/X.lua
table.insert(package.loaders, 2, function(modname)
    local remapped = modname:gsub("^Scripts%.DCS%-BIOS%.", "")
    if remapped ~= modname then
        local path = bios_root .. "/" .. remapped:gsub("%.", "/") .. ".lua"
        local f = io.open(path, "r")
        if f then
            f:close()
            return loadfile(path)
        end
    end
end)

-- Stub DCS-specific APIs
lfs = {
    writedir = function() return bios_root .. "/../" end,
    mkdir = function() end,
}
GetDevice = function() return nil end

-- Load JSON encoder
local ok_json, JSON = pcall(require, "Scripts.DCS-BIOS.lib.ext.JSON")
if not ok_json then
    io.stderr:write("FATAL: Could not load JSON encoder: " .. tostring(JSON) .. "\n")
    os.exit(1)
end

-- Skip list (metadata, not aircraft)
local SKIP = {
    CommonData = true, MetadataStart = true, MetadataEnd = true,
    AircraftAliases = true, NS430 = true, SuperCarrier = true, FC3 = true,
}

-- Discover aircraft modules
local aircraft_dir = bios_root .. "/lib/modules/aircraft_modules"
local pipe = io.popen('dir /b "' .. aircraft_dir .. '" 2>nul')
if not pipe then
    io.stderr:write("FATAL: Could not list " .. aircraft_dir .. "\n")
    os.exit(1)
end

local modules = {}
for line in pipe:lines() do
    if line:match("%.lua$") then
        local name = line:gsub("%.lua$", "")
        if not SKIP[name] then
            table.insert(modules, name)
        end
    end
end
pipe:close()
table.sort(modules)

-- Compile each module
local compiled = 0
local failed = 0

for _, name in ipairs(modules) do
    local require_path = "Scripts.DCS-BIOS.lib.modules.aircraft_modules." .. name
    local ok_mod, mod = pcall(require, require_path)

    if ok_mod and mod and mod.documentation then
        local ok_enc, json_str = pcall(function() return JSON:encode_pretty(mod.documentation) end)
        if ok_enc and json_str then
            local outpath = output_dir .. "/" .. (mod.name or name) .. ".json"
            local f, err = io.open(outpath, "w")
            if f then
                f:write(json_str)
                f:close()
                compiled = compiled + 1
                print("OK " .. (mod.name or name))
            else
                failed = failed + 1
                print("FAIL " .. name .. " " .. (err or "write error"))
            end
        else
            failed = failed + 1
            print("FAIL " .. name .. " " .. tostring(json_str))
        end
    else
        failed = failed + 1
        print("FAIL " .. name .. " " .. tostring(mod))
    end
end

print("DONE " .. compiled .. " " .. failed)
