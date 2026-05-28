describe("File offset / line column conversion functions", function()
	local file = vis.win.file

	describe("file:line_column_from_offset()", function()
		it("offset -1: negative offset", function()
			assert.has_error(function() file:line_column_from_offset(-1) end)
		end)

		it("offset 0: start of file", function()
			local l, c = file:line_column_from_offset(0)
			assert.are.equal(1, l)
			assert.are.equal(1, c)
		end)

		it("offset 5: in first line", function()
			local l, c = file:line_column_from_offset(5)
			assert.are.equal(1, l)
			assert.are.equal(6, c)
		end)

		it("offset 11: start of second line", function()
			local l, c = file:line_column_from_offset(11)
			assert.are.equal(2, l)
			assert.are.equal(1, c)
		end)

		it("offset 30: empty line", function()
			local l, c = file:line_column_from_offset(30)
			assert.are.equal(3, l)
			assert.are.equal(1, c)
		end)

		it("offset 34: behind 2 byte code point í", function()
			local l, c = file:line_column_from_offset(34)
			assert.are.equal(4, l)
			assert.are.equal(3, c)
		end)

		it("offset 68: last char", function()
			local l, c = file:line_column_from_offset(68)
			assert.are.equal(5, l)
			assert.are.equal(10, c)
		end)

		it("offset 69: end of file", function()
			local l, c = file:line_column_from_offset(69)
			assert.are.equal(6, l)
			assert.are.equal(1, c)
		end)

		it("offset 1000: too big offset", function()
			local l, c = file:line_column_from_offset(1000)
			assert.are.equal(6, l)
			assert.are.equal(1, c)
		end)
	end)

	describe("file:offset_from_line_column()", function()
		it("no column: invalid line", function()
			assert.are.equal(nil, file:offset_from_line_column(100))
		end)

		it("no column: first line", function()
			assert.are.equal(0, file:offset_from_line_column(1))
		end)

		it("no column: second line", function()
			assert.are.equal(11, file:offset_from_line_column(2))
		end)

		it("no line", function()
			assert.has_error(function() file:offset_from_line_column(nil, 1) end)
		end)

		it("(-1,1): negative line", function()
			assert.has_error(function() file:offset_from_line_column(-1, 1) end)
		end)

		it("(1,-1): negative column", function()
			assert.has_error(function() file:offset_from_line_column(1, -1) end)
		end)

		it("(0,1): zero line", function()
			assert.are.equal(0, file:offset_from_line_column(0, 1))
		end)

		it("(1,0): zero column", function()
			assert.are.equal(0, file:offset_from_line_column(1, 0))
		end)

		it("(1,1): start of file", function()
			assert.are.equal(0, file:offset_from_line_column(1, 1))
		end)

		it("(1, 6): in first line", function()
			assert.are.equal(5, file:offset_from_line_column(1, 6))
		end)

		it("(1, 100): invalid column -> end of line", function()
			assert.are.equal(10, file:offset_from_line_column(1, 100))
		end)

		it("(2,1): start of second line", function()
			assert.are.equal(11, file:offset_from_line_column(2, 1))
		end)

		it("(4,12): behind 2 byte code point Ü", function()
			assert.are.equal(44, file:offset_from_line_column(4, 12))
		end)
	end)

	describe("Symmetry: pos to line, column to pos", function()
		it("start of file", function()
			local test_pos = 0
			local l, c = file:line_column_from_offset(test_pos)
			local pos_result = file:offset_from_line_column(l, c)
			assert.are.equal(test_pos, pos_result)
		end)

		it("first line", function()
			local test_pos = 6
			local l, c = file:line_column_from_offset(test_pos)
			local pos_result = file:offset_from_line_column(l, c)
			assert.are.equal(test_pos, pos_result)
		end)

		it("behind 2 byte code point", function()
			local test_pos = 34
			local l, c = file:line_column_from_offset(test_pos)
			local pos_result = file:offset_from_line_column(l, c)
			assert.are.equal(test_pos, pos_result)
		end)
	end)

	describe("Symmetry: line, column to pos to line, column", function()
		it("second line", function()
			local test_line, test_col = 2, 3
			local pos = file:offset_from_line_column(test_line, test_col)
			local l_result, c_result = file:line_column_from_offset(pos)
			assert.are.equal(test_line, l_result)
			assert.are.equal(test_col, c_result)
		end)

		it("behind multiple 2 byte code point", function()
			local test_line, test_col = 4, 12  -- 'n' in "Ünicode"
			local pos = file:offset_from_line_column(test_line, test_col)
			local l_result, c_result = file:line_column_from_offset(pos)
			assert.are.equal(test_line, l_result)
			assert.are.equal(test_col, c_result)
		end)
	end)
end)
