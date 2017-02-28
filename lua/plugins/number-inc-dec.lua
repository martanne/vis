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

		local word_range = file:text_object_word(pos);
		local word_start = word_range.start
		local word = file:content(word_range.start,word_range.finish)
		local Cp = lpeg.Cp()
		-- create Pattern for hex,oct,int
		local int_pattern = lpeg.S('+-')^-1 * lexer.dec_num
		local pattern = lpeg.P{ Cp * (lexer.hex_num + lexer.oct_num + int_pattern ) * Cp +1 * lpeg.V(1) }
		local s, e = pattern:match(word)

		if s then
			local data = string.sub(word,s,e-1)
			-- align start and end for fileindex
			s = word_start+s-1
			e = word_start+e
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
						-- http://www.lua.org/manual/5.1/manual.html#pdf-tonumber
						-- tonumber does not handle negative hex/oct values
						-- should we wrap around?
						number = 0
					end
					number = string.format((base == 16 and "0x" or "") .. "%0"..padding..format, number)
					if base == 8 and string.sub(number, 0, 1) ~= "0" then
						number = '0' .. number
					end
					-- need to increment s for length
					file:delete(s, e-(s+1))
					file:insert(s, number)
					cursor.pos = s
				else
	--				vis:info("Not a number")
				end
			end
		end
	end
end

vis:map(vis.modes.NORMAL, "<C-a>", function() change( 1) end, "Increment number")
vis:map(vis.modes.NORMAL, "<C-x>", function() change(-1) end, "Decrement number")
