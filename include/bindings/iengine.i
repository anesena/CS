%module iengine
%import "bindings/cspace.i"
%import "bindings/ivideo.i" /* due to at least csZBufMode graph3d.h */
%{
#include "crystalspace.h"
%}
LANG_FUNCTIONS

%include "bindings/common/iengine.i"

