# Library description
DESCRIPTION.cssndldr = Crystal Space sound loader library

#-------------------------------------------------------------- rootdefines ---#
ifeq ($(MAKESECTION),rootdefines)

# Library-specific help commands
LIBHELP += $(NEWLINE)echo $"  make cssndldr     Make the $(DESCRIPTION.cssndldr)$"

ifeq ($(DO_AIFF),yes)
  MAKE_VOLATILE_H += $(NEWLINE)echo $"\#define DO_AIFF$">>$@
endif
ifeq ($(DO_IFF),yes)
  MAKE_VOLATILE_H += $(NEWLINE)echo $"\#define DO_IFF$">>$@
endif
ifeq ($(DO_WAV),yes)
  MAKE_VOLATILE_H += $(NEWLINE)echo $"\#define DO_WAV$">>$@
endif
ifeq ($(DO_AU),yes)
  MAKE_VOLATILE_H += $(NEWLINE)echo $"\#define DO_AU$">>$@
endif

endif # ifeq ($(MAKESECTION),rootdefines)

#-------------------------------------------------------------- roottargets ---#
ifeq ($(MAKESECTION),roottargets)

.PHONY: cssndldr

all libs: cssndldr
cssndldr:
	$(MAKE_TARGET)

endif # ifeq ($(MAKESECTION),roottargets)

#-------------------------------------------------------------- postdefines ---#
ifeq ($(MAKESECTION),postdefines)

vpath %.cpp libs/cssndldr libs/cssndldr/common

SRC.CSSNDLDR = $(wildcard libs/cssndldr/common/*.cpp) \
  libs/cssndldr/sndload.cpp libs/cssndldr/funcs.cpp

ifeq ($(DO_AIFF),yes)
  SRC.CSSNDLDR+=libs/cssndldr/aifffile.cpp
endif
ifeq ($(DO_IFF),yes)
  SRC.CSSNDLDR+=libs/cssndldr/ifffile.cpp
endif
ifeq ($(DO_WAV),yes)
  SRC.CSSNDLDR+=libs/cssndldr/wavfile.cpp
endif
ifeq ($(DO_AU),yes)
  SRC.CSSNDLDR+=libs/cssndldr/aufile.cpp
endif

CSSNDLDR.LIB = $(OUT)$(LIB_PREFIX)cssndldr$(LIB)
OBJ.CSSNDLDR = $(addprefix $(OUT),$(notdir $(SRC.CSSNDLDR:.cpp=$O)))

endif # ifeq ($(MAKESECTION),postdefines)

#------------------------------------------------------------------ targets ---#
ifeq ($(MAKESECTION),targets)

.PHONY: cssndldr cssndldrclean

all: $(CSSNDLDR.LIB)
cssndldr: $(OUTDIRS) $(CSSNDLDR.LIB)
clean: cssndldrclean

$(CSSNDLDR.LIB): $(OBJ.CSSNDLDR)
	$(DO.STATIC.LIBRARY)

cssndldrclean:
	-$(RM) $(CSSNDLDR.LIB)

ifdef DO_DEPEND
$(OUTOS)cssndldr.dep: $(SRC.CSSNDLDR)
	$(DO.DEP)
endif

-include $(OUTOS)cssndldr.dep

endif # ifeq ($(MAKESECTION),targets)
