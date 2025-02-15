#include <getopt.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <omp.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#define CL_HPP_TARGET_OPENCL_VERSION 220
#define CL_HPP_ENABLE_EXCEPTIONS
#include <CL/opencl.hpp>
#include <GL/freeglut.h>
#include <GL/glx.h>

// ----------------------------------------------------------------------
// game variables
// ----------------------------------------------------------------------
static int paused = 0;
static std::vector<cl_char> field_image;
static cl_int field_width = 1024;
static cl_int field_height = 1024;
static size_t step = 0;

// ----------------------------------------------------------------------
// cl kernel
// ----------------------------------------------------------------------
static const char *kernel_source = "bz_kernel.cl";

// ----------------------------------------------------------------------
// cl variables
// ----------------------------------------------------------------------
static cl::Platform platform;
static cl::Device device;
static cl::Context context;
static cl::CommandQueue command_queue;
static cl::Program program;
static cl::Kernel bz_kernel_diffusion_a;
static cl::Kernel bz_kernel_diffusion_b;
static cl::Kernel bz_kernel_diffusion_c;
static cl::Kernel bz_kernel_reaction;
static cl::Kernel bz_kernel_draw_image;
static cl::Buffer dev_field_a;
static cl::Buffer dev_field_b;
static cl::Buffer dev_field_c;
static cl::Buffer dev_field_a2;
static cl::Buffer dev_field_b2;
static cl::Buffer dev_field_c2;
static cl::Memory dev_image;

//
static cl_float diffusion_rate = 0.9;
static cl_float param_a = 1.2;
static cl_float param_b = 1.0;
static cl_float param_c = 1.0;

// ----------------------------------------------------------------------
// work size info
// ----------------------------------------------------------------------
static std::vector<cl_int> elements_size;
static std::vector<cl_int> global_work_size;
static std::vector<cl_int> local_work_size;

// ----------------------------------------------------------------------
// gl variables
// ----------------------------------------------------------------------
static int gen_mills = 0;
static int refresh_mills = 10.0/30.0;  // refresh interval in milliseconds
static bool full_screen_mode = false;
static char title[] = "B-Z Reaction on OpenCL";
static int window_width  = 1024;     // Windowed mode's width
static int window_height = 1024;     // Windowed mode's height
static int window_pos_x   = 50;      // Windowed mode's top-left corner x
static int window_pos_y   = 50;      // Windowed mode's top-left corner y
static GLfloat translate_x = 0.0f;
static GLfloat translate_y = 0.0f;
static GLfloat zoom = 1.0f;
static const GLfloat ORTHO_LEFT = -1.0f;
static const GLfloat ORTHO_RIGHT = 1.0f;
static const GLfloat ORTHO_TOP = 1.0f;
static const GLfloat ORTHO_BOTTOM = -1.0f;
static clock_t wall_clock = 0;

static GLuint rendered_texture;

static void report_cl_error(const cl::Error& err) {
  const char* s = "-";
  switch (err.err()) {
#define CASE_CL_CODE(code)\
    case code: s = #code; break
    CASE_CL_CODE(CL_DEVICE_NOT_FOUND);
    CASE_CL_CODE(CL_DEVICE_NOT_AVAILABLE);
    CASE_CL_CODE(CL_COMPILER_NOT_AVAILABLE);
    CASE_CL_CODE(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    CASE_CL_CODE(CL_OUT_OF_RESOURCES);
    CASE_CL_CODE(CL_OUT_OF_HOST_MEMORY);
    CASE_CL_CODE(CL_PROFILING_INFO_NOT_AVAILABLE);
    CASE_CL_CODE(CL_MEM_COPY_OVERLAP);
    CASE_CL_CODE(CL_IMAGE_FORMAT_MISMATCH);
    CASE_CL_CODE(CL_IMAGE_FORMAT_NOT_SUPPORTED);
    CASE_CL_CODE(CL_BUILD_PROGRAM_FAILURE);
    CASE_CL_CODE(CL_MAP_FAILURE);
#ifdef CL_VERSION_1_1
    CASE_CL_CODE(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    CASE_CL_CODE(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
#endif
#ifdef CL_VERSION_1_2
    CASE_CL_CODE(CL_COMPILE_PROGRAM_FAILURE);
    CASE_CL_CODE(CL_LINKER_NOT_AVAILABLE);
    CASE_CL_CODE(CL_LINK_PROGRAM_FAILURE);
    CASE_CL_CODE(CL_DEVICE_PARTITION_FAILED);
    CASE_CL_CODE(CL_KERNEL_ARG_INFO_NOT_AVAILABLE);
#endif
    CASE_CL_CODE(CL_INVALID_VALUE);
    CASE_CL_CODE(CL_INVALID_DEVICE_TYPE);
    CASE_CL_CODE(CL_INVALID_PLATFORM);
    CASE_CL_CODE(CL_INVALID_DEVICE);
    CASE_CL_CODE(CL_INVALID_CONTEXT);
    CASE_CL_CODE(CL_INVALID_QUEUE_PROPERTIES);
    CASE_CL_CODE(CL_INVALID_COMMAND_QUEUE);
    CASE_CL_CODE(CL_INVALID_HOST_PTR);
    CASE_CL_CODE(CL_INVALID_MEM_OBJECT);
    CASE_CL_CODE(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
    CASE_CL_CODE(CL_INVALID_IMAGE_SIZE);
    CASE_CL_CODE(CL_INVALID_SAMPLER);
    CASE_CL_CODE(CL_INVALID_BINARY);
    CASE_CL_CODE(CL_INVALID_BUILD_OPTIONS);
    CASE_CL_CODE(CL_INVALID_PROGRAM);
    CASE_CL_CODE(CL_INVALID_PROGRAM_EXECUTABLE);
    CASE_CL_CODE(CL_INVALID_KERNEL_NAME);
    CASE_CL_CODE(CL_INVALID_KERNEL_DEFINITION);
    CASE_CL_CODE(CL_INVALID_KERNEL);
    CASE_CL_CODE(CL_INVALID_ARG_INDEX);
    CASE_CL_CODE(CL_INVALID_ARG_VALUE);
    CASE_CL_CODE(CL_INVALID_ARG_SIZE);
    CASE_CL_CODE(CL_INVALID_KERNEL_ARGS);
    CASE_CL_CODE(CL_INVALID_WORK_DIMENSION);
    CASE_CL_CODE(CL_INVALID_WORK_GROUP_SIZE);
    CASE_CL_CODE(CL_INVALID_WORK_ITEM_SIZE);
    CASE_CL_CODE(CL_INVALID_GLOBAL_OFFSET);
    CASE_CL_CODE(CL_INVALID_EVENT_WAIT_LIST);
    CASE_CL_CODE(CL_INVALID_EVENT);
    CASE_CL_CODE(CL_INVALID_OPERATION);
    CASE_CL_CODE(CL_INVALID_GL_OBJECT);
    CASE_CL_CODE(CL_INVALID_BUFFER_SIZE);
    CASE_CL_CODE(CL_INVALID_MIP_LEVEL);
    CASE_CL_CODE(CL_INVALID_GLOBAL_WORK_SIZE);
#ifdef CL_VERSION_1_1
    CASE_CL_CODE(CL_INVALID_PROPERTY);
#endif
#ifdef CL_VERSION_1_2
    CASE_CL_CODE(CL_INVALID_IMAGE_DESCRIPTOR);
    CASE_CL_CODE(CL_INVALID_COMPILER_OPTIONS);
    CASE_CL_CODE(CL_INVALID_LINKER_OPTIONS);
    CASE_CL_CODE(CL_INVALID_DEVICE_PARTITION_COUNT);
#endif
#ifdef CL_VERSION_2_0
    CASE_CL_CODE(CL_INVALID_PIPE_SIZE);
    CASE_CL_CODE(CL_INVALID_DEVICE_QUEUE);
#endif
#ifdef CL_VERSION_2_2
    CASE_CL_CODE(CL_INVALID_SPEC_ID);
    CASE_CL_CODE(CL_MAX_SIZE_RESTRICTION_EXCEEDED);
#endif
    CASE_CL_CODE(CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR);
  }
  std::cerr << "caught exception: " << err.what() <<
    ": " << s << "(" << err.err() << ")" << std::endl;
}

// ----------------------------------------------------------------------
// gl functions
// ----------------------------------------------------------------------
static void glCheck_(const char* target) {
  const GLenum st = glGetError();
  if (st) {
    std::cout << target << ": " << gluErrorString(st) << std::endl;
  }
}
static void display_cb() {
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glOrtho(ORTHO_LEFT, ORTHO_RIGHT,
          ORTHO_BOTTOM, ORTHO_TOP,
          -10, 10);
  glScalef(zoom, zoom, 1.0f);
  glTranslatef(translate_x, translate_y, 0.0f);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, rendered_texture);
  glCheck_("glBindTexture");
  glBegin(GL_TRIANGLES);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 0.0f);
  glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f,  -1.0f, 0.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f,   1.0f, 0.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f,   1.0f, 0.0f);
  glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, 0.0f);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 0.0f);
  glEnd();
  glutSwapBuffers();
}

static void reshape_cb(GLsizei width, GLsizei height) {
  // Set the viewport to cover the new window
  glViewport(0, 0, width, height);

  // Set the aspect ratio of the clipping area to match the viewport
  glMatrixMode(GL_PROJECTION);  // To operate on the Projection matrix
  glLoadIdentity();             // Reset the projection matrix
}

static void specialKeys_cb(int key, int x, int y) {
  switch (key) {
  case GLUT_KEY_F1:    // F1: Toggle between full-screen and windowed mode
    full_screen_mode = !full_screen_mode;         // Toggle state
    if (full_screen_mode) {                     // Full-screen mode
      // Save parameters for restoring later
      window_pos_x   = glutGet(GLUT_WINDOW_X);
      window_pos_y   = glutGet(GLUT_WINDOW_Y);
      window_width  = glutGet(GLUT_WINDOW_WIDTH);
      window_height = glutGet(GLUT_WINDOW_HEIGHT);
      // Switch into full screen
      glutFullScreen();
    } else {
      // Windowed mode
      // Switch into windowed mode
      glutReshapeWindow(window_width, window_height);
      // Position top-left corner
      glutPositionWindow(window_pos_x, window_pos_y);
    }
    break;
  case GLUT_KEY_HOME:
    zoom = 1.0f;
    translate_x = 0.0f;
    translate_y = 0.0f;
    glutPostRedisplay();
    break;
  case GLUT_KEY_END:
    glutLeaveMainLoop();
    break;
  }
}
static void nonspecialKeys_cb(unsigned char key, int x, int y) {
  switch (key) {
  case 'p':
    if (paused == 0) {
      paused = 2;
    } else {
      paused = 0;
    }
    break;
  case 'q':
    glutLeaveMainLoop();
    break;
  }
}
static void
mouse_cb(int button, int state, int x, int y) {
  switch (button) {
  case GLUT_LEFT_BUTTON:
    if (state == GLUT_DOWN) {
      translate_x -=
        ((ORTHO_RIGHT - ORTHO_LEFT) * x / window_width + ORTHO_LEFT) / zoom;
      translate_y -=
        ((ORTHO_BOTTOM - ORTHO_TOP) * y / window_height + ORTHO_TOP) / zoom;
      glutPostRedisplay();
    }
    break;
  case GLUT_RIGHT_BUTTON:
    if (state == GLUT_DOWN) {
      translate_x = 0.0f;
      translate_y = 0.0f;
      zoom = 1.0f;
      glutPostRedisplay();
    }
    break;
  }
}
static void
wheel_cb(int wheel, int direction, int x, int y) {
  if (direction == 1) {
    zoom += 0.1f;
  } else {
    zoom -= 0.1f;
  }
  zoom = std::max(zoom, 1.0f);
  glutPostRedisplay();
}

static void initGL(int argc, char *argv[]) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(window_width, window_height);
  glutInitWindowPosition(window_pos_x, window_pos_y);
  glutCreateWindow(title);

  glutDisplayFunc(display_cb);
  glutReshapeFunc(reshape_cb);
  // Register callback handler for special-key event
  glutSpecialFunc(specialKeys_cb);
  glutKeyboardFunc(nonspecialKeys_cb);
  glutMouseFunc(mouse_cb);
  glutMouseWheelFunc(wheel_cb);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  // texture
  glEnable(GL_TEXTURE_2D);

  glGenTextures(1, &rendered_texture);
  glCheck_("glGenTexture");
  glBindTexture(GL_TEXTURE_2D, rendered_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F,
               field_width, field_height,
               0, GL_RGBA, GL_FLOAT, 0);
  glCheck_("glTexImage2D");
  glFinish();
}

static void displayTimer_cb(int dummy) {
  glutPostRedisplay();
  glutTimerFunc(refresh_mills, displayTimer_cb, 0);
}

static int step_count = 0;

static void generationTimer_cb(int dummy) {
  if (paused == 1) {
    glutTimerFunc(gen_mills, generationTimer_cb, 0);
    return;
  }
  try {
    std::vector<cl::Memory> dev_image_vec({dev_image});
    command_queue.enqueueAcquireGLObjects(&dev_image_vec);
    command_queue.enqueueNDRangeKernel(bz_kernel_diffusion_a,
                                       cl::NullRange,
                                       cl::NDRange(global_work_size[0],
                                                   global_work_size[1]),
                                       cl::NDRange(local_work_size[0],
                                                   local_work_size[1]));
    command_queue.enqueueNDRangeKernel(bz_kernel_diffusion_b,
                                       cl::NullRange,
                                       cl::NDRange(global_work_size[0],
                                                   global_work_size[1]),
                                       cl::NDRange(local_work_size[0],
                                                   local_work_size[1]));
    command_queue.enqueueNDRangeKernel(bz_kernel_diffusion_c,
                                       cl::NullRange,
                                       cl::NDRange(global_work_size[0],
                                                   global_work_size[1]),
                                       cl::NDRange(local_work_size[0],
                                                   local_work_size[1]));
    command_queue.enqueueNDRangeKernel(bz_kernel_reaction,
                                       cl::NullRange,
                                       cl::NDRange(global_work_size[0],
                                                   global_work_size[1]),
                                       cl::NDRange(local_work_size[0],
                                                   local_work_size[1]));
    command_queue.enqueueNDRangeKernel(bz_kernel_draw_image,
                                       cl::NullRange,
                                       cl::NDRange(global_work_size[0],
                                                   global_work_size[1]),
                                       cl::NDRange(local_work_size[0],
                                                   local_work_size[1]));
    //command_queue.finish();
    command_queue.flush();

    command_queue.enqueueReleaseGLObjects(&dev_image_vec);
    ++step;
    ++step_count;

    const clock_t now = clock();
    if (now > wall_clock + CLOCKS_PER_SEC) {
      const double fps = static_cast<double>(step_count)
        / ((now - wall_clock) / CLOCKS_PER_SEC);
      step_count = 0;
      static std::string report;
      std::stringstream ss;
      ss << "step[" << step << "],fps[" << fps << "]";
      std::string s = ss.str();
      if (s.size() < report.size()) {
        s += std::string(report.size() - s.size(), ' ');
      }
      report = s;
      std::cout << report << "\r" << std::flush;
      wall_clock = now;
    }
    if (paused == 2) {
      paused = 1;
    }
    glutTimerFunc(gen_mills, generationTimer_cb, 0);
  } catch (const cl::Error& err) {
    std::cerr << err.what() << std::endl;
    throw;
  }
}

static void startGL() {
  glutTimerFunc(0, displayTimer_cb, 0);
  glutTimerFunc(0, generationTimer_cb, 0);
  glutMainLoop();
}

// ----------------------------------------------------------------------
// utility function
// ----------------------------------------------------------------------
static std::string loadProgramSource(const char *filename) {
  std::ifstream ifs(filename);
  const std::string content((std::istreambuf_iterator<char>(ifs)),
                            (std::istreambuf_iterator<char>()));
  return content;
}

// ----------------------------------------------------------------------
// game functions
// ----------------------------------------------------------------------

void bz_init_random_field(std::vector<cl_float>& field_init,
                          unsigned& seed) {
  for (cl_int y = 0; y < field_height; ++y) {
    for (cl_int x = 0; x < field_width; ++x) {
      if (y > field_height / 3 && y < 2 * field_height / 3 &&
          x > field_width / 3 && x < 2 * field_width / 3) {
        const cl_float c = (rand_r(&seed) % 256) / 256.0;
        field_init[y * field_width + x] = c;
      } else {
        field_init[y * field_width + x] = 0.5;
      }
    }
  }
}

inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

bool with_cl_gl_sharing(const std::string& extensions) {
  std::stringstream ex_ss(extensions);
  std::string ex_item;
  while (std::getline(ex_ss, ex_item, ' ')) {
    if (ex_item == "cl_khr_gl_sharing") {
      return true;
    }
  }
  return false;
}

int main(int argc, char *argv[]) {
  try {
    size_t device_index = 0;
    for (;;) {
      int option_index = 0;
      static struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
        {"width", required_argument, 0, 'w'},
        {"height", required_argument, 0, 'h'},
        {"interval", required_argument, 0, 'i'},
        {"diffusion", required_argument, 0, 'D'},
        {"pause", no_argument, 0, 'P'},
        {0, 0, 0}};
      int c = getopt_long(argc, argv, "w:h:n:i:d:a:b:c:P",
                          long_options, &option_index);
      if (c == -1) {
        break;
      }
      switch (c) {
      case 'd':
        device_index = atoi(optarg);
        break;
      case 'w':
        {
          const int w = atoi(optarg);
          field_width = w;
          window_width = w;
        }
        break;
      case 'h':
        {
          const int h = atoi(optarg);
          field_height = h;
          window_height = h;
        }
        break;
      case 'i':
        gen_mills = atoi(optarg);
        break;
      case 'D':
        diffusion_rate = atof(optarg);
        if (diffusion_rate < 0.0 ||
            diffusion_rate > 1.0) {
          std::cerr << "diffusion rate "
                    << diffusion_rate << " is out of range." << std::endl;
        }
        break;
      case 'a':
        param_a = atof(optarg);
        break;
      case 'b':
        param_b = atof(optarg);
        break;
      case 'c':
        param_c = atof(optarg);
        break;
      case 'P':
        paused = 2;
        break;
      default:
        std::cerr << "Usage: " << argv[0] <<
          " [--device N]"
          " [-w width]"
          " [-h height]"
          " [-i interval_millis]"
          " [-D diffusion_rate]"
          " [-P]" << std::endl;
        std::cerr << " -d, --device    : Select compute device." << std::endl;
        std::cerr << " -w, --width     : Field width." << std::endl;
        std::cerr << " -h, --height    : Field height." << std::endl;
        std::cerr << " -i, --interval  : Step interval in milli seconds." << std::endl;
        std::cerr << " -D, --diffusion : Diffusion rate (0.0 ~ 1.0)." << std::endl;
        std::cerr << " -a              : Parameter a." << std::endl;
        std::cerr << " -b              : Parameter b." << std::endl;
        std::cerr << " -c              : Parameter c." << std::endl;
        std::cerr << " -P, --pause     : Pause at start. Will be released by 'p' key."
                  << std::endl;
        exit(1);
      }
    }
    initGL(argc, argv);
    elements_size = std::vector<cl_int>({
        field_width, field_height});  // cell slots
    local_work_size = std::vector<cl_int>({
        256, 256});

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    bool device_found = false;
    size_t dev_index = 0;
    for (cl::Platform& plat : platforms) {
      for (cl_device_type device_type
             : {CL_DEVICE_TYPE_CPU, CL_DEVICE_TYPE_GPU}) {
        std::vector<cl::Device> devices;
        plat.getDevices(device_type, &devices);
        for (cl::Device& dev : devices) {
          const std::string extensions = dev.getInfo<CL_DEVICE_EXTENSIONS>();
          if (!with_cl_gl_sharing(extensions)) {
            continue;
          }
          const std::string devvendor = dev.getInfo<CL_DEVICE_VENDOR>();
          const std::string devname = dev.getInfo<CL_DEVICE_NAME>();
          const std::string devver = dev.getInfo<CL_DEVICE_VERSION>();
          std::cout <<
            ((dev_index == device_index) ? '*' : ' ') <<
            "device[" << dev_index << "]:"
            " vendor[" << devvendor << "]"
            ",name[" << devname << "]"
            ",version[" << devver << "]" << std::endl;
          size_t max_work_group_size;
          dev.getInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE,
                      &max_work_group_size);
          std::cout << "        MAX_WORK_GROUP_SIZE="
                    << max_work_group_size << std::endl;
          if (dev_index == device_index) {
            platform = plat;
            device = dev;
            device_found = true;
            while (static_cast<size_t>(
                local_work_size[0] *
                local_work_size[1]) > max_work_group_size) {
              local_work_size[0] /= 2;
              if (static_cast<size_t>(
                  local_work_size[0] *
                  local_work_size[1]) > max_work_group_size) {
                local_work_size[1] /= 2;
              }
            }
          }
          ++dev_index;
        }
      }
    }
    if (!device_found) {
      std::cerr << "device[" << device_index << "] not found" << std::endl;
      exit(1);
    }
    const cl_platform_id platform_id = device.getInfo<CL_DEVICE_PLATFORM>()();
    cl_context_properties properties[7];
    properties[0] = CL_GL_CONTEXT_KHR;
    properties[1] =
      reinterpret_cast<cl_context_properties>(glXGetCurrentContext());
    properties[2] = CL_GLX_DISPLAY_KHR;
    properties[3] =
      reinterpret_cast<cl_context_properties>(glXGetCurrentDisplay());
    properties[4] = CL_CONTEXT_PLATFORM;
    properties[5] = reinterpret_cast<cl_context_properties>(platform_id);
    properties[6] = 0;

    clGetGLContextInfoKHR_fn myGetGLContextInfoKHR =
      reinterpret_cast<clGetGLContextInfoKHR_fn>(
          clGetExtensionFunctionAddressForPlatform(
              platform_id, "clGetGLContextInfoKHR"));

    size_t size;
    myGetGLContextInfoKHR(properties, CL_DEVICES_FOR_GL_CONTEXT_KHR,
                          sizeof(cl_device_id), &device, &size);

    context = cl::Context(device, properties);
    command_queue = cl::CommandQueue(context, device, 0);

    global_work_size = std::vector<cl_int>({
        static_cast<cl_int>(ceil(
            static_cast<double>(elements_size[0])
            / local_work_size[0]) * local_work_size[0]),
        static_cast<cl_int>(ceil(
            static_cast<double>(elements_size[1])
            / local_work_size[1]) * local_work_size[1])});
    std::cout << "global_work_size[0]=" << global_work_size[0]
              << ", local_work_size[0]=" << local_work_size[0]
              << ", elements_size[0]=" << elements_size[0]
              << ", work_groups_x="
              << (global_work_size[0] / local_work_size[0])
              << std::endl;
    std::cout << "global_work_size[1]=" << global_work_size[1]
              << ", local_work_size[1]=" << local_work_size[1]
              << ", elements_size[1]=" << elements_size[1]
              << ", work_groups_y="
              << (global_work_size[1] / local_work_size[1])
              << std::endl;
    /* allocate host memory */
    field_image.resize(global_work_size[0] * global_work_size[1] * 4);
    /* end allocate host memory */

    /* create buffers */
    dev_image = cl::ImageGL(
        context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, rendered_texture);
    dev_field_a = cl::Buffer(
        context, CL_MEM_READ_WRITE,
        sizeof(cl_float) * global_work_size[0] * global_work_size[1]);
    dev_field_b = cl::Buffer(
        context, CL_MEM_READ_WRITE,
        sizeof(cl_float) * global_work_size[0] * global_work_size[1]);
    dev_field_c = cl::Buffer(
        context, CL_MEM_READ_WRITE,
        sizeof(cl_float) * global_work_size[0] * global_work_size[1]);

    dev_field_a2 = cl::Buffer(
        context, CL_MEM_READ_WRITE,
        sizeof(cl_float) * global_work_size[0] * global_work_size[1]);
    dev_field_b2 = cl::Buffer(
        context, CL_MEM_READ_WRITE,
        sizeof(cl_float) * global_work_size[0] * global_work_size[1]);
    dev_field_c2 = cl::Buffer(
        context, CL_MEM_READ_WRITE,
        sizeof(cl_float) * global_work_size[0] * global_work_size[1]);
    /* end create buffers */

    const std::string source_string = loadProgramSource(kernel_source);

    program = cl::Program(context, source_string);
    try {
      program.build();
    } catch (const cl::Error& err) {
      std::cout << "Build Status: "
                << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(
                    context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
      std::cout << "Build Options: "
                << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(
                    context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
      std::cout << "Build Log: "
                << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(
                    context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
      throw err;
    }
    try {
      bz_kernel_diffusion_a = cl::Kernel(program, "bz_diffusion");
      bz_kernel_diffusion_a.setArg(0, dev_field_a);
      bz_kernel_diffusion_a.setArg(1, dev_field_a2);
      bz_kernel_diffusion_a.setArg(2, sizeof(cl_float), &diffusion_rate);
    } catch (...) {
      std::cerr << "bz_diffusion_a" << std::endl;
      throw;
    }
    try {
      bz_kernel_diffusion_b = cl::Kernel(program, "bz_diffusion");
      bz_kernel_diffusion_b.setArg(0, dev_field_b);
      bz_kernel_diffusion_b.setArg(1, dev_field_b2);
      bz_kernel_diffusion_b.setArg(2, sizeof(cl_float), &diffusion_rate);
    } catch (...) {
      std::cerr << "bz_diffusion_b" << std::endl;
      throw;
    }
    try {
      bz_kernel_diffusion_c = cl::Kernel(program, "bz_diffusion");
      bz_kernel_diffusion_c.setArg(0, dev_field_c);
      bz_kernel_diffusion_c.setArg(1, dev_field_c2);
      bz_kernel_diffusion_c.setArg(2, sizeof(cl_float), &diffusion_rate);
    } catch (...) {
      std::cerr << "bz_diffusion_c" << std::endl;
      throw;
    }

    try {
      bz_kernel_reaction = cl::Kernel(program, "bz_reaction");
      bz_kernel_reaction.setArg(0, dev_field_a2);
      bz_kernel_reaction.setArg(1, dev_field_b2);
      bz_kernel_reaction.setArg(2, dev_field_c2);
      bz_kernel_reaction.setArg(3, dev_field_a);
      bz_kernel_reaction.setArg(4, dev_field_b);
      bz_kernel_reaction.setArg(5, dev_field_c);
      bz_kernel_reaction.setArg(6, sizeof(cl_float), &param_a);
      bz_kernel_reaction.setArg(7, sizeof(cl_float), &param_b);
      bz_kernel_reaction.setArg(8, sizeof(cl_float), &param_c);
    } catch (...) {
      std::cerr << "bz_reaction" << std::endl;
      throw;
    }

    try {
      bz_kernel_draw_image = cl::Kernel(program, "bz_draw_image");
      bz_kernel_draw_image.setArg(0, dev_field_a);
      bz_kernel_draw_image.setArg(1, dev_field_b);
      bz_kernel_draw_image.setArg(2, dev_field_c);
      bz_kernel_draw_image.setArg(3, dev_image);
    } catch (...) {
      std::cerr << "bz_draw_image" << std::endl;
      throw;
    }
    /* init field_init */
    unsigned seed = time(0);
    srand(seed);
    std::vector<cl_float> field_init;
    field_init.resize(global_work_size[0] * global_work_size[1]);
    bz_init_random_field(field_init, seed);
    command_queue.enqueueWriteBuffer(
        dev_field_a, CL_TRUE, 0,
        sizeof(cl_float) * global_work_size[0] * global_work_size[1],
        &field_init.front());
    bz_init_random_field(field_init, seed);
    command_queue.enqueueWriteBuffer(
        dev_field_b, CL_TRUE, 0,
        sizeof(cl_float) * global_work_size[0] * global_work_size[1],
        &field_init.front());
    bz_init_random_field(field_init, seed);
    command_queue.enqueueWriteBuffer(
        dev_field_c, CL_TRUE, 0,
        sizeof(cl_float) * global_work_size[0] * global_work_size[1],
        &field_init.front());

    startGL();
  } catch (const cl::Error& err) {
    report_cl_error(err);
  }
  return 0;
}
