LUADIR = $(PROJECT_DIR)/controllers/lua/builtin
SRCPATHS += $(LUADIR)

LUASRC = $(LUADIR)/fan.lua $(LUADIR)/builtin.lua

LUAOBJS = $(addprefix $(OBJDIR)/, $(notdir $(LUASRC:.lua=.o)))

# Add it to the object list so it gets linked
OBJS += $(LUAOBJS)

$(LUAOBJS) : $(OBJDIR)/%.o : %.lua $(MAKEFILE_LIST)
	$(TRGT)ld -r -b binary -o $@ $<

# Elf binary depends on lua objects
$(BUILDDIR)/$(PROJECT).elf: $(LUAOBJS)
