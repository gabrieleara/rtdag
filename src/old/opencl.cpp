#include <string>
#include <vector>

// #include <CL_2_0/CL/cl.hpp>
#include <CL/cl.hpp>

#define OPENCL_COUT (std::cout << "OpenCL: ")
#define OPENCL_CERR (std::cerr << "OpenCL: ERROR: ")

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║             Static Function Declarations              ║
 * ╚═══════════════════════════════════════════════════════╝
 */

static cl::Platform cl_get_platform();
static cl::Program cl_get_program(const cl::Context &context,
                                  const cl::Device &device,
                                  const std::string &source_code);

cl::Platform cl_get_platform() {
    // Get all valid platforms
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.size() < 1) {
        OPENCL_CERR << "No platforms!\n";
        exit(EXIT_FAILURE);
    }

    // Assume that the first platform is the one we are interested in:
    auto cl_platform = all_platforms[0];
    OPENCL_COUT << "Selecting platform\t"
                << cl_platform.getInfo<CL_PLATFORM_NAME>() << '\n';

    return cl_platform;
}

cl::Program cl_get_program(const cl::Context &context, const cl::Device &device,
                           const std::string &source_code) {
    // Create the source code for our program to run using OpenCL
    auto sources = cl::Program::Sources();
    sources.push_back({source_code.c_str(), source_code.length()});

    // The program will be compiled now
    cl::Program program(context, sources);
    if (program.build({device}) != CL_SUCCESS) {
        OPENCL_CERR << "Could not build: "
                    << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device)
                    << "\n";
        exit(EXIT_FAILURE);
    }

    return program;
}

// Defines a functor for an OpenCL
struct My_CL_Functor : My_Functor_Prototype {
    // Order of these attributes is important! It defines the order of
    // initialization during construction!
    cl::Context context;
    cl::Program program;
    cl::CommandQueue queue;
    cl::Kernel kernel;
    size_t matrix_size;
    cl::Buffer buffer_A;
    cl::Buffer buffer_B;
    cl::Buffer buffer_C;

    static constexpr size_t vsize = sizeof(float);

    static std::unique_ptr<My_CL_Functor> create(const std::string &source_code,
                                                 const std::string &function,
                                                 size_t size) {
        cl::Platform platform = cl_get_platform();
        cl::Device device = cl_get_device();

        return std::make_unique<My_CL_Functor>(platform, device, source_code,
                                               function, size);
    }

    void operator()(const float *A, const float *B, float *C) override {
        // Copy input arrays A and B to the device
        queue.enqueueWriteBuffer(buffer_A, CL_TRUE, 0, vsize * matrix_size, A);
        queue.enqueueWriteBuffer(buffer_B, CL_TRUE, 0, vsize * matrix_size, B);

        // Invoke kernel and immediately wait for completion
        queue.enqueueNDRangeKernel(kernel, cl::NullRange,
                                   cl::NDRange(matrix_size / vsize),
                                   cl::NullRange);
        queue.finish();

        // And we read the result in C from the device
        queue.enqueueReadBuffer(buffer_C, CL_TRUE, 0, vsize * matrix_size, C);
    }

    ~My_CL_Functor() override = default;

private:
    My_CL_Functor(const cl::Platform &platform, const cl::Device &device,
                  const std::string &source_code, const std::string &function) :
        context({device}),
        program(cl_get_program(device, source_code)),

        // Create queue to which we will push commands for the device and a
        // reference to our kernel function
        queue(context, device),
        kernel(program, func_name),
        matrix_size(vsize * size * size),

        // Create one buffer per parameter
        buffer_A(context, CL_MEM_READ_WRITE, matrix_size),
        buffer_B(context, CL_MEM_READ_WRITE, matrix_size),
        buffer_C(context, CL_MEM_READ_WRITE, matrix_size) {

        int argcnt = 0;
        // Bind kernel arguments to buffers
        kernel.setArg(argcnt++, size);
        kernel.setArg(argcnt++, buffer_A);
        kernel.setArg(argcnt++, buffer_B);
        kernel.setArg(argcnt++, buffer_C);
        kernel.setArg(argcnt++, repetitions);
    }
};

std::unique_ptr<My_Functor_Type> functor_factory(const std::string &type) {
    if (type == 'OpenCL') {
        return My_CL_Functor::create(
            R"(((
            // First naive implementation

            void kernel naive_matrix_mul(
                const size_t MATRIX_SIZE, const global float *A,
                const global float *B, global float *C) {
                // Thread identifiers will give us output element ID of C
                const int idx = get_global_id(0);
                const int row = idx % MATRIX_SIZE;
                const int col = idx / MATRIX_SIZE;

                // Perform matrix multiplication by iterating over elements to
                // be multiplied
                float acc = 0.0f;
                for (int i = 0; i < MATRIX_SIZE; ++i) {
                    acc += A[i * MATRIX_SIZE + row] * B[col * MATRIX_SIZE + i];
                }

                // Store the result
                C[row * MATRIX_SIZE + col] = acc;
            }

            void kernel repeated_naive_matrix_mul(
                const size_t MATRIX_SIZE, const global float *A,
                const global float *B, global float *C, int repeat) {
                // Repeat multiple times to hopefully elongate computation time
                for (int r = 0; r < repeat; ++r) {
                    naive_matrix_mul(MATRIX_SIZE, A, B, C);
                }
            }
            )))",
            "repeated_naive_matrix_mul", 4);
    }

    return nullptr;
}

// WORK IN PROGRESS...
