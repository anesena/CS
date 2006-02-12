///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for random generators.
 *	\file		IceRandom.cpp
 *	\author		Pierre Terdiman
 *	\date		August, 9, 2001
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "Stdafx.h"

using namespace CS::Plugins::Opcode;
using namespace CS::Plugins::Opcode::IceCore;

void IceCore::	SRand(udword seed)
{
	srand(seed);
}

udword IceCore::Rand()
{
	return rand();
}


static BasicRandom gRandomGenerator(42);

udword IceCore::GetRandomIndex(udword max_index)
{
	// We don't use rand() since it's limited to RAND_MAX
	udword Index = gRandomGenerator.Randomize();
	return Index % max_index;
}

