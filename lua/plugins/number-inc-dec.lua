-- increment/decrement number in dec/hex/oct format
local lexer = vis.lexers
if not lexer then return end

local change = function(delta)

	local win = vis.win
	local file = win.file
	local count = vis.count
	if not count then count = 1 end
	vis.count = nil -- reset count

	for cursor in win:cursors_iterator() do
		local pos = cursor.pos
		local word = file:text_object_word(pos);
		local s, e = word.start, word.finish;
		if s then
			local data = file:content(s, e - s)
			if data and #data > 0 then
				local base, format, padding = 10, 'd', 0
				if lexer.oct_num:match(data) then
					base = 8
					format = 'o'
					padding = #data
				elseif lexer.hex_num:match(data) then
					base = 16
					format = 'x'
					padding = #data - #"0x"
				end
				local number = tonumber(data, base == 8 and 8 or nil)
				if number then
					number = number + delta * count
					if base ~= 10 and number < 0 then
						-- string.format does not handle negative hex/oct values
						-- should we wrap around?
						number = 0
					end
					number = string.format((base == 16 and "0x" or "") .. "%0"..padding..format, number)
					if base == 8 and string.sub(number, 0, 1) ~= "0" then
						number = '0' .. number
					end
					file:delete(s, e - s)
					file:insert(s, number)
					cursor.pos = s
				else
					vis:info("Not a number")
				end
			end
		end
	end
end

vis:map(vis.modes.NORMAL, "<C-a>", function() change( 1) end, "Increment number")
vis:map(vis.modes.NORMAL, "<C-x>", function() change(-1) end, "Decrement number")
