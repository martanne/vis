-- open file at primary cursor location

local lpeg = vis.lpeg
local l = vis.lexers
local dq_str = l.delimited_range('"', true)
local sq_str = l.delimited_range("'", true)
local include = l.delimited_range("<>", true, true, true)
local filename = dq_str + sq_str + include + (1 - lpeg.S('"\'\t\v\f\r()[]{} \n'))^1

vis:map(vis.modes.NORMAL, "gf", function(keys)
	local mstart, mend = vis.win.file:match_at(filename, vis.win.cursor.pos, 200)
	if not mstart or not mend then
		vis:info("No filename found under the cursor.")
		return #keys
	end
	local fnoffstr = vis.win.file:content(mstart, mend-mstart)
	local offsetcmd
	local fn = fnoffstr
	local offset = fnoffstr:find(":")
	if not offset then
		local offset = fnoffstr:find("/")
	end
	if offset then
		offsetcmd = fnoffstr:sub(offset)
		fn = fnoffstr:sub(1, offset-1)
	end
	local ok = vis:command(string.format("open %s", fn))
	if not ok then
		vis:info("Could not open file " .. fn)
	return #keys
	end
	if offsetcmd then
		vis:command(offsetcmd)
	end
	return #keys
end, "Open file under cursor in a new window")

vis:map(vis.modes.NORMAL, "<C-w>gf", "gf")
