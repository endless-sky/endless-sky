local luatest = {}

function luatest.es_init()
    es_debug("Loaded plugin luatest.lua :)")
end

function luatest.es_daily()
    es_addMsg("A day has passed from lua", 0)
end

return luatest
