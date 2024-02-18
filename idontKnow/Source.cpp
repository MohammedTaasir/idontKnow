#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <Windows.h>
#include <gl/GL.h>
#include <GLFW/glfw3.h>
#include <CL/cl.h>
#include <CL/cl_gl.h>

#include <stdio.h>
#include <stdlib.h>

static void error_callback(int error, const char* description) {
    fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(void) {
    GLFWwindow* window;
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        exit(EXIT_FAILURE);
    window = glfwCreateWindow(512, 512, "Simple example", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glEnable(GL_TEXTURE_2D);
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    // Initialize OpenCL
    cl_platform_id platform_id = NULL;
    cl_device_id device_id = NULL;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

    cl_context_properties properties[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform_id,
        CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
        CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
        0
    };

    cl_context ctx = clCreateContext(properties, 1, &device_id, NULL, NULL, &ret);
    if (ret != CL_SUCCESS)
        printf("Context creation => OpenCL error: %d\n", ret);

    // Create a command queue
    cl_command_queue queue = clCreateCommandQueue(ctx, device_id, 0, &ret);

    //cl_command_queue queue = clCreateCommandQueue(ctx, device_id, CL_QUEUE_PROFILING_ENABLE, &ret);



    // Create Image
    cl_mem image = clCreateFromGLTexture2D(ctx, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, texture, &ret);
    if (ret != CL_SUCCESS)
        printf("Texture creation => OpenCL error: %d\n", ret);

    // Create Buffer RGB * width * height
    cl_mem buffer = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, 3 * width * height, NULL, &ret);
    if (ret != CL_SUCCESS)
        printf("Buffer creation => OpenCL error: %d\n", ret);

    while (!glfwWindowShouldClose(window)) {
        float ratio;
        ratio = width / (float)height;
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotatef(0.f, 0.f, 0.f, 1.f);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBegin(GL_QUADS);
        glColor3f(1.f, 0.f, 0.f);
        glVertex3f(-1.f, -1.f, 0.f);
        glColor3f(0.f, 1.f, 0.f);
        glVertex3f(-1.f, 1.f, 0.f);
        glColor3f(0.f, 0.f, 1.f);
        glVertex3f(1.f, 1.f, 0.f);
        glColor3f(0.f, 0.f, 1.f);
        glVertex3f(1.f, -1.f, 0.f);
        glEnd();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ret = clFlush(queue);
    ret = clFinish(queue);
    ret = clReleaseMemObject(image);
    ret = clReleaseMemObject(buffer);
    ret = clReleaseCommandQueue(queue);
    ret = clReleaseContext(ctx);

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
