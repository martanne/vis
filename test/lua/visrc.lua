package.path = '../../lua/?.lua;'..package.path

dofile("../../lua/vis.lua")

local function str(e)
	if type(e) == "string" then
		if e == "" then
			return "an empty string"
		else
			return '"'..e..'"'
		end
	else
		return tostring(e)
	end
end


local function same(a, b)
	if type(a) ~= type(b) then
		return string.format("expected %s - got %s", type(a), type(b))
	end

	if type(a) ~= 'table' then
		if a == b then
			return nil
		else
			return string.format("expected %s - got %s", str(a), str(b))
		end
	end

	if #a ~= #b then
		return string.format("expected table of size %d got %d", #a, #b)
	end

	for k, v in pairs(a) do
		if b[k] == nil then
			return string.format("expected %s got nil", str(k))
		end
		r = same(v, b[k])
		if r ~= nil then
			return r
		end
	end
	return nil
end


local msg = ""
function describe(s, fn)
	group = {
		before_each = function()
		end,
		tests = {},
		after_each = function()
		end,

	}
	function before_each(fn) group.before_each = fn end
	function after_each(fn) group.after_each = fn end
	function it(s, fn)
		table.insert(group.tests, {
			s = s,
			fn = fn,
			assertions = {},
		})
	end
	fn()
	for j, t in pairs(group.tests) do
		group.before_each()
		assert = {
			has_error = function(fn)
				local status, err = pcall(fn)
				if err == nil then
					msg = msg..string.format("%s %s: expected error\n", s, t.s)
				end
			end,
			truthy = function(stat)
				if not (stat) then
					msg = msg..string.format("%s %s: expected to be truthy: %s\n", s, t.s, str(stat))
				end
			end,
			falsy = function(stat)
				if (stat) then
					msg = msg..string.format("%s %s: expected to be falsy: %s\n", s, t.s, str(stat))
				end
			end,
			are = {
				equal = function(a, b)
					if a ~= b then
						msg = msg..string.format("%s %s: expected %s - got %s\n", s, t.s, str(a), str(b))
					end
				end,
				same = function(a, b)
					r = same(a, b) -- same returns a string which is a reason why a & b are not equal
					if r ~= nil then
						msg = msg..string.format("%s %s: %s\n", s, t.s, r)
					end
				end,
			},
		}
		t.fn()
		group.after_each()
	end
end


vis.events.subscribe(vis.events.WIN_OPEN, function(win)
	-- test.in file passed to vis
	local in_file = win.file.name
	if not in_file then
		return
	end

	-- use the corresponding test.lua file
	lua_file = string.gsub(in_file, '%.in$', '.lua')

	local ok, err = pcall(dofile, lua_file)
	if not ok then
		print(tostring(err))
		vis:exit(2) -- ERROR
	elseif msg ~= "" then
		io.write(msg) -- no newline
		vis:exit(1) -- FAIL
	else
		vis:exit(0) -- SUCCESS
	end
end)
