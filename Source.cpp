#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#include <chrono>
#include <windows.h>
#include <vector>
#include "camera.h"

// image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> 




// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Joshua Morris: CS330 Project"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    enum Material {matte, satin, gloss, glow};

    // Structure used to store mesh data
    struct GLMesh
    {
        //vertex array
        GLuint vao;
        //vertex buffer
        GLuint vbo;
        //vertex buffer
        GLuint vbos[2];
        //indices of the mesh
        GLuint nIndices;

        //indices to draw
        std::vector<float> v;
        //translation properties of the shape
        std::vector<float> p;

        //physical properties of the shape
        float height;
        float length;
        float radius;
        float innerRadius;
        float number_of_sides;

        // each shape gets a matrix object
        glm::mat4 scale;
        glm::mat4 xrotation;
        glm::mat4 yrotation;
        glm::mat4 zrotation;
        glm::mat4 rotation;
        glm::mat4 translation;
        glm::mat4 model;
        glm::vec2 gUVScale;

        // texture information
        const char* texFilename;
        GLuint textureId;

        GLuint lightSourceId;


        float transparency = 1.0f;

        Material material = matte;

    

        //texture wrapping mode: 
        GLint gTextWrapMode = GL_REPEAT;
        //GLint gTextWrapMode = GL_MIRRORED_REPEAT;
        //GLint gTextWrapMode = GL_CLAMP_TO_EDGE;
        //GLint gTextWrapMode = GL_CLAMP_TO_BORDER;



        class Mesh
        {
        };
    };


    //Mesh used to render the light cubes
    struct GLightMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbo;         // Handle for the vertex buffer object
        GLuint nVertices;    // Number of indices of the mesh
    };

    //Meshes for the lights
    GLightMesh spotLightMesh;
    GLightMesh keyLightMesh;

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;

    //Mesh vector, holds the shapes that make up the scene
    vector<GLMesh> scene;

    // Shader programs
    GLuint gProgramIdMatte;
    GLuint gProgramIdSatin;
    GLuint gProgramIdGloss;
    GLuint gProgramIdGlow;
    GLuint gLightProgramId;

    GLuint gUseProgramId;

 

    //Stores the scene's current rotation
    float currRotation = 0;

    std::vector<bool> lightSources;


    // camera
    // set the initial position of the camera
    Camera gCamera(glm::vec3(-5.0f, 2.5f, -3.0f), glm::vec3(0.0f, 1.0f, 0.0f), 35.0f, -20.0f);

    float gLastX = WINDOW_WIDTH / 2.0f;

    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    bool isPerspective = true;

    bool keyDown = false;




    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Light color, position and scale
    glm::vec3 gSpotLightColor(0.7f, 0.7f, 0.6f);
    glm::vec3 gSpotLightPosition(1.5f, 2.0f, -1.5f);
    glm::vec3 gSpotLightScale(0.1f);

    // Light color, position and scale
    glm::vec3 gKeyLightColor(0.9f, 0.2f, 0.0f);
    glm::vec3 gKeyLightPosition(0.5f, 1.1f, 2.5f);
    glm::vec3 gKeyLightScale(0.1f);

    glm::vec3 gAmbientLightColor(0.1f, 0.1f, 0.1f);

    bool gSpotLightOrbit = false;
    bool gSpotLightOn = true;
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UBuildScene(vector<GLMesh>& scene);
void UTranslator(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
void URenderScene(vector<GLMesh> world);
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
void UBuildCone(GLMesh& mesh);
void UBuildPlane(GLMesh& mesh);
void UBuildCube(GLMesh& mesh);
void UBuildHollowCylinder(GLMesh& mesh);
void UBuildCylinder(GLMesh& mesh);
void UBuildCircle(GLMesh& mesh);
void flipImageVertically(unsigned char* image, int width, int height, int channels);
void UCreateLightMesh(GLightMesh& lightMesh);

// texture create
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);

//---------------------------------------------------------------------------- SHADERS -----------------------------------------------------------------------------------------------------


/*
Each shader is programmed with different parameters to 
produce different lighting effects.
Matte = low specular 
Satin = mid specular
Gloss = high specular
Glow = ignores lighting effects to produce a glowing effect
*/


//------------------------------------------------------------------- MATTE ---------------------------------------------------------------------------------------------
/* Vertex Shader Source Code (Matte)*/
const GLchar* vertexShaderSourceMatte = GLSL(440,
layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate; //outgoing texture coordinate

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f);
    vertexFragmentPos = vec3(model * vec4(position, 1.0f));
    vertexNormal = mat3(transpose(inverse(model))) * normal;
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Fragment Shader Source Code (Matte)*/
const GLchar* fragmentShaderSourceMatte = GLSL(440,
in vec3 vertexFragmentPos;
in vec3 vertexNormal;
in vec2 vertexTextureCoordinate; // for texture coordinates, not color

out vec4 fragmentColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 keyLightColor;
uniform vec3 ambientLightColor;
uniform vec3 lightPos;
uniform vec3 keyLightPos;
uniform vec3 viewPosition;

uniform float transparency;

uniform sampler2D uTexture;
uniform vec2 uvScale;

void main()
{
    //Ambient lighting received through uniform
    vec3 ambient = ambientLightColor;

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    vec3 keyLightDirection = normalize(keyLightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube

    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    float keyImpact = max(dot(norm, keyLightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light

    vec3 diffuse = impact * lightColor; // Generate diffuse light color
    vec3 keyDiffuse = keyImpact * keyLightColor;

    //Calculate Specular lighting*/
    float specularIntensity = 0.2f; // Set specular light strength
    float highlightSize = 15.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    vec3 keyReflectDir = reflect(-keyLightDirection, norm);// Calculate key reflection vector

    //Calculate specular component for both lights
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    float keySpecularComponent = pow(max(dot(viewDir, keyReflectDir), 0.0), highlightSize);

    vec3 specular = specularIntensity * specularComponent * lightColor;
    vec3 keySpecular = specularIntensity * keySpecularComponent * keyLightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + keyDiffuse + specular + keySpecular) * textureColor.xyz;

    // This algorithm allows transparent shapes to display specular highlights by rendering the part of the shape
    //affected by the highlight as less transparent
    float netTransparency = min(transparency + specular.x + specular.y + specular.z + keySpecular.x + keySpecular.y + keySpecular.z, 1.0f);


    fragmentColor = vec4(phong, netTransparency); // Send lighting results to GPU

}
);

//------------------------------------------------------------------- SATIN ---------------------------------------------------------------------------------------------
/* Vertex Shader Source Code (Satin)*/
const GLchar* vertexShaderSourceSatin = GLSL(440,
layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate; //outgoing texture coordinate

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f);
    vertexFragmentPos = vec3(model * vec4(position, 1.0f));
    vertexNormal = mat3(transpose(inverse(model))) * normal;
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Fragment Shader Source Code (Satin)*/
const GLchar* fragmentShaderSourceSatin = GLSL(440,
    in vec3 vertexFragmentPos;
in vec3 vertexNormal;
in vec2 vertexTextureCoordinate; // for texture coordinates, not color

out vec4 fragmentColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 keyLightColor;
uniform vec3 ambientLightColor;
uniform vec3 lightPos;
uniform vec3 keyLightPos;
uniform vec3 viewPosition;

uniform float transparency;

uniform sampler2D uTexture;
uniform vec2 uvScale;

void main()
{
    //Ambient lighting received through uniform
    vec3 ambient = ambientLightColor;

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    vec3 keyLightDirection = normalize(keyLightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube

    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    float keyImpact = max(dot(norm, keyLightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light

    vec3 diffuse = impact * lightColor; // Generate diffuse light color
    vec3 keyDiffuse = keyImpact * keyLightColor;

    //Calculate Specular lighting*/
    float specularIntensity = 0.5f; // Set specular light strength
    float highlightSize = 25.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    vec3 keyReflectDir = reflect(-keyLightDirection, norm);// Calculate key reflection vector

    //Calculate specular component for both lights
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    float keySpecularComponent = pow(max(dot(viewDir, keyReflectDir), 0.0), highlightSize);

    vec3 specular = specularIntensity * specularComponent * lightColor;
    vec3 keySpecular = specularIntensity * keySpecularComponent * keyLightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + keyDiffuse + specular + keySpecular) * textureColor.xyz;

    // This algorithm allows transparent shapes to display specular highlights by rendering the part of the shape
    //affected by the highlight as less transparent
    float netTransparency = min(transparency + specular.x + specular.y + specular.z + keySpecular.x + keySpecular.y + keySpecular.z, 1.0f);


    fragmentColor = vec4(phong, netTransparency); // Send lighting results to GPU

}
);

//------------------------------------------------------------------- Gloss ---------------------------------------------------------------------------------------------
/* Vertex Shader Source Code (Gloss)*/
const GLchar* vertexShaderSourceGloss = GLSL(440,
layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate; //outgoing texture coordinate

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f);
    vertexFragmentPos = vec3(model * vec4(position, 1.0f));
    vertexNormal = mat3(transpose(inverse(model))) * normal;
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Fragment Shader Source Code (Gloss)*/
const GLchar* fragmentShaderSourceGloss = GLSL(440,
    in vec3 vertexFragmentPos;
in vec3 vertexNormal;
in vec2 vertexTextureCoordinate; // for texture coordinates, not color

out vec4 fragmentColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 keyLightColor;
uniform vec3 ambientLightColor;
uniform vec3 lightPos;
uniform vec3 keyLightPos;
uniform vec3 viewPosition;

uniform float transparency;

uniform sampler2D uTexture;
uniform vec2 uvScale;

void main()
{
    //Ambient lighting received through uniform
    vec3 ambient = ambientLightColor;

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    vec3 keyLightDirection = normalize(keyLightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube

    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    float keyImpact = max(dot(norm, keyLightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light

    vec3 diffuse = impact * lightColor; // Generate diffuse light color
    vec3 keyDiffuse = keyImpact * keyLightColor;

    //Calculate Specular lighting*/
    float specularIntensity = 3.5f; // Set specular light strength
    float highlightSize = 55.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    vec3 keyReflectDir = reflect(-keyLightDirection, norm);// Calculate key reflection vector

    //Calculate specular component for both lights
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    float keySpecularComponent = pow(max(dot(viewDir, keyReflectDir), 0.0), highlightSize);

    vec3 specular = specularIntensity * specularComponent * lightColor;
    vec3 keySpecular = specularIntensity * keySpecularComponent * keyLightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + keyDiffuse + specular + keySpecular) * textureColor.xyz;

    // This algorithm allows transparent shapes to display specular highlights by rendering the part of the shape
    //affected by the highlight as less transparent
    float netTransparency = min(transparency + specular.x + specular.y + specular.z + keySpecular.x + keySpecular.y + keySpecular.z, 1.0f);


    fragmentColor = vec4(phong, netTransparency); // Send lighting results to GPU

}
);

//------------------------------------------------------------------- Glow ---------------------------------------------------------------------------------------------
/* Vertex Shader Source Code (Glow)*/
const GLchar* vertexShaderSourceGlow = GLSL(440,
    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate; //outgoing texture coordinate

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f);
    vertexFragmentPos = vec3(model * vec4(position, 1.0f));
    vertexNormal = mat3(transpose(inverse(model))) * normal;
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Fragment Shader Source Code (Glow)*/
const GLchar* fragmentShaderSourceGlow = GLSL(440,
    in vec3 vertexFragmentPos;
in vec3 vertexNormal;
in vec2 vertexTextureCoordinate; // for texture coordinates, not color

out vec4 fragmentColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 keyLightColor;
uniform vec3 ambientLightColor;
uniform vec3 lightPos;
uniform vec3 keyLightPos;
uniform vec3 viewPosition;

uniform float transparency;

uniform sampler2D uTexture;
uniform vec2 uvScale;

void main()
{
    //Ambient/diffuse light is not calculated for glowing objects
    //Specular is still calculated to allow other light sources to reflect off of the glowing object
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    //Calculate Specular lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    vec3 keyLightDirection = normalize(keyLightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float specularIntensity = 3.5f; // Set specular light strength
    float highlightSize = 55.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    vec3 keyReflectDir = reflect(-keyLightDirection, norm);// Calculate key reflection vector

     //Calculate specular component for both lights
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    float keySpecularComponent = pow(max(dot(viewDir, keyReflectDir), 0.0), highlightSize);

    vec3 specular = specularIntensity * specularComponent * lightColor;
    vec3 keySpecular = specularIntensity * keySpecularComponent * keyLightColor;

    // This algorithm allows transparent shapes to display specular highlights by rendering the part of the shape
    //affected by the highlight as less transparent
    float netTransparency = min(transparency + specular.x + specular.y + specular.z + keySpecular.x + keySpecular.y + keySpecular.z, 1.0f);


    // Lighting calculation is additive for glowing objects
    vec3 phong = specular + keySpecular + textureColor.xyz;


    fragmentColor = vec4(phong, netTransparency); // Send lighting results to GPU

}
);




// Light Shader Source Code
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);

// Light Fragment Shader Source Code
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing light color to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);








int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UBuildScene(scene);

    // Create Light Object
    UCreateLightMesh(spotLightMesh);
    UCreateLightMesh(keyLightMesh);

    // Create the shader programs
    if (!UCreateShaderProgram(vertexShaderSourceMatte, fragmentShaderSourceMatte, gProgramIdMatte))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(vertexShaderSourceSatin, fragmentShaderSourceSatin, gProgramIdSatin))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(vertexShaderSourceGloss, fragmentShaderSourceGloss, gProgramIdGloss))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(vertexShaderSourceGlow, fragmentShaderSourceGlow, gProgramIdGlow))
        return EXIT_FAILURE;   

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLightProgramId))
        return EXIT_FAILURE;


    for (auto& m : scene)
    {
        if (!UCreateTexture(m.texFilename, m.textureId))
        {
            cout << "Failed to load texture " << m.texFilename << endl;
            //cin.get();
            return EXIT_FAILURE;

        }

    }

    // Background window color set to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {


        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URenderScene(scene);



        glfwPollEvents();

        //Add time delay before starting next loop
        Sleep(40);
    }

    for (auto& m : scene)
    {
        UDestroyMesh(m);
    }

    scene.clear();


    // Release shader program
    UDestroyShaderProgram(gProgramIdMatte);
    UDestroyShaderProgram(gProgramIdSatin);
    UDestroyShaderProgram(gProgramIdGloss);
    UDestroyShaderProgram(gProgramIdGlow);
    UDestroyShaderProgram(gLightProgramId);


    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);

    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    //glDisable(GL_CULL_FACE);

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // draw lines
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // fill shapes
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);


    //Process input from toggle keys. keyDown variable used to prevent
    //rapid swapping if the key is held
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        if (keyDown == false) {
            keyDown = true;
            isPerspective = !isPerspective;
        }
    }
    else if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        if (keyDown == false) {
            keyDown = true;
            lightSources[0] = !lightSources[0];
            if (lightSources[0]) {
                gKeyLightColor = glm::vec3(0.9f, 0.2f, 0.0f);
            }
            else {
                gKeyLightColor = glm::vec3(0.0f);
            }
        }
    }
    else if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        if (keyDown == false) {
            keyDown = true;
            gSpotLightOn = !gSpotLightOn;
            if (gSpotLightOn) {
                gSpotLightColor = glm::vec3(0.7f, 0.7f, 0.6f);;
            }
            else {
                gSpotLightColor = glm::vec3(0.0f);
                gSpotLightOrbit = false;
            }
        }
    }
    else if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        if (keyDown == false) {
            keyDown = true;
            gSpotLightOrbit = !gSpotLightOrbit;
        }      

    }
    else {
        keyDown = false;
    }

   


}

void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // change camera speed by mouse scroll
    gCamera.ProcessMouseScroll(yoffset);
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void URenderScene(vector<GLMesh> world)
{

    // Borrowed from the Tutorial; animates the Spot Light to circle around the scene
    constexpr float angularVelocity = glm::radians(45.0f);
    if (gSpotLightOrbit)
    {
        glm::vec4 newPosition = glm::rotate(angularVelocity * gDeltaTime, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(gSpotLightPosition, 1.0f);
        gSpotLightPosition.x = newPosition.x;
        gSpotLightPosition.y = newPosition.y;
        gSpotLightPosition.z = newPosition.z;
    }


    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // transform the camera (x, y, z)
    glm::mat4 view = gCamera.GetViewMatrix();


    //Create a projection depending on whether we are set to perspective or orthographic
    glm::mat4 projection;
    if (isPerspective)
    {
        // p for perspective (default)
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else
        // o for ortho
        projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);


    




    // loop to draw each shape individually
    for (auto i = 0; i < world.size(); ++i)
    {
        auto mesh = world[i];

        // activate vbo's within mesh's vao
        glBindVertexArray(mesh.vao);

        switch (mesh.material)
        {
        case matte:
            gUseProgramId = gProgramIdMatte;
            break;
        case satin:
            gUseProgramId = gProgramIdSatin;
            break;
        case gloss:
            gUseProgramId = gProgramIdGloss;
            break;
        case glow:
            if (lightSources[mesh.lightSourceId]) {
                gUseProgramId = gProgramIdGlow;
            }
            else {
                gUseProgramId = gProgramIdGloss;
            }

            break;

        default:
            gUseProgramId = gProgramIdMatte;
        }

        // set default shader
        glUseProgram(gUseProgramId);

        // Initializes location variables
        GLint modelLocation = glGetUniformLocation(gUseProgramId, "model");
        GLint viewLocation = glGetUniformLocation(gUseProgramId, "view");
        GLint projLocation = glGetUniformLocation(gUseProgramId, "projection");
        GLint UVScaleLoc = glGetUniformLocation(gUseProgramId, "uvScale");


        // Spotlight
        GLint lightColorLoc = glGetUniformLocation(gUseProgramId, "lightColor");
        GLint lightPositionLoc = glGetUniformLocation(gUseProgramId, "lightPos");

        // Key light
        GLint keyLightColorLoc = glGetUniformLocation(gUseProgramId, "keyLightColor");
        GLint keyLightPositionLoc = glGetUniformLocation(gUseProgramId, "keyLightPos");

        //Ambient light
        GLint ambientLightLoc = glGetUniformLocation(gUseProgramId, "ambientLightColor");

        // Camera view
        GLint viewPositionLoc = glGetUniformLocation(gUseProgramId, "viewPosition");

        // Reference matrix uniforms from the shape shader program for the shape color, light color, light position, and camera position
        GLint objectColorLoc = glGetUniformLocation(gUseProgramId, "objectColor");

        GLint transparencyLoc = glGetUniformLocation(gUseProgramId, "transparency");

       

        // Spot Light
        glUniform3f(lightColorLoc, gSpotLightColor.r, gSpotLightColor.g, gSpotLightColor.b);
        glUniform3f(lightPositionLoc, gSpotLightPosition.x, gSpotLightPosition.y, gSpotLightPosition.z);

        // Key Light
        glUniform3f(keyLightColorLoc, gKeyLightColor.r, gKeyLightColor.g, gKeyLightColor.b);
        glUniform3f(keyLightPositionLoc, gKeyLightPosition.x, gKeyLightPosition.y, gKeyLightPosition.z);

        // Ambient Light
        glUniform3f(ambientLightLoc, gAmbientLightColor.r, gAmbientLightColor.g, gAmbientLightColor.b);


        const glm::vec3 cameraPosition = gCamera.Position;
        glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);





        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(mesh.model));
        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLocation, 1, GL_FALSE, glm::value_ptr(projection));

        glUniform1f(transparencyLoc, mesh.transparency);



        glUniform2fv(UVScaleLoc, 1, glm::value_ptr(mesh.gUVScale));

        // Pass color, light, and camera data to the shape shader 
        glUniform3f(objectColorLoc, mesh.p[0], mesh.p[1], mesh.p[2]);



        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.textureId);



        // Draws the triangles
        glDrawArrays(GL_TRIANGLES, 0, mesh.nIndices);

    }

    // Vars for lights
    glm::mat4 model;
    GLint modelLoc;
    GLint viewLoc;
    GLint projLoc;

    // --------------------
    // Draw the Spot Light
    if (gSpotLightOn) {
        glUseProgram(gLightProgramId);
        glBindVertexArray(spotLightMesh.vao);

        // Light location and Scale
        model = glm::translate(gSpotLightPosition) * glm::scale(gSpotLightScale);

        // Matrix uniforms from the Light Shader program
        modelLoc = glGetUniformLocation(gLightProgramId, "model");
        viewLoc = glGetUniformLocation(gLightProgramId, "view");
        projLoc = glGetUniformLocation(gLightProgramId, "projection");

        // Matrix data
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw the light
        glDrawArrays(GL_TRIANGLES, 0, spotLightMesh.nVertices);
        // --------------------
    }
    


   


    // deactivate vao
    glBindVertexArray(0);
    glUseProgram(0);

    // swap front and back buffers
    glfwSwapBuffers(gWindow);

}



void UBuildScene(vector<GLMesh>& scene)
{



    //---------------------------------------- LAVA LAMP -----------------------------------------------------------------------

    // CONE - Top Cap of Lava Lamp
    GLMesh con_mesh_00;
    con_mesh_00.p = {
        1.0f, 1.0f, 1.0f, 1.0f,				// color r, g, b a
        0.2f, 0.2f, 0.2f,					// scale x, y, z
        0.0f, 1.0f, 0.0f, 0.0f,				// x amount of rotation, rotate x, y, z
        0.0f, 0.0f, 1.0f, 0.0f,				// y amount of rotation, rotate x, y, z
        0.0f, 0.0f, 0.0f, 1.0f,				// z amount of rotation, rotate x, y, z
        0.4f, 1.47f, 2.4f,					// translate x, y, z
        1.0f, 1.0f
    };
    con_mesh_00.height = 1.8f;
    con_mesh_00.radius = 0.5f;
    con_mesh_00.length = 0.5f;
    con_mesh_00.number_of_sides = 144.0f;
    con_mesh_00.texFilename = "textures/blacksparkle.png";
    con_mesh_00.material = gloss;

    UBuildCone(con_mesh_00);
    scene.push_back(con_mesh_00);



    // CONE - Top Glass of Lava Lamp
    GLMesh con_mesh_01;
    con_mesh_01.p = {
        1.0f, 1.0f, 1.0f, 1.0f,				// color r, g, b a
        1.0f, 1.0f, 1.0f,					// scale x, y, z
        0.0f, 1.0f, 0.0f, 0.0f,				// x amount of rotation, rotate x, y, z
        0.0f, 0.0f, 1.0f, 0.0f,				// y amount of rotation, rotate x, y, z
        0.0f, 0.0f, 0.0f, 1.0f,				// z amount of rotation, rotate x, y, z
        0.0f, 0.0f, 2.0f,					// translate x, y, z
        1.0f, 1.0f
    };
    con_mesh_01.height = 1.8f;
    con_mesh_01.radius = 0.5f;
    con_mesh_01.length = 0.5f;
    con_mesh_01.number_of_sides = 144.0f;
    con_mesh_01.texFilename = "textures/lava.png";
    con_mesh_01.transparency = 0.7f;
    con_mesh_01.material = glow;
    lightSources.push_back(true);
    con_mesh_01.lightSourceId = lightSources.size() - 1;

    UBuildCone(con_mesh_01);
    scene.push_back(con_mesh_01);



    // CONE - Lower Glass of Lava Lamp
    GLMesh con_mesh_02;
    con_mesh_02.p = {
        1.0f, 1.0f, 1.0f, 1.0f,				// color r, g, b a
        1.0f, 1.0f, 1.0f,					// scale x, y, z
        180.0f, 1.0f, 0.0f, 0.0f,				// x amount of rotation, rotate x, y, z
        0.0f, 0.0f, 1.0f, 0.0f,				// y amount of rotation, rotate x, y, z
        0.0f, 0.0f, 0.0f, 1.0f,				// z amount of rotation, rotate x, y, z
        0.0f, 0.0f, 3.0f,					// translate x, y, z
        1.0f, 1.0f
    };
    con_mesh_02.height = 0.5f;
    con_mesh_02.radius = 0.5f;
    con_mesh_02.length = 0.5f;
    con_mesh_02.number_of_sides = 144.0f;
    con_mesh_02.texFilename = "textures/blacksparkle.png";
    con_mesh_02.material = gloss;
    UBuildCone(con_mesh_02);
    scene.push_back(con_mesh_02);

    // CONE - Base of Lava Lamp
    GLMesh con_mesh_03;
    con_mesh_03.p = {
        1.0f, 1.0f, 1.0f, 1.0f,				// color r, g, b a
        1.0f, 1.0f, 1.0f,					// scale x, y, z
        0.0f, 1.0f, 0.0f, 0.0f,				// x amount of rotation, rotate x, y, z
        0.0f, 0.0f, 1.0f, 0.0f,				// y amount of rotation, rotate x, y, z
        0.0f, 0.0f, 0.0f, 1.0f,				// z amount of rotation, rotate x, y, z
        0.0f, -1.0f, 2.0f,					// translate x, y, z
        1.0f, 1.0f
    };
    con_mesh_03.height = 1.0f;
    con_mesh_03.radius = 0.5f;
    con_mesh_03.length = 0.5f;
    con_mesh_03.number_of_sides = 144.0f;
    con_mesh_03.texFilename = "textures/blacksparkle.png";
    con_mesh_03.material = gloss;
    UBuildCone(con_mesh_03);
    scene.push_back(con_mesh_03);

    //------------------------------------------------------- END LAVA LAMP ------------------------------------------------------------------------

    //------------------------------------------------------- COFFEE MUG ----------------------------------------------------------------------

    GLMesh hollow_cyl;
    hollow_cyl.p = {
        1.0f,	1.0f,	1.0f,	1.0f,
        0.8f,	0.8f,	0.8f,
        0.0f,	1.0f,	0.0f,	0.0f,
        -30.0f,	0.0f,	1.0f,	0.0f,
        0.0f,	0.0f,	0.0f,	1.0f,
        -1.0f,	-1.0f,	1.1f,
        1.0f,	1.0f
    };
    hollow_cyl.texFilename = "textures/mug1.png";
    hollow_cyl.innerRadius = 0.45f;
    hollow_cyl.radius = 0.5f;
    hollow_cyl.height = 1.0f;
    hollow_cyl.number_of_sides = 144.0f;
    hollow_cyl.material = gloss;
    UBuildHollowCylinder(hollow_cyl);
    scene.push_back(hollow_cyl);

    GLMesh handle_cyl;
    handle_cyl.p = {
        1.0f,	1.0f,	1.0f,	1.0f,
        0.3f,	0.1f,	0.4f,
        90.0f,	1.0f,	0.0f,	0.0f,
        0.0f,	0.0f,	1.0f,	0.0f,
        -60.0f,	0.0f,	0.0f,	1.0f,
        -1.2f,	-0.35f,	2.15f,
        1.0f,	1.0f
    };
    handle_cyl.texFilename = "textures/mug2.png";
    handle_cyl.innerRadius = 0.35f;
    handle_cyl.radius = 0.5f;
    handle_cyl.height = 1.0f;
    handle_cyl.number_of_sides = 144.0f;
    handle_cyl.material = gloss;
    UBuildHollowCylinder(handle_cyl);
    scene.push_back(handle_cyl);

    GLMesh coffee;
    coffee.p = {
        1.0f,	1.0f,	1.0f,	1.0f,
        0.8f,	0.8f,	0.8f,
        0.0f,	1.0f,	0.0f,	0.0f,
        180.0f,	0.0f,	1.0f,	0.0f,
        0.0f,	0.0f,	0.0f,	1.0f,
        -0.47f,	-0.4f,	2.05f,
        1.0f,	1.0f
    };
    coffee.radius = 0.45f;
    coffee.number_of_sides = 144.0f;
    coffee.material = satin;
    coffee.texFilename = "textures/coffee1.png";
    UBuildCircle(coffee);
    scene.push_back(coffee);

    //------------------------------------------------------- PENCIL GLASS ----------------------------------------------------------------------

    GLMesh hollow_cyl2;
    hollow_cyl2.p = {
        1.0f,	1.0f,	1.0f,	1.0f,
        0.5f,	0.8f,	0.5f,
        0.0f,	1.0f,	0.0f,	0.0f,
        -30.0f,	0.0f,	1.0f,	0.0f,
        0.0f,	0.0f,	0.0f,	1.0f,
        -0.8f,	-1.0f,	-3.0f,
        1.0f,	1.0f
    };
    hollow_cyl2.texFilename = "textures/glass1.png";
    hollow_cyl2.innerRadius = 0.45f;
    hollow_cyl2.radius = 0.5f;
    hollow_cyl2.height = 1.0f;
    hollow_cyl2.number_of_sides = 144.0f;
    hollow_cyl2.material = gloss;
    hollow_cyl2.transparency = 0.4f;
    UBuildHollowCylinder(hollow_cyl2);
    scene.push_back(hollow_cyl2);

    GLMesh cyl_gMesh01;
	cyl_gMesh01.p = {
		1.0f, 1.0f, 1.0f, 1.0f,
		0.06f, 0.3f, 0.06f,
		-25.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
		-0.8f, -1.0f, -2.5f,
		1.0f, 1.0f
	};
	cyl_gMesh01.height = 4.0f;
	cyl_gMesh01.radius = 0.5f;
	cyl_gMesh01.number_of_sides = 128.0f;
	cyl_gMesh01.texFilename = "textures/pencil1.png";
	UBuildCylinder(cyl_gMesh01);
	scene.push_back(cyl_gMesh01);

    GLMesh cyl_gMesh02;
    cyl_gMesh02.p = {
        1.0f, 1.0f, 1.0f, 1.0f,
        0.06f, 0.3f, 0.06f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -20.0f, 0.0f, 0.0f, 1.0f,
        -0.85f, -1.0f, -2.7f,
        1.0f, 1.0f
    };
    cyl_gMesh02.height = 4.0f;
    cyl_gMesh02.radius = 0.5f;
    cyl_gMesh02.number_of_sides = 128.0f;
    cyl_gMesh02.texFilename = "textures/pencil2.png";
    UBuildCylinder(cyl_gMesh02);
    scene.push_back(cyl_gMesh02);



    //------------------------------------------------------------ DESK ----------------------------------------------------------------------------


    GLMesh desk_gMesh01;
    desk_gMesh01.p = {
        1.0f,	1.0f,	1.0f,	1.0f,
        2.6f,	0.07f,	3.3f,
        0.0f,	1.0f,	0.0f,	0.0f,
        0.0f,	0.0f,	1.0f,	0.0f,
        30.0f,	0.0f,	0.0f,	1.0f,
        0.0f,	-0.32,	-0.5f,
        1.0f,	1.0f
    };
    desk_gMesh01.texFilename = "textures/lightboard.png";
    desk_gMesh01.material = satin;
    UBuildCube(desk_gMesh01);
    scene.push_back(desk_gMesh01);

    GLMesh desk_gMesh02;
    desk_gMesh02.p = {
        1.0f,	1.0f,	1.0f,	1.0f,
        0.1f,	1.15f,	0.1f,
        0.0f,	1.0f,	0.0f,	0.0f,
        0.0f,	0.0f,	1.0f,	0.0f,
        30.0f,	0.0f,	0.0f,	1.0f,
        1.0f,	-1.0f,	1.0f,
        1.0f,	1.0f
    };
    desk_gMesh02.texFilename = "textures/black.png";
    desk_gMesh02.material = satin;
    UBuildCube(desk_gMesh02);
    scene.push_back(desk_gMesh02);

    GLMesh desk_gMesh03;
    desk_gMesh03.p = {
        1.0f,	1.0f,	1.0f,	1.0f,
        0.1f,	1.15f,	0.1f,
        0.0f,	1.0f,	0.0f,	0.0f,
        0.0f,	0.0f,	1.0f,	0.0f,
        30.0f,	0.0f,	0.0f,	1.0f,
        1.0f,	-1.0f,	-2.0f,
        1.0f,	1.0f
    };
    desk_gMesh03.texFilename = "textures/black.png";
    desk_gMesh03.material = satin;
    UBuildCube(desk_gMesh03);
    scene.push_back(desk_gMesh03);

    GLMesh paper_gMesh01;
    paper_gMesh01.p = {
        1.0f, 1.0f, 1.0f, 1.0f,				// color r, g, b a
        1.0f, 1.0f, 0.75f,					// scale x, y, z
        0.0f, 1.0f, 0.0f, 0.0f,				// x amount of rotation, rotate x, y, z
        0.0f, 0.0f, 1.0f, 0.0f,				// y amount of rotation, rotate x, y, z
        30.0f, 0.0f, 0.0f, 1.0f,				// z amount of rotation, rotate x, y, z
        0.0f, -0.22f, -0.2f,					// translate x, y, z
        1.0f, 1.0f
    };



    paper_gMesh01.texFilename = "textures/drawing3.png";
    paper_gMesh01.gUVScale = glm::vec2(0.5f);
    UBuildPlane(paper_gMesh01);
    scene.push_back(paper_gMesh01);

    GLMesh pencil_gMesh01;
    pencil_gMesh01.p = {
        1.0f, 1.0f, 1.0f, 1.0f,
        0.06f, 0.3f, 0.06f,
        90.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        20.0f, 0.0f, 0.0f, 1.0f,
        -1.3f, -0.9f, -2.0f,
        1.0f, 1.0f
    };
    pencil_gMesh01.height = 4.0f;
    pencil_gMesh01.radius = 0.5f;
    pencil_gMesh01.number_of_sides = 128.0f;
    pencil_gMesh01.texFilename = "textures/pencil2.png";
    UBuildCylinder(pencil_gMesh01);
    scene.push_back(pencil_gMesh01);

  
    GLMesh con_mesh_04;
    con_mesh_04.p = {
        1.0f, 1.0f, 1.0f, 1.0f,				// color r, g, b a
        0.06f, 0.2f, 0.06f,					// scale x, y, z
        90.0f, 1.0f, 0.0f, 0.0f,				// x amount of rotation, rotate x, y, z
        0.0f, 0.0f, 1.0f, 0.0f,				// y amount of rotation, rotate x, y, z
        -160.0f, 0.0f, 0.0f, 1.0f,				// z amount of rotation, rotate x, y, z
        -1.244f, -0.9f, -1.98f,					// translate x, y, z
        1.0f, 1.0f
    };
    con_mesh_04.height = 1.0f;
    con_mesh_04.radius = 0.5f;
    con_mesh_04.length = 0.5f;
    con_mesh_04.number_of_sides = 144.0f;
    con_mesh_04.texFilename = "textures/penciltip1.png";
    UBuildCone(con_mesh_04);
    scene.push_back(con_mesh_04);





    //------------------------------------------------------- BASE PLANE ---------------------------------------------------------------------------

    GLMesh plan_gMesh01;
    plan_gMesh01.p = {
        1.0f, 1.0f, 1.0f, 1.0f,				// color r, g, b a
        2.0f, 6.0f, 3.0f,					// scale x, y, z
        0.0f, 1.0f, 0.0f, 0.0f,				// x amount of rotation, rotate x, y, z
        0.0f, 0.0f, 1.0f, 0.0f,				// y amount of rotation, rotate x, y, z
        0.0f, 0.0f, 0.0f, 1.0f,				// z amount of rotation, rotate x, y, z
        0.0f, -1.0f, 0.0f,					// translate x, y, z
        1.0f, 1.0f
    };

    plan_gMesh01.texFilename = "textures/wood1.png";

    UBuildPlane(plan_gMesh01);
    scene.push_back(plan_gMesh01);








}

void UBuildCube(GLMesh& mesh)
{
    vector<float> c = { mesh.p[0], mesh.p[1], mesh.p[2], mesh.p[3] };

    mesh.v = {
        0.5f,	0.0f,	0.5f,	0.0f,	0.0f,	1.0f,	1.0f,	0.25f,	0.5f,	// front left
        -0.5f,	0.0f,	0.5f,	0.0f,	0.0f,	1.0f,	1.0f,	0.0f,	0.5f,
        -0.5f,	1.0f,	0.5f,	0.0f,	0.0f,	1.0f,	1.0f,	0.0f,	1.0f,

        0.5f,	0.0f,	0.5f,	0.0f,	0.0f,	1.0f,	1.0f,	0.25f,	0.5f,	// front right
        0.5f,	1.0f,	0.5f,	0.0f,	0.0f,	1.0f,	1.0f,	0.25f,	1.0f,
        -0.5f,	1.0f,	0.5f,	0.0f,	0.0f,	1.0f,	1.0f,	0.0f,	1.0f,


        0.5f,	0.0f,	0.5f,	1.0f,	0.0f,	0.0f,	1.0f,	0.25f,	0.5f,	// right front
        0.5f,	1.0f,	0.5f,	1.0f,	0.0f,	0.0f,	1.0f,	0.25f,	1.0f,
        0.5f,	1.0f,	-0.5f,	1.0f,	0.0f,	0.0f,	1.0f,	0.5f,	1.0f,

        0.5f,	0.0f,	0.5f,	1.0f,	0.0f,	0.0f,	1.0f,	0.25f,	0.5f,	// right back
        0.5f,	0.0f,	-0.5f,	1.0f,	0.0f,	0.0f,	1.0f,	0.5f,	0.5f,
        0.5f,	1.0f,	-0.5f,	1.0f,	0.0f,	0.0f,	1.0f,	0.5f,	1.0f,


        0.5f,	0.0f,	-0.5f,	0.0f,	0.0f,	-1.0f,	1.0f,	0.5f,	0.5f,	// back left
        -0.5f,	0.0f,	-0.5f,	0.0f,	0.0f,	-1.0f,	1.0f,	0.75f,	0.5f,
        -0.5f,	1.0f,	-0.5f,	0.0f,	0.0f,	-1.0f,	1.0f,	0.75f,	1.0f,

        0.5f,	0.0f,	-0.5f,	0.0f,	0.0f,	-1.0f,	1.0f,	0.5f,	0.5f,	// back right
        0.5f,	1.0f,	-0.5f,	0.0f,	0.0f,	-1.0f,	1.0f,	0.5f,	1.0f,
        -0.5f,	1.0f,	-0.5f,	0.0f,	0.0f,	-1.0f,	1.0f,	0.75f,	1.0f,


        -0.5f,	0.0f,	0.5f,	-1.0f,	0.0f,	0.0f,	1.0f,	1.0f,	0.5f,	// left back
        -0.5f,	1.0f,	0.5f,	-1.0f,	0.0f,	0.0f,	1.0f,	1.0f,	1.0f,
        -0.5f,	1.0f,	-0.5f,	-1.0f,	0.0f,	0.0f,	1.0f,	0.75f,	1.0f,

        -0.5f,	0.0f,	0.5f,	-1.0f,	0.0f,	0.0f,	1.0f,	1.0f,	0.5f,	// left front
        -0.5f,	0.0f,	-0.5f,	-1.0f,	0.0f,	0.0f,	1.0f,	0.75f,	0.5f,
        -0.5f,	1.0f,	-0.5f,	-1.0f,	0.0f,	0.0f,	1.0f,	0.75f,	1.0f,




        -0.5f,	1.0f,	0.5f,	-0.0f,	1.0f,	0.0f,	1.0f,	0.0f,	0.0f,	// top left
        -0.5f,	1.0f,	-0.5f,	-0.0f,	1.0f,	0.0f,	1.0f,	0.0f,	0.5f,
        0.5f,	1.0f,	0.5f,	-0.0f,	1.0f,	0.0f,	1.0f,	0.25f,	0.0f,

        -0.5f,	1.0f,	-0.5f,	-0.0f,	1.0f,	0.0f,	1.0f,	0.0f,	0.5f,	// top right
        0.5f,	1.0f,	0.5f,	-0.0f,	1.0f,	0.0f,	1.0f,	0.25f,	0.0f,
        0.5f,	1.0f,	-0.5f,	-0.0f,	1.0f,	0.0f,	1.0f,	0.25f,	0.5f,

        -0.5f,	0.0f,	0.5f,	0.0f,	-1.0f,	0.0f,	1.0f,	0.0f,	0.0f,	// bottom left
        -0.5f,	0.0f,	-0.5f,	0.0f,	-1.0f,	0.0f,	1.0f,	0.0f,	0.5f,
        0.5f,	0.0f,	0.5f,	0.0f,	-1.0f,	0.0f,	1.0f,	0.25f,	0.0f,

        -0.5f,	0.0f,	-0.5f,	0.0f,	-1.0f,	0.0f,	1.0f,	0.0f,	0.5f,	// bottom right
        0.5f,	0.0f,	0.5f,	0.0f,	-1.0f,	0.0f,	1.0f,	0.25f,	0.0f,
        0.5f,	0.0f,	-0.5f,	0.0f,	-1.0f,	0.0f,	1.0f,	0.25f,	0.5f,

    };

    UTranslator(mesh);
}

void UBuildCircle(GLMesh& mesh)
{
    vector<float> c = { mesh.p[0], mesh.p[1], mesh.p[2], mesh.p[3] };


    float r = mesh.radius;
    float l = mesh.length;
    float s = mesh.number_of_sides;
    float h = mesh.height;

    constexpr float PI = 3.14f;
    const float sectorStep = 2.0f * PI / s;

    vector<float> v;



    for (auto i = 1; i < s + 1; i++)
    {

        float one = 0.5f + r * cos(i * sectorStep);
        float two = 0.5f + r * sin(i * sectorStep);

        one -= 0.5f;
        one *= 2.0f;

        two -= 0.5f;
        two *= 2.0f;

        c[0] = one;
        c[2] = two;
        // triangle fan, top
        v.insert(v.end(), { 0.5f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.5f, 0.5f });			// origin (0.5, 0.5) works best for textures
        v.insert(v.end(), { 0.5f + r * cos(i * sectorStep) ,
                                        0.0f ,										// build this fan the 'l' value away from the other fan
                                        0.5f + r * sin(i * sectorStep) ,
                                        c[0], 1.0f, c[2], 1.0f,					// color data r g b a
                                        0.5f + (0.5f * cos((i)*sectorStep)) ,
                                        0.5f + (0.5f * sin((i)*sectorStep)) });
        v.insert(v.end(), { 0.5f + r * cos((i + 1) * sectorStep) ,
                                        0.0f ,
                                        0.5f + r * sin((i + 1) * sectorStep) ,
                                        c[0], 1.0f, c[2], 1.0f,					// color data r g b a
                                        0.5f + (0.5f * cos((i + 1) * sectorStep)) ,
                                        0.5f + (0.5f * sin((i + 1) * sectorStep)) });
    }
    mesh.v = v;
    v.clear();
    UTranslator(mesh);
}

void UBuildHollowCylinder(GLMesh& mesh)
{
    vector<float> c = { mesh.p[0], mesh.p[1], mesh.p[2], mesh.p[3] };

    float ir = mesh.innerRadius;
    float r = mesh.radius;
    float h = mesh.height;
    float s = mesh.number_of_sides;

    constexpr float PI = 3.14f;
    const float sectorStep = 2.0f * PI / s;

    vector<float> v;

    for (auto i = 0; i < s; i++)
    {

        float one = 0.5f + r * cos(i * sectorStep);
        float two = 0.5f + r * sin(i * sectorStep);

        one -= 0.5f;
        one *= 2.0f;

        two -= 0.5f;
        two *= 2.0f;

        c[0] = one;
        c[2] = two;


        // FOR TEXTURE COORDS
        // use distance formula
        // x2 = x1 + d * cos(theta)
        // y2 = y1 + d * sin(theta)
        //
        // x1 = 0.5, d = 0.5 for outer radius, always; d = (inner radius / outer radius * 0.5)
        // theta = i * sectorStep
        //
        // y1 = 0.125, d = 0.125 for outer radius, always; d = (inner radius / outer radius * 0.125)
        // theta = i * sectorStep
        //


        //BOTTOM OF HOLLOW CYLINDER
        v.insert(v.end(), { 0.5f + ir * cos(i * sectorStep) ,
                                        0.0f ,
                                        0.5f + ir * sin(i * sectorStep) ,
                                        -c[0], -1.0f, -c[2], 1.0f,
                                        0.5f + ((ir / r * 0.5f) * cos((i)*sectorStep)) ,
                                        (0.125f + ((ir / r * 0.125f) * sin((i)*sectorStep))) });

        v.insert(v.end(), { 0.5f + r * cos(i * sectorStep) ,					// x
                                        0.0f ,												// y
                                        0.5f + r * sin(i * sectorStep) ,					// z
                                        c[0], -1.0f, c[2], 1.0f,							// color data r g b a
                                        0.5f + (0.5f * cos((i)*sectorStep)) ,			// texture x; adding the origin for proper alignment
                                        (0.125f + 0.125f * sin((i)*sectorStep)) });		// texture y

        v.insert(v.end(), { 0.5f + ir * cos((i + 1) * sectorStep) ,
                                        0.0f ,
                                        0.5f + ir * sin((i + 1) * sectorStep) ,
                                        -c[0], -1.0f, -c[2], 1.0f,
                                        0.5f + ((ir / r * 0.5f) * cos((i + 1) * sectorStep)) ,
                                        (0.125f + ((ir / r * 0.125f) * sin((i + 1) * sectorStep))) });
        v.insert(v.end(), { 0.5f + ir * cos((i + 1) * sectorStep) ,
                                        0.0f ,
                                        0.5f + ir * sin((i + 1) * sectorStep) ,
                                        -c[0], -1.0f, -c[2], 1.0f,
                                        0.5f + ((ir / r * 0.5f) * cos((i + 1) * sectorStep)) ,
                                        (0.125f + ((ir / r * 0.125f) * sin((i + 1) * sectorStep))) });
        v.insert(v.end(), { 0.5f + r * cos((i + 1) * sectorStep) ,
                                        0.0f ,
                                        0.5f + r * sin((i + 1) * sectorStep) ,
                                        c[0], -1.0f, c[2], 1.0f,
                                        0.5f + (0.5f * cos((i + 1) * sectorStep)) ,
                                        (0.125f + 0.125f * sin((i + 1) * sectorStep)) });
        v.insert(v.end(), { 0.5f + r * cos(i * sectorStep) ,
                                        0.0f ,
                                        0.5f + r * sin(i * sectorStep) ,
                                        c[0], -1.0f, c[2], 1.0f,
                                        0.5f + (0.5f * cos((i)*sectorStep)) ,
                                        (0.125f + (0.125f * sin((i)*sectorStep))) });

    }

    for (auto i = 0; i < s; i++)
    {
        float one = 0.5f + r * cos(i * sectorStep);
        float two = 0.5f + r * sin(i * sectorStep);

        one -= 0.5f;
        one *= 2.0f;

        two -= 0.5f;
        two *= 2.0f;

        c[0] = one;
        c[2] = two;

        //TOP OF HOLLOW CYLINDER
        v.insert(v.end(), { 0.5f + ir * cos(i * sectorStep) ,
                                        h ,
                                        0.5f + ir * sin(i * sectorStep) ,
                                        -c[0], 1.0f, -c[2], 1.0f,
                                        0.5f + ((ir / r * 0.5f) * cos((i)*sectorStep)) ,
                                        (0.125f + ((ir / r * 0.125f) * sin((i)*sectorStep))) });

        v.insert(v.end(), { 0.5f + r * cos(i * sectorStep) ,					// x
                                        h ,												// y
                                        0.5f + r * sin(i * sectorStep) ,					// z
                                        c[0], 1.0f, c[2], 1.0f,							// color data r g b a
                                        0.5f + (0.5f * cos((i)*sectorStep)) ,			// texture x; adding the origin for proper alignment
                                        (0.125f + 0.125f * sin((i)*sectorStep)) });		// texture y

        v.insert(v.end(), { 0.5f + ir * cos((i + 1) * sectorStep) ,
                                        h ,
                                        0.5f + ir * sin((i + 1) * sectorStep) ,
                                        -c[0], 1.0f, -c[2], 1.0f,
                                        0.5f + ((ir / r * 0.5f) * cos((i + 1) * sectorStep)) ,
                                        (0.125f + ((ir / r * 0.125f) * sin((i + 1) * sectorStep))) });
        v.insert(v.end(), { 0.5f + ir * cos((i + 1) * sectorStep) ,
                                        h ,
                                        0.5f + ir * sin((i + 1) * sectorStep) ,
                                        -c[0], 1.0f, -c[2], 1.0f,
                                        0.5f + ((ir / r * 0.5f) * cos((i + 1) * sectorStep)) ,
                                        (0.125f + ((ir / r * 0.125f) * sin((i + 1) * sectorStep))) });
        v.insert(v.end(), { 0.5f + r * cos((i + 1) * sectorStep) ,
                                        h ,
                                        0.5f + r * sin((i + 1) * sectorStep) ,
                                        c[0], 1.0f, c[2], 1.0f,
                                        0.5f + (0.5f * cos((i + 1) * sectorStep)) ,
                                        (0.125f + 0.125f * sin((i + 1) * sectorStep)) });
        v.insert(v.end(), { 0.5f + r * cos(i * sectorStep) ,
                                        h ,
                                        0.5f + r * sin(i * sectorStep) ,
                                        c[0], 1.0f, c[2], 1.0f,
                                        0.5f + (0.5f * cos((i)*sectorStep)) ,
                                        (0.125f + (0.125f * sin((i)*sectorStep))) });

    }

    constexpr float x = 1.0f;
    float j = 1.0f / (s / x);	// for calculating texture location; change 'x' to increase or decrease how many times the texture wraps around the cylinder
    float k = 0.0f;				// for texture clamping


    // OUTSIDE SIDES OF HOLLOW CYLINDER
    for (auto i = 0; i < s; i++)
    {
        float one = 0.5f + r * cos(i * sectorStep);
        float two = 0.5f + r * sin(i * sectorStep);

        one -= 0.5f;
        one *= 2.0f;

        two -= 0.5f;
        two *= 2.0f;

        c[0] = one;
        c[2] = two;


        v.insert(v.end(), { 0.5f + r * cos(i * sectorStep) ,
                                        0.0f ,
                                        0.5f + r * sin(i * sectorStep) ,
                                        c[0], 0.0f, c[2], c[3],					// color data r g b a
                                        k ,
                                        0.25f });

        v.insert(v.end(), { 0.5f + r * cos(i * sectorStep) ,
                                        h ,
                                        0.5f + r * sin(i * sectorStep) ,
                                        c[0], 0.0f, c[2], c[3],					// color data r g b a
                                        k ,
                                        0.75f });
        v.insert(v.end(), { 0.5f + r * cos((i + 1) * sectorStep) ,
                                        h ,
                                        0.5f + r * sin((i + 1) * sectorStep) ,
                                        c[0], 0.0f, c[2], c[3],					// color data r g b a
                                        k + j ,
                                        0.75f });

        v.insert(v.end(), { 0.5f + r * cos((i + 1) * sectorStep) ,
                                        h ,
                                        0.5f + r * sin((i + 1) * sectorStep) ,
                                        c[0], 0.0f, c[2], c[3],					// color data r g b a
                                        k + j ,
                                        0.75f });
        v.insert(v.end(), { 0.5f + r * cos((i + 1) * sectorStep) ,
                                        0.0f ,
                                        0.5f + r * sin((i + 1) * sectorStep) ,
                                        c[0], 0.0f, c[2], c[3],					// color data r g b a
                                        k + j ,
                                        0.25f });

        v.insert(v.end(), { 0.5f + r * cos(i * sectorStep) ,
                                        0.0f ,
                                        0.5f + r * sin(i * sectorStep) ,
                                        c[0], 0.0f, c[2], c[3],					// color data r g b a
                                        k,
                                        0.25f });
        k += j;
    }

    // INSIDE SIDES OF HOLLOW CYLINDER
    for (auto i = 0; i < s; i++)
    {
        float one = 0.5f + r * cos(i * sectorStep);
        float two = 0.5f + r * sin(i * sectorStep);

        one -= 0.5f;
        one *= 2.0f;

        two -= 0.5f;
        two *= 2.0f;

        c[0] = one;
        c[2] = two;


        v.insert(v.end(), { 0.5f + ir * cos(i * sectorStep) ,
                                        0.0f ,
                                        0.5f + ir * sin(i * sectorStep) ,
                                        -c[0], 0.0f, -c[2], c[3],					// color data r g b a
                                        k ,
                                        0.25f });

        v.insert(v.end(), { 0.5f + ir * cos(i * sectorStep) ,
                                        h ,
                                        0.5f + ir * sin(i * sectorStep) ,
                                        -c[0], 0.0f, -c[2], c[3],					// color data r g b a
                                        k ,
                                        0.75f });
        v.insert(v.end(), { 0.5f + ir * cos((i + 1) * sectorStep) ,
                                        h ,
                                        0.5f + ir * sin((i + 1) * sectorStep) ,
                                        -c[0], 0.0f, -c[2], c[3],					// color data r g b a
                                        k + j ,
                                        0.75f });

        v.insert(v.end(), { 0.5f + ir * cos((i + 1) * sectorStep) ,
                                        h ,
                                        0.5f + ir * sin((i + 1) * sectorStep) ,
                                        -c[0], 0.0f, -c[2], c[3],					// color data r g b a
                                        k + j ,
                                        0.75f });
        v.insert(v.end(), { 0.5f + ir * cos((i + 1) * sectorStep) ,
                                        0.0f ,
                                        0.5f + ir * sin((i + 1) * sectorStep) ,
                                        -c[0], 0.0f, -c[2], c[3],					// color data r g b a
                                        k + j ,
                                        0.25f });

        v.insert(v.end(), { 0.5f + ir * cos(i * sectorStep) ,
                                        0.0f ,
                                        0.5f + ir * sin(i * sectorStep) ,
                                        -c[0], 0.0f, -c[2], c[3],					// color data r g b a
                                        k,
                                        0.25f });
        k += j;
    }




    mesh.v = v;

    UTranslator(mesh);

}

void UBuildCylinder(GLMesh& mesh)
{
    vector<float> c = { mesh.p[0], mesh.p[1], mesh.p[2], mesh.p[3] };

    float r = mesh.radius;
    float h = mesh.height;
    float s = mesh.number_of_sides;


    constexpr float PI = 3.14f;
    const float sectorStep = 2.0f * PI / s;

    vector<float> v;

    for (auto i = 0; i < s; i++)
    {
        float one = 0.5f + r * cos(i * sectorStep);
        float two = 0.5f + r * sin(i * sectorStep);

        one -= 0.5f;
        one *= 2.0f;

        two -= 0.5f;
        two *= 2.0f;

        c[0] = one;
        c[2] = two;


        // triangle fan, bottom
        v.insert(v.end(), { 0.5f, 0.0f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.5f, 0.125f });			// origin (0.5, 0.5) works best for textures
        v.insert(v.end(), { 0.5f + r * cos(i * sectorStep) ,			// x
                                        0.0f ,										// y
                                        0.5f + r * sin(i * sectorStep) ,			// z
                                        c[0], -1.0f, c[2], 1.0f,						// color data r g b a
                                        0.5f + (0.5f * cos((i)*sectorStep)) ,		// texture x; adding the origin for proper alignment
                                        (0.125f + (0.125f * sin((i)*sectorStep))) });		// texture y


        v.insert(v.end(), { 0.5f + r * cos((i + 1) * sectorStep) ,
                                        0.0f ,
                                        0.5f + r * sin((i + 1) * sectorStep) ,
                                        c[0], -1.0f, c[2], 1.0f,						// color data r g b a
                                        0.5f + (0.5f * cos((i + 1) * sectorStep)) ,
                                        (0.125f + (0.125f * sin((i + 1) * sectorStep))) });


    }

    for (auto i = 1; i < s + 1; i++)
    {

        float one = 0.5f + r * cos(i * sectorStep);
        float two = 0.5f + r * sin(i * sectorStep);

        one -= 0.5f;
        one *= 2.0f;

        two -= 0.5f;
        two *= 2.0f;

        c[0] = one;
        c[2] = two;
        // triangle fan, top
        v.insert(v.end(), { 0.5f, h, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.5f, 0.875f });			// origin (0.5, 0.5) works best for textures
        v.insert(v.end(), { 0.5f + r * cos(i * sectorStep) ,
                                        h ,										// build this fan the 'l' value away from the other fan
                                        0.5f + r * sin(i * sectorStep) ,
                                        c[0], 1.0f, c[2], 1.0f,					// color data r g b a
                                        0.5f + (0.5f * cos((i)*sectorStep)) ,
                                        0.875f + (0.125f * sin((i)*sectorStep)) });
        v.insert(v.end(), { 0.5f + r * cos((i + 1) * sectorStep) ,
                                        h ,
                                        0.5f + r * sin((i + 1) * sectorStep) ,
                                        c[0], 1.0f, c[2], 1.0f,					// color data r g b a
                                        0.5f + (0.5f * cos((i + 1) * sectorStep)) ,
                                        0.875f + (0.125f * sin((i + 1) * sectorStep)) });
    }

    // since all side triangles have the same points as the fans above, the same calculations are used
    // to wrap the texture around the cylinder, the calculated points are used to determine which section of
    // the texture to clamp to the corresponding point.
    constexpr float x = 1.0f;
    float j = 1.0f / (s / x);	// for calculating texture location; change 'x' to increase or decrease how many times the texture wraps around the cylinder
    float k = 0.0f;				// for texture clamping

    // sides
    for (auto i = 0; i < s; i++)
    {
        float one = 0.5f + r * cos(i * sectorStep);
        float two = 0.5f + r * sin(i * sectorStep);

        one -= 0.5f;
        one *= 2.0f;

        two -= 0.5f;
        two *= 2.0f;

        c[0] = one;
        c[2] = two;


        v.insert(v.end(), { 0.5f + r * cos(i * sectorStep) ,
                                        0.0f ,
                                        0.5f + r * sin(i * sectorStep) ,
                                        c[0], 0.0f, c[2], c[3],					// color data r g b a
                                        k ,
                                        0.25f });

        v.insert(v.end(), { 0.5f + r * cos(i * sectorStep) ,
                                        h ,
                                        0.5f + r * sin(i * sectorStep) ,
                                        c[0], 0.0f, c[2], c[3],					// color data r g b a
                                        k ,
                                        0.75f });
        v.insert(v.end(), { 0.5f + r * cos((i + 1) * sectorStep) ,
                                        h ,
                                        0.5f + r * sin((i + 1) * sectorStep) ,
                                        c[0], 0.0f, c[2], c[3],					// color data r g b a
                                        k + j ,
                                        0.75f });

        v.insert(v.end(), { 0.5f + r * cos((i + 1) * sectorStep) ,
                                        h ,
                                        0.5f + r * sin((i + 1) * sectorStep) ,
                                        c[0], 0.0f, c[2], c[3],					// color data r g b a
                                        k + j ,
                                        0.75f });
        v.insert(v.end(), { 0.5f + r * cos((i + 1) * sectorStep) ,
                                        0.0f ,
                                        0.5f + r * sin((i + 1) * sectorStep) ,
                                        c[0], 0.0f, c[2], c[3],					// color data r g b a
                                        k + j ,
                                        0.25f });

        v.insert(v.end(), { 0.5f + r * cos(i * sectorStep) ,
                                        0.0f ,
                                        0.5f + r * sin(i * sectorStep) ,
                                        c[0], 0.0f, c[2], c[3],					// color data r g b a
                                        k,
                                        0.25f });
        k += j;
    }

    mesh.v = v;
    v.clear();
    UTranslator(mesh);

}


void UBuildCone(GLMesh& mesh)
{
    vector<float> c = { mesh.p[0], mesh.p[1], mesh.p[2], mesh.p[3] };

    float r = mesh.radius;
    float h = mesh.height;
    float s = mesh.number_of_sides;

    constexpr float PI = 3.14f;
    const float sectorStep = 2.0f * PI / s;
    const float textStep = 1.0f / s;
    float textureXLoc = 0.0f;

    vector<float> v;

    for (auto i = 1; i < s + 1; i++) {

        float one = 0.5f + r * cos(i * sectorStep);
        float two = 0.5f + r * sin(i * sectorStep);

        one -= 0.5f;
        one *= 2.0f;

        two -= 0.5f;
        two *= 2.0f;

        c[0] = one;
        c[2] = two;


        // triangle fan, bottom
        v.insert(v.end(), { 0.5f, 0.0f, 0.5f, c[0], -1.0f, c[2], c[3], 0.5f, 0.25f });		// center point; x, y, z, r, g, b, a, texture x, texture y
        v.insert(v.end(), { 0.5f + r * cos(i * sectorStep) ,				// first outer point
                                        0.0f ,
                                        0.5f + r * sin(i * sectorStep) ,
                                        c[0], -1.0f, c[2], c[3],
            /*textureXLoc,
            0.0f*/
           0.5f + (r * cos((i)*sectorStep)) ,	// texture x; adding the origin for proper alignment
            0.5f + (r * sin((i)*sectorStep))
            });
        v.insert(v.end(), { 0.5f + (r * cos((i + 1) * sectorStep)) ,		// second outer point
                                        0.0f ,
                                        0.5f + (r * sin((i + 1) * sectorStep)) ,
                                        c[0], -1.0f, c[2], c[3],
            /*textureXLoc + textStep,
            0.0f*/
            0.5f + (r * cos((i + 1) * sectorStep)) ,
            0.25f + (0.25f * sin((i + 1) * sectorStep))
            });


        // side triangle + point
        v.insert(v.end(), { 0.5f + (r * cos(i * sectorStep)) ,
                                        0.0f ,
                                        0.5f + (r * sin(i * sectorStep)) ,
                                        c[0], 1.0f, c[2], c[3],
                                        textureXLoc ,
                                        0.0f });
        v.insert(v.end(), { 0.5f + (r * cos((i + 1) * sectorStep)) ,
                                        0.0f ,
                                        0.5f + (r * sin((i + 1) * sectorStep)) ,
                                        c[0], 1.0f, c[2], c[3],
                                        textureXLoc + textStep,
                                        0.0f });
        v.insert(v.end(), { 0.5f , h , 0.5f , c[0], 1.0f, c[2], c[3], textureXLoc + (textStep / 2), 1.0f });		// origin, peak

        textureXLoc += textStep;

    }

    mesh.v = v;
    v.clear();	// clear the local vector

    UTranslator(mesh);
}



void UBuildPlane(GLMesh& mesh)
{
    vector<float> c = { mesh.p[0], mesh.p[1], mesh.p[2], mesh.p[3] };



    mesh.v = {

        -1.0f,	0.0f,	-1.0f,	0.0f,	1.0f,	0.0f,	1.0f,	0.0f,	1.0f,	// 0
         0.0f,	0.0f,	 1.0f,	0.0f,	1.0f,	0.0f,	1.0f,	0.5f,	0.0f,	// 1
        -1.0f,	0.0f,	 1.0f,	0.0f,	1.0f,	0.0f,	1.0f,	0.0f,	0.0f,	// 2

        -1.0f,	0.0f,	-1.0f, 	0.0f,	1.0f,	0.0f,	1.0f,	0.0f,	1.0f,	// 0
         0.0f,	0.0f,	 1.0f,	0.0f,	1.0f,	0.0f,	1.0f,	0.5f,	0.0f,	// 2
         0.0f,	0.0f,	-1.0f, 	0.0f,	1.0f,	0.0f,	1.0f,	0.5f,	1.0f,	// 3

         0.0f,	0.0f,	-1.0f, 	0.0f,	1.0f,	0.0f,	1.0f,	0.5f,	1.0f,	// 3
         0.0f,	0.0f,	 1.0f, 	0.0f,	1.0f,	0.0f,	1.0f,	0.5f,	0.0f,	// 2
         1.0f,	0.0f,	 1.0f, 	0.0f,	1.0f,	0.0f,	1.0f,	1.0f,	0.0f,	// 5

         0.0f,	0.0f,	-1.0f, 	0.0f,	1.0f,	0.0f,	1.0f,	0.5f,	1.0f,	// 3
         1.0f,	0.0f,	 1.0f, 	0.0f,	1.0f,	0.0f,	1.0f,	1.0f,	0.0f,	// 5
         1.0f,	0.0f,	-1.0f, 	0.0f,	1.0f,	0.0f,	1.0f,	1.0f,	1.0f,	// 4

    };

    UTranslator(mesh);

}



void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(2, mesh.vbos);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}

bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}
void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

void UTranslator(GLMesh& mesh)
{
    // build the mesh

    constexpr GLuint floatsPerVertex = 3;
    constexpr GLuint floatsPerColor = 4;
    constexpr GLuint floatsPerUV = 2;

    mesh.nIndices = mesh.v.size() / (floatsPerVertex + floatsPerUV + floatsPerColor);

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    // Create VBO
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer

    // use vector instead of array
    glBufferData(
        GL_ARRAY_BUFFER,
        mesh.v.size() * sizeof(float),
        &mesh.v.front(),
        GL_STATIC_DRAW
    ); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates
    constexpr GLint stride = sizeof(float) * (floatsPerVertex + floatsPerUV + floatsPerColor);

    // Create Vertex Attribute Pointers
    // location
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // texture
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);


    // scale the object
    mesh.scale = glm::scale(glm::vec3(mesh.p[4], mesh.p[5], mesh.p[6]));

    const glm::mat4 rot = glm::mat4(1.0f);

    // rotate the object (x, y, z) (0 - 6.4, to the right)
    mesh.xrotation = glm::rotate(rot, glm::radians(mesh.p[7]), glm::vec3(mesh.p[8], mesh.p[9], mesh.p[10]));
    mesh.yrotation = glm::rotate(rot, glm::radians(mesh.p[11]), glm::vec3(mesh.p[12], mesh.p[13], mesh.p[14]));
    mesh.zrotation = glm::rotate(rot, glm::radians(mesh.p[15]), glm::vec3(mesh.p[16], mesh.p[17], mesh.p[18]));


    // move the object (x, y, z)
    mesh.translation = glm::translate(glm::vec3(mesh.p[19], mesh.p[20], mesh.p[21]));

    mesh.model = mesh.translation * mesh.xrotation * mesh.zrotation * mesh.yrotation * mesh.scale;

    mesh.gUVScale = glm::vec2(mesh.p[22], mesh.p[23]);		// scales the text


}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}

void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

// Template for creating a cube light
void UCreateLightMesh(GLightMesh& lightMesh)
{
    // Position and Color data
    GLfloat verts[] = {
        //Positions          //Normals
        // ------------------------------------------------------
        //Back Face          //Negative Z Normal  Texture Coords.
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        //Front Face         //Positive Z Normal
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        //Left Face          //Negative X Normal
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        //Right Face         //Positive X Normal
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        //Bottom Face        //Negative Y Normal
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        //Top Face           //Positive Y Normal
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    lightMesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &lightMesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(lightMesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &lightMesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, lightMesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}



