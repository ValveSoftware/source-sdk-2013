--		INCOMPLETE CODE SAMPLES
----------------------------------------
-- Object = class {
--	
-- }
----------------------------------------
-- local obj = Object()
----------------------------------------
-- -- TODO: Use the '&' operator for inheritence?
-- Subclass = Object & class {
--
-- }
----------------------------------------

-- TODO: 
--
--		- This code needs lots of
--		testing and features, like
--		inheritence, statics, etc.
--
--		- Override a rarely used operator,
--		like __band (&) to use in inheritence
--
--		

function class (body)

	-- class object to be returned
	-- should only contain statics
	-- everything else is in metatables
	local cl = {}
	
	-- cl's metaTable
	-- contains actual class definition
	-- 		__class - class members
	--		__new	- constructor
	local metaTable = {}
	
	-- move constructor to metaTable
	if body.new then
		metaTable.__new = body.new
		body.new = nil
	end
	
	-- class members
	metaTable.__class = body
	
	-- internal constructor (__call)
	metaTable.__call = function(self, ...)
		local instance = {}
		local selfmt = getmetatable(self)
		
		-- add members to __index
		setmetatable(instance, {__index = selfmt.__class})
		
		
		-- call constructor
		if selfmt.__new then selfmt.__new(instance, unpack(arg)) end
		
		return instance
	end
	
	setmetatable(cl, metaTable)
	return cl
end