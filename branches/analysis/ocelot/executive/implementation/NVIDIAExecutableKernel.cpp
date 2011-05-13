/*! \file NVIDIAExecutableKernel.cpp
	\author Andrew Kerr <arkerr@gatech.edu>
	\date October 6, 2009
	\brief implements the GPU kernel callable by the executive
*/

#include <ocelot/executive/interface/NVIDIAExecutableKernel.h>

#include <hydrazine/implementation/debug.h>
#include <hydrazine/implementation/Exception.h>
#include <hydrazine/implementation/macros.h>


#ifdef REPORT_BASE
#undef REPORT_BASE
#endif

#define REPORT_BASE 0

#define Ocelot_Exception(x) { std::stringstream ss; ss << x; report(ss.str()); \
	throw hydrazine::Exception(ss.str()); }

/////////////////////////////////////////////////////////////////////////////////////////////////

executive::NVIDIAExecutableKernel::NVIDIAExecutableKernel() {
	this->ISA = ir::Instruction::SASS;
}

executive::NVIDIAExecutableKernel::~NVIDIAExecutableKernel() {

}

/*!
	Construct a NVIDIAExecutableKernel from an existing kernel
*/
executive::NVIDIAExecutableKernel::NVIDIAExecutableKernel(
	ir::IRKernel& kernel, const CUfunction& function, 
	executive::Device* d ): ExecutableKernel(kernel, d), 
	cuFunction(function) {
	
	report("NVIDIAExecutableKernel()");
	this->ISA = ir::Instruction::SASS;
	
	_argumentMemorySize = mapArgumentOffsets();
		
	cuda::CudaDriver::cuFuncGetAttribute((int*)&_registerCount, 
		CU_FUNC_ATTRIBUTE_NUM_REGS, cuFunction);
	report(" Registers - " << _registerCount);
	cuda::CudaDriver::cuFuncGetAttribute((int*)&_constMemorySize, 
		CU_FUNC_ATTRIBUTE_CONST_SIZE_BYTES, cuFunction);
	report(" Constant Memory - " << _constMemorySize);
	cuda::CudaDriver::cuFuncGetAttribute((int*)&_localMemorySize, 
		CU_FUNC_ATTRIBUTE_LOCAL_SIZE_BYTES, cuFunction);
	report(" Local Memory - " << _localMemorySize);
	cuda::CudaDriver::cuFuncGetAttribute((int*)&_sharedMemorySize, 
		CU_FUNC_ATTRIBUTE_SHARED_SIZE_BYTES, cuFunction);
	report(" Shared Memory - " << _sharedMemorySize);

	report("  constructed new NVIDIAExecutableKernel");
}

/*!
	Launch a kernel on a 2D grid
*/
void executive::NVIDIAExecutableKernel::launchGrid(int width, int height) {
	report("executive::NVIDIAExecutableKernel::launchGrid(" << width 
		<< ", " << height << ")");
	CUresult result;

	result = cuda::CudaDriver::cuLaunchGrid(cuFunction, width, height);
	if (result != CUDA_SUCCESS) {
		report("  - cuLaunchGrid() failed: " << result);
		throw hydrazine::Exception("cuLaunchGrid() failed ");
	}
	else {
		report("  - cuLaunchGrid() succeeded!");
	}
	
	result = cuda::CudaDriver::cuCtxSynchronize();
	if (result != CUDA_SUCCESS) {
		report("  - cuCtxSynchronize() after cuLaunchGrid() failed: " << result);
		throw hydrazine::Exception("cuLaunchGrid() failed ");
	}
	else {
		report("  - cuCtxSynchronize() after cuLaunchGrid() succeeded!");
	}
}

/*!
	Sets the shape of a kernel
*/
void executive::NVIDIAExecutableKernel::setKernelShape(int x, int y, int z) {
	CUresult result = cuda::CudaDriver::cuFuncSetBlockShape(cuFunction, x, y, z);
	if (result != CUDA_SUCCESS) {
		report("failed to set kernel shape with result " << result);
		throw hydrazine::Exception("NVIDIAExecutableKernel::setKernelShape() failed");
	}
}

void executive::NVIDIAExecutableKernel::setDevice(const Device* device,
	unsigned int limit) {
}

void executive::NVIDIAExecutableKernel::setExternSharedMemorySize(unsigned int bytes) {
	_sharedMemorySize = bytes;
	CUresult result;
	result = cuda::CudaDriver::cuFuncSetSharedSize(cuFunction, bytes);
	if (result != CUDA_SUCCESS) {
		report("  - cuFuncSetSharedSize(" << bytes << " bytes) FAILED: " << result);
		throw hydrazine::Exception("cuFuncSetSharedSize() failed");
	}
	else {
		report("  - cuFuncSetSharedSize(" << bytes << " bytes) succeeded");
	}
}

void executive::NVIDIAExecutableKernel::updateArgumentMemory() {
	configureArguments();
}

void executive::NVIDIAExecutableKernel::updateMemory() {
	updateGlobalMemory();
	updateConstantMemory();
}

executive::ExecutableKernel::TextureVector 
	executive::NVIDIAExecutableKernel::textureReferences() const {
	assertM(false, "no support for getting texture references");
	
	return executive::ExecutableKernel::TextureVector();
}

void executive::NVIDIAExecutableKernel::updateGlobalMemory() {

}

void executive::NVIDIAExecutableKernel::updateConstantMemory() {

}

void executive::NVIDIAExecutableKernel::addTraceGenerator(
	trace::TraceGenerator *generator)
{
	assertM(false, "No trace generation support in GPU kernel.");
}

void executive::NVIDIAExecutableKernel::removeTraceGenerator(
	trace::TraceGenerator *generator)
{
	assertM(false, "No trace generation support in GPU kernel.");	
}


void executive::NVIDIAExecutableKernel::setExternalFunctionSet(
	const ir::ExternalFunctionSet& s) {
	assertM(false, "No external function support in GPU kernel.");	

}

void executive::NVIDIAExecutableKernel::clearExternalFunctionSet() {
	assertM(false, "No external function support in GPU kernel.");	
}


void executive::NVIDIAExecutableKernel::setWorkerThreads(unsigned int limit) {

}

void executive::NVIDIAExecutableKernel::configureArguments() {
	report("executive::NVIDIAExecutableKernel::configureArguments() - size: " 
		<< _argumentMemorySize);

	char *argBuffer = new char[_argumentMemorySize];
	getArgumentBlock((unsigned char*)argBuffer, _argumentMemorySize);

	if (cuda::CudaDriver::cuParamSetSize(cuFunction,
		_argumentMemorySize) != CUDA_SUCCESS) {
		delete [] argBuffer;
		Ocelot_Exception("NVIDIAExecutableKernel::configureArguments() " 
			<< "- failed to set parameter size to " << _argumentMemorySize);
	}
	if (cuda::CudaDriver::cuParamSetv(cuFunction, 0, argBuffer,
		_argumentMemorySize) != CUDA_SUCCESS) {
		delete [] argBuffer;
		Ocelot_Exception("NVIDIAExecutableKernel::configureArguments() " << 
			"- failed to set parameter data");
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////
