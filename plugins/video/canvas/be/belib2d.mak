# This is a subinclude file used to define the rules needed
# to build the BeOS 2D driver -- be2d

# Driver description
DESCRIPTION.be2d = Crystal Space BeLib 2D driver

#------------------------------------------------------------- rootdefines ---#
ifeq ($(MAKESECTION),rootdefines)

# Driver-specific help commands
DRIVERHELP += \
  $(NEWLINE)echo $"  make be2d         Make the $(DESCRIPTION.be2d)$"

endif # ifeq ($(MAKESECTION),rootdefines)

#------------------------------------------------------------- roottargets ---#
ifeq ($(MAKESECTION),roottargets)

.PHONY: be2d be2dclean

all plugins drivers drivers2d: be2d

be2d:
	$(MAKE_TARGET) MAKE_DLL=yes
be2dclean:
	$(MAKE_CLEAN)

endif # ifeq ($(MAKESECTION),roottargets)

#------------------------------------------------------------- postdefines ---#
ifeq ($(MAKESECTION),postdefines)

vpath %.cpp plugins/video/canvas/be

# The 2D Belib driver
ifeq ($(USE_SHARED_PLUGINS),yes)
  BE2D=$(OUTDLL)be2d$(DLL)
  DEP.BE2D = $(CSUTIL.LIB) $(CSSYS.LIB)
  TO_INSTALL.DYNAMIC_LIBS += $(BE2D)
else
  BE2D=be2d.a
  DEP.EXE+=$(BE2D)
  CFLAGS.STATIC_SCF+=$(CFLAGS.D)SCL_BE2D
  TO_INSTALL.STATIC_LIBS += $(BE2D)
endif
DESCRIPTION.$(BE2D) = $(DESCRIPTION.be2d)
SRC.BE2D = $(wildcard plugins/video/canvas/be/*.cpp $(SRC.COMMON.DRV2D))
OBJ.BE2D = $(addprefix $(OUT),$(notdir $(SRC.BE2D:.cpp=$O)))

endif # ifeq ($(MAKESECTION),postdefines)

#----------------------------------------------------------------- targets ---#
ifeq ($(MAKESECTION),targets)

vpath %.cpp plugins/video/canvas/be

.PHONY: be2d be2dclean

# Chain rules
clean: be2dclean

be2d: $(OUTDIRS) $(BE2D)

$(BE2D): $(OBJ.BE2D) $(DEP.BE2D)
	$(DO.PLUGIN)

be2dclean:
	$(RM) $(BE2D) $(OBJ.BE2D) $(OUTOS)be2d.dep

ifdef DO_DEPEND
dep: $(OUTOS)be2d.dep
$(OUTOS)be2d.dep: $(SRC.BE2D)
	$(DO.DEP)
else
-include $(OUT)be2d.dep
endif

endif # ifeq ($(MAKESECTION),targets)
