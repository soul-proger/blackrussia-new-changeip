/////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////////


#include "vendor/EASTL/atomic.h"


namespace eastl
{

namespace internal
{


static void EastlCompilerBarrierDataDependencyFunc(void*)
{
}

volatile CompilerBarrierDataDependencyFuncPtr gCompilerBarrierDataDependencyFunc = &EastlCompilerBarrierDataDependencyFunc;


} // namespace internal

} // namespace eastl
