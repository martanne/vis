-- table_cmp - return true if table1 is the same as table2
table_cmp = function(table1, table2)
	if type(table1) ~= 'table' or type(table2) ~= 'table' then
		return false
	end
	if #table1 ~= #table2 then
		return false
	end
	for i, item in pairs(table1) do
		if item ~= table2[i] then
			return false
		end
	end
	return true
end

-- strcasecmp - compare strings case-insensitively
strcasecmp = function(str1, str2)
	if type(str1) ~= 'string' or type(str2) ~= 'string' then
		return false
	end
	str1 = string.lower(str1)
	str2 = string.lower(str2)
	return str1 == str2
end

-- Setline - Set lines starting at [startline] to [lines]
--         - [startline] can be '.' to set the current line
--         - [startline] can be '$' to set the last line in the buffer
--         - [lines] can be left nil in which case, set lines to empty
setline = function(win, startline, lines)
	if startline == '.' then
		startline = win.cursor.line
	elseif startline == '$' then
		startline = #win.file.lines
	end

	if type(lines) == 'string' then
		lines = {lines}
	end

	for i, line in pairs(lines or {}) do
		i = i + ((startline - 1) or 0)
		win.file.lines[i] = line
	end
end

-- Delete - Remove [number] lines starting at [startline]
--        - [number] can be left nil to remove single line
--        - [startline] can be '.' to delete the current line
--        - [startline] can be '$' to delete the last line in the buffer
--        - [startline] can be '%' to empty the buffer
delete = function(win, startline, number)
	if startline == '.' then
		startline = win.cursor.line
	elseif startline == '$' then
		startline = #win.file.lines
	elseif startline == '%' then
		startline = 1
		number = #win.file.lines
	end

	-- Get position in file of startline
	win.cursor:to(startline, 0)
	startpos = win.cursor.pos

	-- Get position in file of endline, or delete single line
	number = number or 1
	win.cursor:to(startline + number, 0)
	endpos = win.cursor.pos

	-- Delete lines
	len = endpos - startpos
	win.file:delete(startpos, len)
end

-- Getline - Return a table of [number] lines starting from [startline]
--         - [startline] can be '.' to return the current line
--         - [startline] can be '$' to return the last line in the buffer
--         - [startline] can be '%' to return the whole buffer
--         - [number] can be left nil, in which case return a string
--         containing line [startline]
getline = function(win, startline, number)
	if startline == '.' then
		startline = win.cursor.line
	elseif startline == '$' or startline > #win.file.lines then
		startline = #win.file.lines
	elseif startline == '%' then
		startline = 1
		number = #win.file.lines
	elseif startline <= 0 then
		startline = 1
	end

	if number == nil then
		return win.file.lines[startline]
	else
		lines = {}
		for i = startline, startline + number - 1, 1 do
			table.insert(lines, win.file.lines[i])
		end
		return lines
	end
end

-- Append - Insert lines after given line number
--        - [line] can be either string or table
--        - [startline] can be '.' to append after the current line
--        - [startline] can be '$' to append after the last line in the buffer
append = function(win, startline, lines)
	if startline == '.' then
		win.cursor:to(win.cursor.line + 1,  0)
	elseif startline == '$' or startline > #win.file.lines then
		win.cursor:to(#win.file.lines + 1, 0)
	else
		win.cursor:to(startline + 1, 0)
	end
	pos = win.cursor.pos

	if type(lines) == 'table' then
		lines = table.concat(lines, "\n") .. "\n"
	elseif type(lines) == 'string' then
		lines = lines .. '\n'
	end

	win.file:insert(pos, lines)
end
