{
static const char* F =
"-----------------------------------------------------------------------------      \n"
"-- FTP support for the Lua language                                                \n"
"-- LuaSocket toolkit.                                                              \n"
"-- Author: Diego Nehab                                                             \n"
"-----------------------------------------------------------------------------      \n"
"                                                                                   \n"
"-----------------------------------------------------------------------------      \n"
"-- Declare module and import dependencies                                          \n"
"-----------------------------------------------------------------------------      \n"
"local base = _G                                                                    \n"
"local table = require('table')                                                     \n"
"local string = require('string')                                                   \n"
"local math = require('math')                                                       \n"
"local socket = require('socket')                                                   \n"
"local url = require('socket.url')                                                  \n"
"local tp = require('socket.tp')                                                    \n"
"local ltn12 = require('ltn12')                                                     \n"
"socket.ftp = {}                                                                    \n"
"local _M = socket.ftp                                                              \n"
"-----------------------------------------------------------------------------      \n"
"-- Program constants                                                               \n"
"-----------------------------------------------------------------------------      \n"
"-- timeout in seconds before the program gives up on a connection                  \n"
"_M.TIMEOUT = 60                                                                    \n"
"-- default port for ftp service                                                    \n"
"_M.PORT = 21                                                                       \n"
"-- this is the default anonymous password. used when no password is                \n"
"-- provided in url. should be changed to your e-mail.                              \n"
"_M.USER = 'ftp'                                                                    \n"
"_M.PASSWORD = 'anonymous@anonymous.org'                                            \n"
"                                                                                   \n"
"-----------------------------------------------------------------------------      \n"
"-- Low level FTP API                                                               \n"
"-----------------------------------------------------------------------------      \n"
"local metat = { __index = {} }                                                     \n"
"                                                                                   \n"
"function _M.open(server, port, create)                                             \n"
"    local tp = socket.try(tp.connect(server, port or _M.PORT, _M.TIMEOUT, create)) \n"
"    local f = base.setmetatable({ tp = tp }, metat)                                \n"
"    -- make sure everything gets closed in an exception                            \n"
"    f.try = socket.newtry(function() f:close() end)                                \n"
"    return f                                                                       \n"
"end                                                                                \n"
"                                                                                   \n"
"function metat.__index:portconnect()                                               \n"
"    self.try(self.server:settimeout(_M.TIMEOUT))                                   \n"
"    self.data = self.try(self.server:accept())                                     \n"
"    self.try(self.data:settimeout(_M.TIMEOUT))                                     \n"
"end                                                                                \n"
"                                                                                   \n"
"function metat.__index:pasvconnect()                                               \n"
"    self.data = self.try(socket.tcp())                                             \n"
"    self.try(self.data:settimeout(_M.TIMEOUT))                                     \n"
"    self.try(self.data:connect(self.pasvt.ip, self.pasvt.port))                    \n"
"end                                                                                \n"
"                                                                                   \n"
"function metat.__index:login(user, password)                                       \n"
"    self.try(self.tp:command('user', user or _M.USER))                             \n"
"    local code, reply = self.try(self.tp:check{'2..', 331})                        \n"
"    if code == 331 then                                                            \n"
"        self.try(self.tp:command('pass', password or _M.PASSWORD))                 \n"
"        self.try(self.tp:check('2..'))                                             \n"
"    end                                                                            \n"
"    return 1                                                                       \n"
"end                                                                                \n"
"                                                                                   \n"
"function metat.__index:pasv()                                                      \n"
"    self.try(self.tp:command('pasv'))                                              \n"
"    local code, reply = self.try(self.tp:check('2..'))                             \n"
"    local pattern = '(%d+)%D(%d+)%D(%d+)%D(%d+)%D(%d+)%D(%d+)'                     \n"
"    local a, b, c, d, p1, p2 = socket.skip(2, string.find(reply, pattern))         \n"
"    self.try(a and b and c and d and p1 and p2, reply)                             \n"
"    self.pasvt = {                                                                 \n"
"        ip = string.format('%d.%d.%d.%d', a, b, c, d),                             \n"
"        port = p1*256 + p2                                                         \n"
"    }                                                                              \n"
"    if self.server then                                                            \n"
"        self.server:close()                                                        \n"
"        self.server = nil                                                          \n"
"    end                                                                            \n"
"    return self.pasvt.ip, self.pasvt.port                                          \n"
"end                                                                                \n"
"                                                                                   \n"
"function metat.__index:port(ip, port)                                              \n"
"    self.pasvt = nil                                                               \n"
"    if not ip then                                                                 \n"
"        ip, port = self.try(self.tp:getcontrol():getsockname())                    \n"
"        self.server = self.try(socket.bind(ip, 0))                                 \n"
"        ip, port = self.try(self.server:getsockname())                             \n"
"        self.try(self.server:settimeout(_M.TIMEOUT))                               \n"
"    end                                                                            \n"
"    local pl = math.mod(port, 256)                                                 \n"
"    local ph = (port - pl)/256                                                     \n"
"    local arg = string.gsub(string.format('%s,%d,%d', ip, ph, pl), '%.', ',')      \n"
"    self.try(self.tp:command('port', arg))                                         \n"
"    self.try(self.tp:check('2..'))                                                 \n"
"    return 1                                                                       \n"
"end                                                                                \n"
"                                                                                   \n"
"function metat.__index:send(sendt)                                                 \n"
"    self.try(self.pasvt or self.server, 'need port or pasv first')                 \n"
"    -- if there is a pasvt table, we already sent a PASV command                   \n"
"    -- we just get the data connection into self.data                              \n"
"    if self.pasvt then self:pasvconnect() end                                      \n"
"    -- get the transfer argument and command                                       \n"
"    local argument = sendt.argument or                                             \n"
"        url.unescape(string.gsub(sendt.path or '', '^[/\\]', ''))                  \n"
"    if argument == '' then argument = nil end                                      \n"
"    local command = sendt.command or 'stor'                                        \n"
"    -- send the transfer command and check the reply                               \n"
"    self.try(self.tp:command(command, argument))                                   \n"
"    local code, reply = self.try(self.tp:check{'2..', '1..'})                      \n"
"    -- if there is not a a pasvt table, then there is a server                     \n"
"    -- and we already sent a PORT command                                          \n"
"    if not self.pasvt then self:portconnect() end                                  \n"
"    -- get the sink, source and step for the transfer                              \n"
"    local step = sendt.step or ltn12.pump.step                                     \n"
"    local readt = {self.tp.c}                                                      \n"
"    local checkstep = function(src, snk)                                           \n"
"        -- check status in control connection while downloading                    \n"
"        local readyt = socket.select(readt, nil, 0)                                \n"
"        if readyt[tp] then code = self.try(self.tp:check('2..')) end               \n"
"        return step(src, snk)                                                      \n"
"    end                                                                            \n"
"    local sink = socket.sink('close-when-done', self.data)                         \n"
"    -- transfer all data and check error                                           \n"
"    self.try(ltn12.pump.all(sendt.source, sink, checkstep))                        \n"
"    if string.find(code, '1..') then self.try(self.tp:check('2..')) end            \n"
"    -- done with data connection                                                   \n"
"    self.data:close()                                                              \n"
"    -- find out how many bytes were sent                                           \n"
"    local sent = socket.skip(1, self.data:getstats())                              \n"
"    self.data = nil                                                                \n"
"    return sent                                                                    \n"
"end                                                                                \n"
"                                                                                   \n"
"function metat.__index:receive(recvt)                                              \n"
"    self.try(self.pasvt or self.server, 'need port or pasv first')                 \n"
"    if self.pasvt then self:pasvconnect() end                                      \n"
"    local argument = recvt.argument or                                             \n"
"        url.unescape(string.gsub(recvt.path or '', '^[/\\]', ''))                  \n"
"    if argument == '' then argument = nil end                                      \n"
"    local command = recvt.command or 'retr'                                        \n"
"    self.try(self.tp:command(command, argument))                                   \n"
"    local code,reply = self.try(self.tp:check{'1..', '2..'})                       \n"
"    if (code >= 200) and (code <= 299) then                                        \n"
"        recvt.sink(reply)                                                          \n"
"        return 1                                                                   \n"
"    end                                                                            \n"
"    if not self.pasvt then self:portconnect() end                                  \n"
"    local source = socket.source('until-closed', self.data)                        \n"
"    local step = recvt.step or ltn12.pump.step                                     \n"
"    self.try(ltn12.pump.all(source, recvt.sink, step))                             \n"
"    if string.find(code, '1..') then self.try(self.tp:check('2..')) end            \n"
"    self.data:close()                                                              \n"
"    self.data = nil                                                                \n"
"    return 1                                                                       \n"
"end                                                                                \n"
"                                                                                   \n"
"function metat.__index:cwd(dir)                                                    \n"
"    self.try(self.tp:command('cwd', dir))                                          \n"
"    self.try(self.tp:check(250))                                                   \n"
"    return 1                                                                       \n"
"end                                                                                \n"
"                                                                                   \n"
"function metat.__index:type(type)                                                  \n"
"    self.try(self.tp:command('type', type))                                        \n"
"    self.try(self.tp:check(200))                                                   \n"
"    return 1                                                                       \n"
"end                                                                                \n"
"                                                                                   \n"
"function metat.__index:greet()                                                     \n"
"    local code = self.try(self.tp:check{'1..', '2..'})                             \n"
"    if string.find(code, '1..') then self.try(self.tp:check('2..')) end            \n"
"    return 1                                                                       \n"
"end                                                                                \n"
"                                                                                   \n"
"function metat.__index:quit()                                                      \n"
"    self.try(self.tp:command('quit'))                                              \n"
"    self.try(self.tp:check('2..'))                                                 \n"
"    return 1                                                                       \n"
"end                                                                                \n"
"                                                                                   \n"
"function metat.__index:close()                                                     \n"
"    if self.data then self.data:close() end                                        \n"
"    if self.server then self.server:close() end                                    \n"
"    return self.tp:close()                                                         \n"
"end                                                                                \n"
"                                                                                   \n"
"-----------------------------------------------------------------------------      \n"
"-- High level FTP API                                                              \n"
"-----------------------------------------------------------------------------      \n"
"local function override(t)                                                         \n"
"    if t.url then                                                                  \n"
"        local u = url.parse(t.url)                                                 \n"
"        for i,v in base.pairs(t) do                                                \n"
"            u[i] = v                                                               \n"
"        end                                                                        \n"
"        return u                                                                   \n"
"    else return t end                                                              \n"
"end                                                                                \n"
"                                                                                   \n"
"local function tput(putt)                                                          \n"
"    putt = override(putt)                                                          \n"
"    socket.try(putt.host, 'missing hostname')                                      \n"
"    local f = _M.open(putt.host, putt.port, putt.create)                           \n"
"    f:greet()                                                                      \n"
"    f:login(putt.user, putt.password)                                              \n"
"    if putt.type then f:type(putt.type) end                                        \n"
"    f:pasv()                                                                       \n"
"    local sent = f:send(putt)                                                      \n"
"    f:quit()                                                                       \n"
"    f:close()                                                                      \n"
"    return sent                                                                    \n"
"end                                                                                \n"
"                                                                                   \n"
"local default = {                                                                  \n"
"    path = '/',                                                                    \n"
"    scheme = 'ftp'                                                                 \n"
"}                                                                                  \n"
"                                                                                   \n"
"local function parse(u)                                                            \n"
"    local t = socket.try(url.parse(u, default))                                    \n"
"    socket.try(t.scheme == 'ftp', 'wrong scheme '' .. t.scheme .. ''')             \n"
"    socket.try(t.host, 'missing hostname')                                         \n"
"    local pat = '^type=(.)$'                                                       \n"
"    if t.params then                                                               \n"
"        t.type = socket.skip(2, string.find(t.params, pat))                        \n"
"        socket.try(t.type == 'a' or t.type == 'i',                                 \n"
"            'invalid type '' .. t.type .. ''')                                     \n"
"    end                                                                            \n"
"    return t                                                                       \n"
"end                                                                                \n"
"                                                                                   \n"
"local function sput(u, body)                                                       \n"
"    local putt = parse(u)                                                          \n"
"    putt.source = ltn12.source.string(body)                                        \n"
"    return tput(putt)                                                              \n"
"end                                                                                \n"
"                                                                                   \n"
"_M.put = socket.protect(function(putt, body)                                       \n"
"    if base.type(putt) == 'string' then return sput(putt, body)                    \n"
"    else return tput(putt) end                                                     \n"
"end)                                                                               \n"
"                                                                                   \n"
"local function tget(gett)                                                          \n"
"    gett = override(gett)                                                          \n"
"    socket.try(gett.host, 'missing hostname')                                      \n"
"    local f = _M.open(gett.host, gett.port, gett.create)                           \n"
"    f:greet()                                                                      \n"
"    f:login(gett.user, gett.password)                                              \n"
"    if gett.type then f:type(gett.type) end                                        \n"
"    f:pasv()                                                                       \n"
"    f:receive(gett)                                                                \n"
"    f:quit()                                                                       \n"
"    return f:close()                                                               \n"
"end                                                                                \n"
"                                                                                   \n"
"local function sget(u)                                                             \n"
"    local gett = parse(u)                                                          \n"
"    local t = {}                                                                   \n"
"    gett.sink = ltn12.sink.table(t)                                                \n"
"    tget(gett)                                                                     \n"
"    return table.concat(t)                                                         \n"
"end                                                                                \n"
"                                                                                   \n"
"_M.command = socket.protect(function(cmdt)                                         \n"
"    cmdt = override(cmdt)                                                          \n"
"    socket.try(cmdt.host, 'missing hostname')                                      \n"
"    socket.try(cmdt.command, 'missing command')                                    \n"
"    local f = _M.open(cmdt.host, cmdt.port, cmdt.create)                           \n"
"    f:greet()                                                                      \n"
"    f:login(cmdt.user, cmdt.password)                                              \n"
"    f.try(f.tp:command(cmdt.command, cmdt.argument))                               \n"
"    if cmdt.check then f.try(f.tp:check(cmdt.check)) end                           \n"
"    f:quit()                                                                       \n"
"    return f:close()                                                               \n"
"end)                                                                               \n"
"                                                                                   \n"
"_M.get = socket.protect(function(gett)                                             \n"
"    if base.type(gett) == 'string' then return sget(gett)                          \n"
"    else return tget(gett) end                                                     \n"
"end)                                                                               \n"
"                                                                                   \n"
"return _M";
if (luaL_loadstring(L, F)==0) lua_call(L, 0, 0);
}