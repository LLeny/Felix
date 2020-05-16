#include "CPUExecute.hpp"
#include "BusMaster.hpp"

AwaitCPURead CpuExecute::promise_type::await_transform( CPURead r )
{
  return AwaitCPURead{ mBus->request( r ) };
}

AwaitCPUFetchOpcode CpuExecute::promise_type::await_transform( CPUFetchOpcode r )
{
  return AwaitCPUFetchOpcode{ mBus->request( r ) };
}

AwaitCPUFetchOperand CpuExecute::promise_type::await_transform( CPUFetchOperand r )
{
  return AwaitCPUFetchOperand{ mBus->request( r ) };
}

AwaitCPUWrite CpuExecute::promise_type::await_transform( CPUWrite w )
{
  return AwaitCPUWrite{ mBus->request( w ) };
}


