
local M

--[[
	statusbar

	On each keypress, a function runs that changes the statusbar
	the statusbar is divided into 2 sides, left and right
	Each side works the same, you have a list of functions and if the function returns
	a string, it will be concatenated into a list and displayed at the end of the iteration

	In order to not have plugins fight over control of the statusbar
	the statusbar controlling function takes a modular approach
	using a dictionary containing functions, which either return nil|string
	if nil, nothing happens, if string, the string will be added into a list
	at the end of the iteration, the list is is concatenated and displayed on the statusbar

	TODO: control the statusbar list via :set commands
--]]

local modes = {
	[vis.modes.NORMAL] = 'NORMAL',
	[vis.modes.OPERATOR_PENDING] = 'OPERATOR',
	[vis.modes.VISUAL] = 'VISUAL',
	[vis.modes.VISUAL_LINE] = 'VISUAL-LINE',
	[vis.modes.INSERT] = 'INSERT',
	[vis.modes.REPLACE] = 'REPLACE',
}


local writers = { --: dictionary
	--[name: string]: function(WINDOW): string|nil
	-- because we use gsub, we always return an empty string
	filename = function (win)
		local file = win.file
		return (file.basename or '[No Name]')
			.. (file.modified and ' [+]' or '')
			.. (vis.recording and ' @' or '')
	end,
	theme = function (win) return vis.theme end,
	filetype = function (win)
		local w, f = win.syntax, win.file.syntax
		return w==f and w or ('%s -> %s'):format(w, f)
	end,
	mode = function () return modes[vis.mode] end,
	countorkeys = function (win)
		local count = vis.count
		local keys = vis.input_queue
		if keys ~= '' then
			return keys
		elseif count then
			return count
		end
	end,
	selections = function (win)
		local l = #win.selections
		if l > 1 then return win.selection.number .. '/' .. l end
	end,
	position = function (win)
		local size = win.file.size
		return (
			size == 0 and "0"
			or math.ceil( (win.selection.pos or 0)/size*100)
		) .. "%"
	end,
	column = function (win)
		if not win.column then
			local sel = win.selection
			return sel.line .. ', ' .. sel.col
		end
	end,
}


-- Todo: make statusbar be controllable via :set
-- leftside and rightside is a lua string which is loaded()?

M = {
	writers = writers -- dictionary of functions
,	left = "mode filename"
,	right = "theme filetype countorkeys selections position column"
}

local global_statusbar_mt = {__index=M, __newindex=rawset}
local win_statusbar_mt = {__index=writers, __newindex=rawset}
local concat = table.concat

M.Catenator = function (win)
	local bar = win.statusbar
	local left_parts = {}
	local right_parts = {}
	local i
		i = 1 for _, F in ipairs(bar.leftT) do
			local str = F(win)
			if str then i, left_parts[i] = i+1, str end
		end
		i = 1 for _, F in ipairs(bar.rightT) do
			local str = F(win)
			if str then i, right_parts[i] = i+1, str end
		end
	return ' ' .. concat(left_parts, " » ") .. ' '
		, ' ' .. concat(right_parts, " « ") .. ' '
end

vis.events.subscribe(vis.events.WIN_STATUS, function(win)
	if win.statusbar==nil then -- WIN_STATUS runs before WIN_OPEN, so this will be nil initially
		win.statusbar = setmetatable(
			{
				writers = setmetatable({}, win_statusbar_mt) --> win.statusbar.writers -> writers
			} , global_statusbar_mt
		)
	end

	local bar = win.statusbar

	-- cachebuilder
		if bar.leftcachestr~=bar.left then
			bar.leftcachestr = bar.left
			bar.leftT = {}
			-- -> {mode, filename}
			for writer in bar.left:gmatch'%l+' do
				local Writer = bar.writers[writer]
				if Writer then
					table.insert(bar.leftT, Writer)
				else
					vis:message"statusbar.left is malformed"
					break
				end
			end
		end
		if bar.rightcachestr~=bar.right then
			bar.rightcachestr = bar.right
			bar.rightT = {}
			-- -> {mode, filename}
			for writer in bar.right:gmatch'%l+' do
				local Writer = bar.writers[writer]
				if Writer then
					table.insert(bar.rightT, Writer)
				else
					vis:message"statusbar.right is malformed"
					break
				end
			end
		end

	win:status( M.Catenator(win) )
end)

return M
