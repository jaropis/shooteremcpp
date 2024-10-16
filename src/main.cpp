#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>

void render_frame() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
int main() {
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha = false;
    attrs.depth = false;
    attrs.stencil = false;
    attrs.antialias = true;
    attrs.majorVersion = 2;

    context = emscripten_webgl_create_context("#canvas", &attrs);
    emscripten_webgl_make_context_current(context);

    emscripten_set_main_loop(render_frame, 0, 1);
    return 0;
}