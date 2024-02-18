#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <CL/cl.h>
#include <CL/cl_gl.h>

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

// OpenCL kernel source code
const char* kernelSource =
"__kernel void generateColors(__write_only image2d_t texture) { \n"
"    const int2 pos = {get_global_id(0), get_global_id(1)}; \n"
"    uint4 color; \n"
"    color.x = pos.x * 255 / 800; \n" // Red component based on X position
"    color.y = pos.y * 255 / 600; \n" // Green component based on Y position
"    color.z = (pos.x + pos.y) * 255 / (800 + 600); \n" // Blue component based on X and Y position
"    color.w = 255; \n" // Alpha component
"    write_imageui(texture, pos, color); \n"
"}";

int main() {
    // Initialize GLFW and create window
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL-OpenCL Interoperability", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    glewInit();

    // Set up OpenGL viewport and projection matrix
    glViewport(0, 0, WIDTH, HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WIDTH, HEIGHT, 0, 0, 1);
    glMatrixMode(GL_MODELVIEW);

    // Create VBO for triangle vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  // bottom left
         0.5f, -0.5f, 0.0f,  // bottom right
         0.0f,  0.5f, 0.0f   // top
    };

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Initialize OpenCL
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;

    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);

    // Create shared context between OpenGL and OpenCL
    /*
    cl_context_properties props[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties)glfwGetWGLContext(window),
        CL_WGL_HDC_KHR, (cl_context_properties)GetDC((HWND)glfwGetWin32Window(window)),
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
        0
    };
    
    context = clCreateContext(props, 1, &device, NULL, NULL, NULL);
    */


    
    queue = clCreateCommandQueue(context, device, 0, NULL);

    program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, NULL, NULL);
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    kernel = clCreateKernel(program, "generateColors", NULL);

    // Create OpenGL texture object
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Register OpenGL texture object with OpenCL
    cl_mem clTexture = clCreateFromGLTexture(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, texture, NULL);

    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &clTexture);

    // Execute OpenCL kernel
    size_t global_work_size[2] = { WIDTH, HEIGHT };
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_work_size, NULL, 0, NULL, NULL);
    clFinish(queue);

    // Cleanup OpenCL resources
    clReleaseMemObject(clTexture);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    // Set up OpenGL shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\0";
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragmentShaderSource = "#version 330 core\n"
        "uniform sampler2D textureSampler;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = texture(textureSampler, gl_FragCoord.xy / vec2(800, 600));\n"
        "}\n\0";
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Link shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    // Set texture sampler uniform
    glUniform1i(glGetUniformLocation(shaderProgram, "textureSampler"), 0);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Render the triangle
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup GLFW
    glfwTerminate();

    return 0;
}
