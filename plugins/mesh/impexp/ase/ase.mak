DESCRIPTION.aseie = ASE Import/Export plug-in

#------------------------------------------------------------- rootdefines ---#
ifeq ($(MAKESECTION),rootdefines)

PLUGINHELP += \
  $(NEWLINE)echo $"  make aseie        Make the $(DESCRIPTION.aseie)$"

endif # ifeq ($(MAKESECTION),rootdefines)
#------------------------------------------------------------- roottargets ---#
ifeq ($(MAKESECTION),roottargets)

.PHONY: aseie aseieclean
plugins meshes all: aseie

aseieclean:
	$(MAKE_CLEAN)
aseie:
	$(MAKE_TARGET) MAKE_DLL=yes

endif # ifeq ($(MAKESECTION),roottargets)
#------------------------------------------------------------- postdefines ---#
ifeq ($(MAKESECTION),postdefines)

vpath %.cpp $(SRCDIR)/plugins/mesh/impexp/ase

ifeq ($(USE_PLUGINS),yes)
  ASEIE = $(OUTDLL)/aseie$(DLL)
  LIB.ASEIE = $(foreach d,$(DEP.ASEIE),$($d.LIB))
  TO_INSTALL.DYNAMIC_LIBS += $(ASEIE)
else
  ASEIE = $(OUT)/$(LIB_PREFIX)aseie$(LIB)
  DEP.EXE += $(ASEIE)
  SCF.STATIC += aseie
  TO_INSTALL.STATIC_LIBS += $(ASEIE)
endif

INF.ASEIE = $(SRCDIR)/plugins/mesh/impexp/ase/aseie.csplugin
INC.ASEIE = $(wildcard $(addprefix $(SRCDIR)/,plugins/mesh/impexp/ase/*.h))
SRC.ASEIE = $(wildcard $(addprefix $(SRCDIR)/,plugins/mesh/impexp/ase/*.cpp))
OBJ.ASEIE = $(addprefix $(OUT)/,$(notdir $(SRC.ASEIE:.cpp=$O)))
DEP.ASEIE = CSGEOM CSUTIL CSTOOL CSGEOM CSUTIL

MSVC.DSP += ASEIE
DSP.ASEIE.NAME = aseie
DSP.ASEIE.TYPE = plugin

endif # ifeq ($(MAKESECTION),postdefines)
#----------------------------------------------------------------- targets ---#
ifeq ($(MAKESECTION),targets)

.PHONY: aseie aseieclean
aseie: $(OUTDIRS) $(ASEIE)

$(ASEIE): $(OBJ.ASEIE) $(LIB.ASEIE)
	$(DO.PLUGIN)

clean: aseieclean
aseieclean:
	-$(RMDIR) $(ASEIE) $(OBJ.ASEIE) $(OUTDLL)/$(notdir $(INF.ASEIE))

ifdef DO_DEPEND
dep: $(OUTOS)/aseie.dep
$(OUTOS)/aseie.dep: $(SRC.ASEIE)
	$(DO.DEP)
else
-include $(OUTOS)/aseie.dep
endif

endif # ifeq ($(MAKESECTION),targets)
