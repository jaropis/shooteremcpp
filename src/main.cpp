#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <string.h> // For strcmp
#include <vector>    // For managing bullets

// Bullet structure
struct Bullet {
    float x, y;       // Position
    float speed;      // Speed (pixels per second)
};

// Vector to store active bullets
std::vector<Bullet> bullets;

// Declare the WebGL context handle globally
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;

// Vertex Shader Source
const char* vertex_shader_src = R"(
    attribute vec2 a_position;
    uniform vec2 u_resolution;
    void main() {
        // Convert from pixels to clipspace
        vec2 zeroToOne = a_position / u_resolution;
        vec2 zeroToTwo = zeroToOne * 2.0;
        vec2 clipSpace = zeroToTwo - 1.0;
        gl_Position = vec4(clipSpace * vec2(1, -1), 0, 1);
    }
)";

// Fragment Shader Source
const char* fragment_shader_src = R"(
    precision mediump float;
    uniform vec4 u_color;
    void main() {
        gl_FragColor = u_color;
    }
)";

// Function to check and print shader compilation/linking errors
void check_compile_errors(GLuint shader, const char* type) {
    GLint success;
    GLchar infoLog[1024];
    if (strcmp(type, "PROGRAM") != 0) { // strcmp is declared via <string.h>
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            printf("ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s\n", type, infoLog);
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            printf("ERROR::PROGRAM_LINKING_ERROR of type: %s\n%s\n", type, infoLog);
        }
    }
}

// Function to compile a shader
GLuint compile_shader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    if (type == GL_VERTEX_SHADER) {
        check_compile_errors(shader, "VERTEX");
    } else if (type == GL_FRAGMENT_SHADER) {
        check_compile_errors(shader, "FRAGMENT");
    }
    return shader;
}

// Function to create and link the shader program
GLuint create_program() {
    // Compile the vertex and fragment shaders
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
    
    // Create a shader program object
    GLuint program = glCreateProgram();
    
    // Attach the compiled shaders to the program
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    
    // Link the shader program (only one argument: the program)
    glLinkProgram(program);
    
    // Check for linking errors
    check_compile_errors(program, "PROGRAM");
    
    // Shaders can be deleted after linking
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return program;
}

// Global variables for shader program and attribute/uniform locations
GLuint program;
GLint a_position_loc;
GLint u_resolution_loc;
GLint u_color_loc;

// Vertex Buffer Object
GLuint vao;

// Player properties
float player_x = 400.0f;
float player_y = 300.0f;
float player_speed = 200.0f; // pixels per second

// Input state
bool left = false, right = false, up = false, down = false;

// Function prototypes
EM_BOOL key_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData);
void render_frame();

// Function to handle key events
EM_BOOL key_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData) {
    if (eventType == EMSCRIPTEN_EVENT_KEYDOWN) {
        if (strcmp(e->key, "ArrowLeft") == 0) left = true;
        if (strcmp(e->key, "ArrowRight") == 0) right = true;
        if (strcmp(e->key, "ArrowUp") == 0) up = true;
        if (strcmp(e->key, "ArrowDown") == 0) down = true;
        if (strcmp(e->key, " ") == 0) { // Spacebar pressed
            // Create a new bullet at the top-center of the player
            Bullet new_bullet;
            new_bullet.x = player_x + (50.0f / 2.0f) - (5.0f / 2.0f); // Centered horizontally, assuming bullet size is 5
            new_bullet.y = player_y;
            new_bullet.speed = 400.0f; // Bullet speed
            bullets.push_back(new_bullet);
        }
    } else if (eventType == EMSCRIPTEN_EVENT_KEYUP) {
        if (strcmp(e->key, "ArrowLeft") == 0) left = false;
        if (strcmp(e->key, "ArrowRight") == 0) right = false;
        if (strcmp(e->key, "ArrowUp") == 0) up = false;
        if (strcmp(e->key, "ArrowDown") == 0) down = false;
    }
    return EM_TRUE;
}

// Function to render each frame
void render_frame() {
    // Declare last_time as a static variable to preserve its value between calls
    static double last_time = 0.0;
    
    // Get current time in seconds
    double current_time = emscripten_get_now() / 1000.0;
    
    // Calculate delta_time
    double delta_time = current_time - last_time;
    last_time = current_time;
    
    // Update player position based on input
    if (left) player_x -= player_speed * delta_time;
    if (right) player_x += player_speed * delta_time;
    if (up) player_y -= player_speed * delta_time;
    if (down) player_y += player_speed * delta_time;
    
    // Clamp player position to canvas boundaries
    if (player_x < 0) player_x = 0;
    if (player_x > 800 - 50) player_x = 800 - 50; // Assuming player size is 50
    if (player_y < 0) player_y = 0;
    if (player_y > 600 - 50) player_y = 600 - 50; // Assuming player size is 50
    
    // Update bullets
    for (size_t i = 0; i < bullets.size(); ) {
        // Move the bullet upwards
        bullets[i].y -= bullets[i].speed * delta_time;
        
        // Remove the bullet if it goes off the top of the screen
        if (bullets[i].y < -5.0f) { // Adding a small buffer
            bullets.erase(bullets.begin() + i);
        } else {
            ++i;
        }
    }
    
    // Clear the screen with black color
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Use the compiled shader program
    glUseProgram(program);
    
    // Define a rectangle (player) based on updated position
    float size = 50.0f;
    float player_vertices[] = {
        player_x, player_y,
        player_x + size, player_y,
        player_x + size, player_y + size,
        player_x, player_y + size
    };
    
    // Bind the VAO for the player
    glBindBuffer(GL_ARRAY_BUFFER, vao);
    // Upload the vertex data to the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(player_vertices), player_vertices, GL_DYNAMIC_DRAW);
    
    // Enable the vertex attribute array and specify the layout
    glEnableVertexAttribArray(a_position_loc);
    glVertexAttribPointer(a_position_loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    // Set the resolution uniform
    glUniform2f(u_resolution_loc, 800, 600);
    // Set the color uniform to green for the player
    glUniform4f(u_color_loc, 0.0, 1.0, 0.0, 1.0); // RGBA
    
    // Draw the player rectangle as a triangle fan
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Render bullets
    // Change color to yellow for bullets
    glUniform4f(u_color_loc, 1.0, 1.0, 0.0, 1.0); // RGBA
    
    for (const auto& bullet : bullets) {
        float bullet_size = 5.0f;
        float bullet_vertices[] = {
            bullet.x, bullet.y,
            bullet.x + bullet_size, bullet.y,
            bullet.x + bullet_size, bullet.y + bullet_size,
            bullet.x, bullet.y + bullet_size
        };
        
        // Update the buffer with bullet vertices
        glBufferData(GL_ARRAY_BUFFER, sizeof(bullet_vertices), bullet_vertices, GL_DYNAMIC_DRAW);
        
        // Draw the bullet rectangle as a triangle fan
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
}

int main() {
    // Initialize WebGL context attributes
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha = false;
    attrs.depth = false;
    attrs.stencil = false;
    attrs.antialias = true;
    attrs.majorVersion = 2;
    
    // Create and make the WebGL context current
    context = emscripten_webgl_create_context("#canvas", &attrs);
    if (context <= 0) {
        printf("Failed to create WebGL context.\n");
        return -1;
    }
    emscripten_webgl_make_context_current(context);
    
    // Create and link the shader program
    program = create_program();
    
    // Get the locations of attributes and uniforms
    a_position_loc = glGetAttribLocation(program, "a_position");
    u_resolution_loc = glGetUniformLocation(program, "u_resolution");
    u_color_loc = glGetUniformLocation(program, "u_color");
    
    // Create and bind the Vertex Buffer Object (VBO)
    glGenBuffers(1, &vao);
    glBindBuffer(GL_ARRAY_BUFFER, vao);
    // Initialize the buffer with no data; it will be updated each frame
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, NULL, GL_DYNAMIC_DRAW);
    
    // Set up input callbacks
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, key_callback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, key_callback);
    
    // Set the main loop to render frames
    emscripten_set_main_loop(render_frame, 0, 1);
    
    return 0;
}
