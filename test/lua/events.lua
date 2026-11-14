local events = vis.events
local modes = vis.modes
local win = vis.win

describe("Insert mode events", function()

	local enter_count = 0
	local leave_count = 0

	events.subscribe(events.INSERT_ENTER, function()
		enter_count = enter_count + 1
	end)

	events.subscribe(events.INSERT_LEAVE, function()
		leave_count = leave_count + 1
	end)

	before_each(function()
		enter_count = 0
		leave_count = 0
		-- reset file content
		if win.file.size > 0 then
			win.file:delete(0, win.file.size)
		end
		win.selection.pos = 0
	end)

	it("i from normal mode", function()
		vis:feedkeys("i")
		assert.are.equal(1, enter_count)
		assert.are.equal(0, leave_count)
		vis:feedkeys("<Escape>")
		assert.are.equal(1, enter_count)
		assert.are.equal(1, leave_count)
	end)

	it("a from normal mode", function()
		vis:feedkeys("a")
		assert.are.equal(1, enter_count)
		assert.are.equal(0, leave_count)
		vis:feedkeys("<Escape>")
		assert.are.equal(1, enter_count)
		assert.are.equal(1, leave_count)
	end)

	it("I from normal mode", function()
		vis:feedkeys("I")
		assert.are.equal(1, enter_count)
		assert.are.equal(0, leave_count)
		vis:feedkeys("<Escape>")
		assert.are.equal(1, enter_count)
		assert.are.equal(1, leave_count)
	end)

	it("A from normal mode", function()
		vis:feedkeys("A")
		assert.are.equal(1, enter_count)
		assert.are.equal(0, leave_count)
		vis:feedkeys("<Escape>")
		assert.are.equal(1, enter_count)
		assert.are.equal(1, leave_count)
	end)

	it("o from normal mode", function()
		vis:feedkeys("o")
		assert.are.equal(1, enter_count)
		assert.are.equal(0, leave_count)
		vis:feedkeys("<Escape>")
		assert.are.equal(1, enter_count)
		assert.are.equal(1, leave_count)
	end)

	it("O from normal mode", function()
		vis:feedkeys("O")
		assert.are.equal(1, enter_count)
		assert.are.equal(0, leave_count)
		vis:feedkeys("<Escape>")
		assert.are.equal(1, enter_count)
		assert.are.equal(1, leave_count)
	end)

	it("c{motion} from normal mode", function()
		win.file:insert(0, "some text")
		win.selection.pos = 0
		vis:feedkeys("cw")
		assert.are.equal(1, enter_count)
		assert.are.equal(0, leave_count)
		vis:feedkeys("<Escape>")
		assert.are.equal(1, enter_count)
		assert.are.equal(1, leave_count)
	end)

	it("s from normal mode", function()
		win.file:insert(0, "some text")
		win.selection.pos = 0
		vis:feedkeys("s")
		assert.are.equal(1, enter_count)
		assert.are.equal(0, leave_count)
		vis:feedkeys("<Escape>")
		assert.are.equal(1, enter_count)
		assert.are.equal(1, leave_count)
	end)

	it("S from normal mode", function()
		win.file:insert(0, "some text")
		win.selection.pos = 0
		vis:feedkeys("S")
		assert.are.equal(1, enter_count)
		assert.are.equal(0, leave_count)
		vis:feedkeys("<Escape>")
		assert.are.equal(1, enter_count)
		assert.are.equal(1, leave_count)
	end)

	it("multiple enter/leave", function()
		vis:feedkeys("i<Escape>a<Escape>o<Escape>")
		assert.are.equal(3, enter_count)
		assert.are.equal(3, leave_count)
	end)

	it("no event when changing to other modes", function()
		vis:feedkeys("v")
		assert.are.equal(0, enter_count)
		assert.are.equal(0, leave_count)
		vis:feedkeys("<Escape>")
		assert.are.equal(0, enter_count)
		assert.are.equal(0, leave_count)
	end)

end)
