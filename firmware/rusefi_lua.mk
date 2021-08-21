LUADIR = $(PROJECT_DIR)/controllers/lua/builtin
LUAGEN = $(LUADIR)/generated
SRCPATHS += $(LUADIR)

LUASRC = $(LUADIR)/fan.lua $(LUADIR)/builtin.lua $(LUADIR)/pump.lua

LUAOBJS = $(addprefix $(LUAGEN)/, $(notdir $(LUASRC:.lua=.h)))

SHELL := bash

$(LUAOBJS) : $(LUAGEN)/%.h : %.lua $(MAKEFILE_LIST)
	@echo Converting $(<F) -\> $(@F)
	@xxd -i $< | cat <(echo -n "static const ") - > $@

#$(LUAOBJS) : $(OBJDIR)/%.o : %.lua $(MAKEFILE_LIST)
#	$(TRGT)ld -r -b binary -o $@ $<

# Lua builtin file depends on lua headers being generated
$(OBJDIR)/lua_builtin.o : $(LUAOBJS)
