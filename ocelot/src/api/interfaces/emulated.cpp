#include <sstream>

#include <ocelot/ir/Module.h>
#include <ocelot/executive/EmulatedKernel.h>

extern "C" void ptx_run(const char* source, int n_args, void* args[],
    int blck_x, int blck_y, int blck_z,
    int grid_x, int grid_y, int grid_z, int shared_mem_size = 0)
{
	std::stringstream ss;
    ss << source;
	ir::Module mod((void*)source, ss);
	ir::PTXKernel* rawk = mod.kernels().begin()->second;
	executive::EmulatedKernel emuk(rawk, NULL);
    
	const ir::PTXKernel::Prototype proto = rawk->getPrototype();
	for (int i=0; i<n_args; i++) {
		ir::Parameter* param = emuk.getParameter(proto.arguments[i].name);
		param->arrayValues.resize(1);
		param->arrayValues[0].val_u64 = (ir::PTXU64)args[i];
	}

	emuk.updateArgumentMemory();
	if (shared_mem_size > 0)
	{
		emuk.setExternSharedMemorySize(shared_mem_size);
	}
	emuk.setKernelShape(blck_x, blck_y, blck_z);
	emuk.launchGrid(grid_x, grid_y, grid_z);
}