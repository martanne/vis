local local_options = {}
local option_set_handler = function(value, toggle, name)
	if name == "test1" then
		local_options[name] = toggle and not local_options[name] or value
	else
		local_options[name] = value
	end
end
local option_get_handler = function(name)
	return local_options[name]
end

vis:option_register("test1", "bool",   option_set_handler, option_get_handler)
vis:option_register("test2", "string", option_set_handler, option_get_handler)
vis:option_register("test3", "number", option_set_handler, option_get_handler)
vis:option_register("test4", "number", option_set_handler)

describe("option_register", function()
	it("set/get: bool", function()
		local_options["test1"] = false
		vis:command(":set test1 true")
		assert.are.same(local_options["test1"], true)
		assert.are.same(local_options["test1"], vis.options.test1)
	end)

	it("set/get: string", function()
		local_options["test2"] = "foo"
		vis:command(":set test2 bar")
		assert.are.same(local_options["test2"], "bar")
		assert.are.same(local_options["test2"], vis.options.test2)
	end)

	it("set/get: number", function()
		local_options["test3"] = 1
		vis:command(":set test3 2")
		assert.are.same(local_options["test3"], 2)
		assert.are.same(local_options["test3"], vis.options.test3)
	end)

	it("set/get: no get function", function()
		local_options["test4"] = 1
		vis:command(":set test4 2")
		assert.are.same(local_options["test4"], 2)
		assert.are.same(vis.options.test4, nil)
	end)

	it("set: bool toggle", function()
		local_options["test1"] = true
		vis:command(":set test1!")
		assert.are.same(local_options["test1"], false)
	end)

	it("set: through vis.options", function()
		local_options["test2"] = "foo"
		vis.options.test2 = "bar"
		assert.are.same(local_options["test2"], "bar")
	end)
end)
