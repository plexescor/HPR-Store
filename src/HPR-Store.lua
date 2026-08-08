HPR.extensionName = "HPR Store"
HPR.authorName = "Plexescor"
HPR.versionSupport = { "v0.9.7" }

local initializer, err
local destroy, err1
local showUi, err2

function init()
    HPR.log(HPR.extensionName, HPR.extensionName .. " Initialized")
    print("Initialized")

    local extDir = (HPR.getExtensionAbsoluteDir ~= nil) and HPR.getExtensionAbsoluteDir() or HPR.getExtensionDir()

    local candidates = {}
    if HPR.getOsName() == "Windows" then
        candidates = {
            extDir .. "HPR-Store.dll",
            extDir .. "libHPR-Store.dll"
        }
    else
        candidates = {
            extDir .. "HPR-Store.so",
            extDir .. "libHPR-Store.so"
        }
    end

    HPR.log(HPR.extensionName, "Extension dir: " .. extDir)
    HPR.log(HPR.extensionName, "package.loadlib = " .. tostring(package.loadlib))

    local dllPath = candidates[1]
    for _, path in ipairs(candidates) do
        HPR.log(HPR.extensionName, "Attempting to load library: " .. path)
        initializer, err = package.loadlib(path, "initialize")
        destroy, err1 = package.loadlib(path, "destroy")
        showUi, err2 = package.loadlib(path, "showUi")
        if initializer and destroy and showUi then
            dllPath = path
            break
        end
    end

    HPR.log(HPR.extensionName, "Selected DLL path: " .. dllPath)
    HPR.log(HPR.extensionName, "initializer = " .. tostring(initializer))
    HPR.log(HPR.extensionName, "error = " .. tostring(err))

    HPR.log(HPR.extensionName, "destroy = " .. tostring(destroy))
    HPR.log(HPR.extensionName, "error = " .. tostring(err1))

    HPR.log(HPR.extensionName, "special = " .. tostring(showUi))
    HPR.log(HPR.extensionName, "error = " .. tostring(err2))

    if not initializer or not destroy then
        return
    end

    HPR.log(HPR.extensionName, "Calling initialize...")
    initializer()
    HPR.log(HPR.extensionName, "initialize returned successfully")

end

function onTick()

end

function onExit()
    destroy()
end

function onAction()
    if showUi then
        showUi()
    end
end