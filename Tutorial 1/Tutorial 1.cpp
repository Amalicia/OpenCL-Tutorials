#include <iostream>
#include <vector>

#include "Utils.h"

void print_help() {
	std::cerr << "Application usage:" << std::endl;

	std::cerr << "  -p : select platform " << std::endl;
	std::cerr << "  -d : select device" << std::endl;
	std::cerr << "  -l : list all platforms and devices" << std::endl;
	std::cerr << "  -h : print this message" << std::endl;
}

int main(int argc, char **argv) {
	//Part 1 - handle command line options such as device selection, verbosity, etc.
	int platform_id = 0;
	int device_id = 0;

	for (int i = 1; i < argc; i++)	{
		if ((strcmp(argv[i], "-p") == 0) && (i < (argc - 1))) { platform_id = atoi(argv[++i]); }
		else if ((strcmp(argv[i], "-d") == 0) && (i < (argc - 1))) { device_id = atoi(argv[++i]); }
		else if (strcmp(argv[i], "-l") == 0) { std::cout << ListPlatformsDevices() << std::endl; }
		else if (strcmp(argv[i], "-h") == 0) { print_help(); return 0; }
	}

	//detect any potential exceptions
	try {
		//Part 2 - host operations
		//2.1 Select computing devices
		cl::Context context = GetContext(platform_id, device_id);

		//display the selected device
		std::cout << "Runinng on " << GetPlatformName(platform_id) << ", " << GetDeviceName(platform_id, device_id) << std::endl;

		//create a queue to which we will push commands for the device
		// cl::CommandQueue queue(context);		<- commented out due to profiling
		// Task 4 - enable profiling
		cl::CommandQueue  queue(context, CL_QUEUE_PROFILING_ENABLE);

		//2.2 Load & build the device code
		cl::Program::Sources sources;

		AddSources(sources, "kernels/my_kernels.cl");

		cl::Program program(context, sources);

		//build and debug the kernel code
		try {
			program.build();
		}
		catch (const cl::Error& err) {
			std::cout << "Build Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
			std::cout << "Build Options:\t" << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
			std::cout << "Build Log:\t " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
			throw err;
		}

		//Part 3 - memory allocation
		//host - input
		// commented out to play around with vector sizes
		//std::vector<int> A = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }; //C++11 allows this type of initialisation
		//std::vector<int> B = { 0, 1, 2, 0, 1, 2, 0, 1, 2, 0 };
		/*std::vector<int> A(1000000);
		std::vector<int> B(1000000);*/

		// float vectors
		std::vector<float> A(100000);
		std::vector<float> B(100000);
		
		size_t vector_elements = A.size();//number of elements
		//size_t vector_size = A.size()*sizeof(int);//size in bytes
		size_t vector_size = A.size() * sizeof(float);

		//host - output
		std::vector<float> C(vector_elements);

		//device - buffers
		cl::Buffer buffer_A(context, CL_MEM_READ_WRITE, vector_size);
		cl::Buffer buffer_B(context, CL_MEM_READ_WRITE, vector_size);
		cl::Buffer buffer_C(context, CL_MEM_READ_WRITE, vector_size);

		//Part 4 - device operations

		//4.1 Copy arrays A and B to device memory
		queue.enqueueWriteBuffer(buffer_A, CL_TRUE, 0, vector_size, &A[0]);
		queue.enqueueWriteBuffer(buffer_B, CL_TRUE, 0, vector_size, &B[0]);

		// create multiplication kernel
		/*cl::Kernel kernel_mult = cl::Kernel(program, "mult");
		kernel_mult.setArg(0, buffer_A);
		kernel_mult.setArg(1, buffer_B);
		kernel_mult.setArg(2, buffer_C)*/;

		//4.2 Setup and execute the kernel (i.e. device code)
		// commented to allow us to perform A * B + B
		/*cl::Kernel kernel_add = cl::Kernel(program, "add");
		kernel_add.setArg(0, buffer_A);
		kernel_add.setArg(1, buffer_B);
		kernel_add.setArg(2, buffer_C);*/
	/*	cl::Kernel kernel_add = cl::Kernel(program, "add");
		kernel_add.setArg(0, buffer_C);
		kernel_add.setArg(1, buffer_B);
		kernel_add.setArg(2, buffer_C);*/

		/*cl::Kernel kernel_mult_add = cl::Kernel(program, "mult_add");
		kernel_mult_add.setArg(0, buffer_A);
		kernel_mult_add.setArg(1, buffer_B);
		kernel_mult_add.setArg(2, buffer_C);*/

		cl::Kernel kernel_addf = cl::Kernel(program, "mult_add");
		kernel_addf.setArg(0, buffer_A);
		kernel_addf.setArg(1, buffer_B);
		kernel_addf.setArg(2, buffer_C);

		//perform multiplication first
		// commented to perform single mult add operation
		//queue.enqueueNDRangeKernel(kernel_mult, cl::NullRange, cl::NDRange(vector_elements), cl::NullRange);

		//// queue.enqueueNDRangeKernel(kernel_add, cl::NullRange, cl::NDRange(vector_elements), cl::NullRange);		<-- commented for same reason as above
		////Task 4 - create event for queue
		cl::Event prof_event;
		//queue.enqueueNDRangeKernel(kernel_add, cl::NullRange, cl::NDRange(vector_elements), cl::NullRange, NULL, &prof_event);

		//queue.enqueueNDRangeKernel(kernel_mult_add, cl::NullRange, cl::NDRange(vector_elements), cl::NullRange, NULL, &prof_event);

		queue.enqueueNDRangeKernel(kernel_addf, cl::NullRange, cl::NDRange(vector_elements), cl::NullRange, NULL, &prof_event);

		//4.3 Copy the result from device to host
		queue.enqueueReadBuffer(buffer_C, CL_TRUE, 0, vector_size, &C[0]);

		// commenetd out to avoid console spam for large vextors
		/*std::cout << "A = " << A << std::endl;
		std::cout << "B = " << B << std::endl;
		std::cout << "C = " << C << std::endl;*/

		//Task 4 - Display kernel execution time
		std::cout << "Kernel execution time [ns]: " << prof_event.getProfilingInfo<CL_PROFILING_COMMAND_END>() - prof_event.getProfilingInfo<CL_PROFILING_COMMAND_START>() << std::endl;
		std::cout << GetFullProfilingInfo(prof_event, ProfilingResolution::PROF_US) << endl;

		// Not a huge ammount of difference between the device and platform on lab machine. Device normally pefroms better
	}
	catch (cl::Error err) {
		std::cerr << "ERROR: " << err.what() << ", " << getErrorString(err.err()) << std::endl;
	}

	return 0;
}