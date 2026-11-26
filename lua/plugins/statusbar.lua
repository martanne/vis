--- modular status bar plugin for vis
--
-- This modules provides a modular statusbar.
-- It exports its module table as vis.statusbar.
-- The table consists of an predefined actions table and tables for the right
-- and the left parts of the status line.
--
-- Predefined actions are:
-- * filename - the name of the file opened in the window
-- * mode - the current mode of the window
-- * countorkeys - the current command prefix
-- * selections - the number of the primary selection and the selection count
-- * position - the position of the primary selection in percent
-- * location - the line and column of the primary selection
--
-- To include some text in the statusbar simply add a function returning
-- the desired text to one of the parts tables. 
-- @usage table.insert(vis.statusbar.left, function() return "hello" end)
--
-- Each function in a parts table is called with the the current window and
-- the returned strings are concatenated using a " » " or " « " respectively.
--
-- The default left and right tables are:
-- left = {mode, filename}
-- right = {countorkeys, selections, position, location}
--
-- @author Nomarian <nomarian@outlook.com>
-- @author Florian Fischer <muhq@muhq.space>

local modes = {
	[vis.modes.NORMAL] = '',
	[vis.modes.OPERATOR_PENDING] = '',
	[vis.modes.VISUAL] = 'VISUAL',
	[vis.modes.VISUAL_LINE] = 'VISUAL-LINE',
	[vis.modes.INSERT] = 'INSERT',
	[vis.modes.REPLACE] = 'REPLACE',
}

--- A table containing predefined parts of the statusbar
local actions = {
	filename = function (win)
		local file = win.file
		return (file.name or file.path and file.path:match'[^/]+$' or '[No Name]')
			.. (file.modified and ' [+]' or '') .. (vis.recording and ' @' or '')
	end,

	mode = function (win)
		local mode = modes[vis.mode]
		if mode ~= '' and vis.win == win then
			return mode
		end
	end,

	countorkeys = function (_)
		local count = vis.count
		local keys = vis.input_queue
		if keys ~= '' then
			return keys
		elseif count then
			return count
		end
	end,

	selections = function (win)
		if #win.selections > 1 then
			return win.selection.number .. '/' .. #win.selections
		end
	end,

	position = function (win)
		local pos = win.selection.pos
		if not pos then pos = 0 end
		local size = win.file.size
		return (size == 0 and "0" or math.ceil(pos/size*100)) .. "%"
	end,

	location = function (win)
		return win.selection.line .. ', ' .. win.selection.col
	end,
}

local left = {actions.mode, actions.filename}
local right = {actions.countorkeys, actions.selections, actions.position, actions.location}
local statusbar = {
	actions = actions,
	left = left,
	right = right
}

vis.statusbar = statusbar

local concat = table.concat

vis.events.subscribe(vis.events.WIN_STATUS, function(win)
	local left_parts = {}
	local right_parts = {}

	if not win:large() then
		local col = win.selection.col
		table.insert(right_parts, win.selection.line..', '..col)
	end

	local i = 1
	for _, f in ipairs(vis.statusbar.left) do
		local str = f(win)
		if str then i, left_parts[i] = i+1, str end
	end
	i = 1 for _, f in ipairs(vis.statusbar.right) do
		local str = f(win)
		if str then i, right_parts[i] = i+1, str end
	end

	local left_concat = ' ' .. concat(left_parts, " » ") .. ' '
	local right_concat = ' ' .. concat(right_parts, " « ") .. ' '

	win:status(left_concat, right_concat);
end)

return statusbar
