require("plugins/number-inc-dec")
local win = vis.win

describe("between numbers", function()
	it("between numbers", function()
		win.selection.pos = 4
		vis:feedkeys("<C-a>")
		assert.are.equal("1.2rc4", win.file.lines[1])
	end)
end)

describe("in number", function()
	it("in number", function()
		win.selection.pos = 11
		vis:feedkeys("<C-a>")
		assert.are.equal("1xx20xx", win.file.lines[2])
	end)
end)

describe("end of word/file", function()
	it("end of word/file", function()
		win.selection.pos = 24
		vis:feedkeys("<C-a>")
		assert.are.equal("1foo22bar4", win.file.lines[3])
	end)
end)
