require 'busted.runner'()

local file = vis.win.file

describe("vis.pipe", function()

	local FULLSCREEN = true

	it("vis.pipe no input", function()
		vis:pipe("cat > f")
		local f = io.open("f", "r")
		assert.truthy(f)
		assert.are.equal("", f:read("*a"))
		f:close()
		os.remove("f")
	end)

	it("vis.pipe no input fullscreen", function()
		vis:pipe("cat > f", FULLSCREEN)
		local f = io.open("f", "r")
		assert.truthy(f)
		assert.are.equal("", f:read("*a"))
		f:close()
		os.remove("f")
	end)

	it("vis.pipe buffer", function()
		vis:pipe("foo", "cat > f")
		local f = io.open("f", "r")
		assert.truthy(f)
		assert.are.equal(f:read("*a"), "foo")
		f:close()
		os.remove("f")
	end)

	it("vis.pipe buffer fullscreen", function()
		vis:pipe("foo", "cat > f", FULLSCREEN)
		local f = io.open("f", "r")
		assert.truthy(f)
		assert.are.equal(f:read("*a"), "foo")
		f:close()
		os.remove("f")
	end)

	it("vis.pipe range", function()
		vis:pipe(file, {start=0, finish=3}, "cat > f")
		local f = io.open("f", "r")
		assert.truthy(f)
		assert.are.equal(f:read("*a"), "foo")
		f:close()
		os.remove("f")
	end)

	it("vis.pipe range fullscreen", function()
		vis:pipe(file, {start=0, finish=3}, "cat > f", FULLSCREEN)
		local f = io.open("f", "r")
		assert.truthy(f)
		assert.are.equal(f:read("*a"), "foo")
		f:close()
		os.remove("f")
	end)

	it("vis.pipe explicit nil text", function()
		assert.has_error(function() vis:pipe(nil, "true") end)
	end)

	it("vis.pipe explicit nil text fullscreen", function()
		assert.has_error(function() vis:pipe(nil, "true", FULLSCREEN) end)
	end)

	it("vis.pipe explicit nil file", function()
		assert.has_error(function() vis:pipe(nil, {start=0, finish=0}, "true") end)
	end)

	it("vis.pipe explicit nil file fullscreen", function()
		assert.has_error(function() vis:pipe(nil, {start=0, finish=0}, "true", FULLSCREEN) end)
	end)

	it("vis.pipe wrong argument order file, range, cmd", function()
		assert.has_error(function() vis:pipe({start=0, finish=0}, vis.win.file, "true") end)
	end)

	it("vis.pipe wrong argument order fullscreen, cmd", function()
		assert.has_error(function() vis:pipe(FULLSCREEN, "true") end)
	end)
end)
