{
static const char* F =
"-----------------------------------------------------------------------------     \n"
"-- Unified SMTP/FTP subsystem                                                     \n"
"-- LuaSocket toolkit.                                                             \n"
"-- Author: Diego Nehab                                                            \n"
"-----------------------------------------------------------------------------     \n"
"                                                                                  \n"
"-----------------------------------------------------------------------------     \n"
"-- Declare module and import dependencies                                         \n"
"-----------------------------------------------------------------------------     \n"
"local base = _G                                                                   \n"
"local string = require('string')                                                  \n"
"local socket = require('socket')                                                  \n"
"local ltn12 = require('ltn12')                                                    \n"
"                                                                                  \n"
"socket.tp = {}                                                                    \n"
"local _M = socket.tp                                                              \n"
"                                                                                  \n"
"-----------------------------------------------------------------------------     \n"
"-- Program constants                                                              \n"
"-----------------------------------------------------------------------------     \n"
"_M.TIMEOUT = 60                                                                   \n"
"                                                                                  \n"
"-----------------------------------------------------------------------------     \n"
"-- Implementation                                                                 \n"
"-----------------------------------------------------------------------------     \n"
"-- gets server reply (works for SMTP and FTP)                                     \n"
"local function get_reply(c)                                                       \n"
"    local code, current, sep                                                      \n"
"    local line, err = c:receive()                                                 \n"
"    local reply = line                                                            \n"
"    if err then return nil, err end                                               \n"
"    code, sep = socket.skip(2, string.find(line, '^(%d%d%d)(.?)'))                \n"
"    if not code then return nil, 'invalid server reply' end                       \n"
"    if sep == '-' then -- reply is multiline                                      \n"
"        repeat                                                                    \n"
"            line, err = c:receive()                                               \n"
"            if err then return nil, err end                                       \n"
"            current, sep = socket.skip(2, string.find(line, '^(%d%d%d)(.?)'))     \n"
"            reply = reply .. '\n' .. line                                         \n"
"        -- reply ends with same code                                              \n"
"        until code == current and sep == ' '                                      \n"
"    end                                                                           \n"
"    return code, reply                                                            \n"
"end                                                                               \n"
"                                                                                  \n"
"-- metatable for sock object                                                      \n"
"local metat = { __index = {} }                                                    \n"
"                                                                                  \n"
"function metat.__index:check(ok)                                                  \n"
"    local code, reply = get_reply(self.c)                                         \n"
"    if not code then return nil, reply end                                        \n"
"    if base.type(ok) ~= 'function' then                                           \n"
"        if base.type(ok) == 'table' then                                          \n"
"            for i, v in base.ipairs(ok) do                                        \n"
"                if string.find(code, v) then                                      \n"
"                    return base.tonumber(code), reply                             \n"
"                end                                                               \n"
"            end                                                                   \n"
"            return nil, reply                                                     \n"
"        else                                                                      \n"
"            if string.find(code, ok) then return base.tonumber(code), reply       \n"
"            else return nil, reply end                                            \n"
"        end                                                                       \n"
"    else return ok(base.tonumber(code), reply) end                                \n"
"end                                                                               \n"
"                                                                                  \n"
"function metat.__index:command(cmd, arg)                                          \n"
"    cmd = string.upper(cmd)                                                       \n"
"    if arg then                                                                   \n"
"        return self.c:send(cmd .. ' ' .. arg.. '\r\n')                            \n"
"    else                                                                          \n"
"        return self.c:send(cmd .. '\r\n')                                         \n"
"    end                                                                           \n"
"end                                                                               \n"
"                                                                                  \n"
"function metat.__index:sink(snk, pat)                                             \n"
"    local chunk, err = c:receive(pat)                                             \n"
"    return snk(chunk, err)                                                        \n"
"end                                                                               \n"
"                                                                                  \n"
"function metat.__index:send(data)                                                 \n"
"    return self.c:send(data)                                                      \n"
"end                                                                               \n"
"                                                                                  \n"
"function metat.__index:receive(pat)                                               \n"
"    return self.c:receive(pat)                                                    \n"
"end                                                                               \n"
"                                                                                  \n"
"function metat.__index:getfd()                                                    \n"
"    return self.c:getfd()                                                         \n"
"end                                                                               \n"
"                                                                                  \n"
"function metat.__index:dirty()                                                    \n"
"    return self.c:dirty()                                                         \n"
"end                                                                               \n"
"                                                                                  \n"
"function metat.__index:getcontrol()                                               \n"
"    return self.c                                                                 \n"
"end                                                                               \n"
"                                                                                  \n"
"function metat.__index:source(source, step)                                       \n"
"    local sink = socket.sink('keep-open', self.c)                                 \n"
"    local ret, err = ltn12.pump.all(source, sink, step or ltn12.pump.step)        \n"
"    return ret, err                                                               \n"
"end                                                                               \n"
"                                                                                  \n"
"-- closes the underlying c                                                        \n"
"function metat.__index:close()                                                    \n"
"    self.c:close()                                                                \n"
"    return 1                                                                      \n"
"end                                                                               \n"
"                                                                                  \n"
"-- connect with server and return c object                                        \n"
"function _M.connect(host, port, timeout, create)                                  \n"
"    local c, e = (create or socket.tcp)()                                         \n"
"    if not c then return nil, e end                                               \n"
"    c:settimeout(timeout or _M.TIMEOUT)                                           \n"
"    local r, e = c:connect(host, port)                                            \n"
"    if not r then                                                                 \n"
"        c:close()                                                                 \n"
"        return nil, e                                                             \n"
"    end                                                                           \n"
"    return base.setmetatable({c = c}, metat)                                      \n"
"end                                                                               \n"
"                                                                                  \n"
"return _M";
if (luaL_loadstring(L, F)==0) lua_call(L, 0, 0);
}