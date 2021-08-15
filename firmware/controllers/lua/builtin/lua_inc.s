	.section .rodata
	.global LuaData
	.type   LuaData, %object
	.align  4
LuaData:
	.incbin "controllers/lua/builtin/fan.lua"
	.global LuaData_size
	.type   LuaData_size, %object
	.align  4
LuaData_size:
	.int    LuaData_size - LuaData
