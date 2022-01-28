#pragma comment(lib, "Winmm.lib")

#include "common.hpp"


#define reportNum 100

/* Imports */
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <thread>
#include <imgui.hpp>
#include "mesh.hpp"

#include <shaderdemo.hpp>
#include <advanced_camera.hpp>
#include <modeldemo.hpp>
#include <Particle2.hpp>
#include <Interaction.hpp>
//initialize particle
Particle *particles;
int n=10;//particle number
double timeStep=0.5f;//<1.0
//particle box size
double xleft=-1.15f;//<0
double xright=1.15f;
double yup=0.55f;
double ydown=-2.0f;//<0
double restitution=0.8;//<1.0
//
double PI = 3.1415f;
// GUI
GLFWwindow* window;
bool shouldClose = false;



inline void drawParticle(Model obj,const Shader &sha,Particle p,glm::mat4 &viewmatrix){
    
    glm::mat4 view = viewmatrix;
    if(p.getState() == alive){
        double *pos;
        pos = p.getPosition();

        Shader shader=sha;
        glm::mat4 model=glm::mat4(1.0f);
        glm::mat4 modelmatrix= transpose(view);
        model[0][0]=modelmatrix[0][0];
        model[0][1]=modelmatrix[0][1];
        model[0][2]=modelmatrix[0][2];
        model[1][0]=modelmatrix[1][0];
        model[1][1]=modelmatrix[1][1];
        model[1][2]=modelmatrix[1][2];
        model[2][0]=modelmatrix[2][0];
        model[2][1]=modelmatrix[2][1];
        model[2][2]=modelmatrix[2][2];
        // to disable the rotation part in the model view matrix
        model = glm::translate(model, glm::vec3(pos[0], pos[1], pos[2]));
        
        model = glm::rotate(model, 180.63f, glm::vec3(0,1,0));
        
        model = glm::scale(model, glm::vec3(0.09f));//size
        
        shader.setMat4("model", model);
        obj.Draw(shader);
        
        
    }
}

static void simThread(Interaction interactions){
    while(!shouldClose){
        interactions.interact();
        // Update Particle Position
        for (int i = 0; i < n; i++){
            particles[i].step(timeStep,xleft,xright,yup,ydown,restitution);
        }
    }
}






float*
load_texture_data(std::string filename, int* width, int* height) {
    int channels;
    unsigned char* file_data = stbi_load(filename.c_str(), width, height, &channels, 3);
    
    int w = *width;
    int h = *height;
    
    float* data = new float[4 * w * h];
    for (int j=0; j < h; ++j) {
        for (int i=0; i < w; ++i) {
            data[j*w*4 + i*4 + 0] = static_cast<float>(file_data[j*w*3 + i*3 + 0]) / 255;
            data[j*w*4 + i*4 + 1] = static_cast<float>(file_data[j*w*3 + i*3 + 1]) / 255;
            data[j*w*4 + i*4 + 2] = static_cast<float>(file_data[j*w*3 + i*3 + 2]) / 255;
            data[j*w*4 + i*4 + 3] = 1.f;
        }
    }
    
    delete [] file_data;
    
    return data;
}
unsigned int
create_texture_rgba32f(int width, int height, float* data) {
    unsigned int handle;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, width, height);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, data);
    
    return handle;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);
void renderScene(Shader *shaderarr[],Model* obj[]);
void renderDepth(const Shader &shader,Model* obj[]);


void renderCube();
void renderQuad();
void rendermodel(geometry model);
void renderPlane();
void renderWaves(unsigned int shaderProgram);





// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
bool shadows = true;
bool shadowsKeyPressed = false;
bool hdr = true;
bool hdrKeyPressed = false;
int hdr_algo_selection = 5;
float lightColor = 1.0f;
float exposure = 0.2f;

// camera1
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH ;
float lastY = (float)SCR_HEIGHT ;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;





int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "demo", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    // build and compile shaders
    // -------------------------
    Shader simpleDepthShader("point_shadows_depth.vert", "point_shadows_depth.frag", "point_shadows_depth.geom");
    //for depth
    //------------
    //actual shader:
    Shader shader("point_shadows_water.vert", "point_shadows_water.frag");
    
    
    Shader watershader("water.vert", "water.frag","water.geom");

    //for HDR
    Shader hdrShader("hdr.vert", "hdr.frag");
    
    
    Shader* shaderarr[100];
    shaderarr[0]=&shader;
    //models
    
    
    
   
    Model obj1(MODEL_ROOT+"girl/girl.obj");
    Model obj2(MODEL_ROOT+"planet/planet.obj");
    Model obj3(MODEL_ROOT + "water/waterplane.obj");
    Model obj(MODEL_ROOT+"glitter/glitter.obj");
    Model *geoarr[100];
    geoarr[0]=&obj1;
    geoarr[1]=&obj2;
    geoarr[2] = &obj3;
    
    
    // load texture
    // -------------
    int image_width, image_height;
    
    float* image_tex_data = load_texture_data(DATA_ROOT + "blue_painted_planks.png", &image_width, &image_height);
    
    unsigned int image_tex = create_texture_rgba32f(image_width, image_height, image_tex_data);


    //WaterFBO and watertexture waveradius

    float waveradiuslimit = 1.0f;
    float waveradius = 0.0f;
    float period = 0.0f;

    int image2_width, image2_height;
    float* image2_tex_data = load_texture_data(MODEL_ROOT + "water/water.png", &image2_width, &image2_height);
    unsigned int watertexture = create_texture_rgba32f(image2_width, image2_height, image2_tex_data);
    
    const unsigned int WATER_WIDTH = 1024, WATER_HEIGHT = 1024;
    
    unsigned int waterFBO;
    glGenFramebuffers(1, &waterFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, waterFBO);
    
    unsigned int water;
    glGenTextures(1, &water);
    glBindTexture(GL_TEXTURE_2D, water);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WATER_WIDTH,WATER_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, water, 0);
    GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, DrawBuffers);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("Framebuffer is sick!");
    else
        printf("Framebuffer works!");
   
    //HdrFBO
    //---------------------------------
    unsigned int hdrFBO = 5;
    GLuint depthRBO = 6;
    GLuint colorBuffer = 7;
    glGenFramebuffers(1, &hdrFBO);
    // create floating point color buffer
    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLuint depthrenderbuffer;
    glGenRenderbuffers(1, &depthrenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    // attach buffers
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);



   //---------------------------
    
    
    // configure depth map FBO
    // -----------------------
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    // create depth cubemap texture
    unsigned int depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
   
    
    // shader configuration
    // --------------------
    shader.use();
    shader.setInt("diffuseTexture", 0);
    shader.setInt("depthMap", 1);
    shader.setInt("waterTexture",2);
    shader.setFloat("lColor",lightColor);
    shader.setBool("water", 0);

    hdrShader.use();
    hdrShader.setInt("hdrBuffer", 0);

    // lighting info
    // -------------
    glm::vec3 lightPos(5.0f, 0.0f, 5.0f);
    glm::vec3 lightPos2(-6.0f, 0.0f, -5.0f);
    
    //Particle generation-------------------------------
   
    srand(static_cast<unsigned>(time(0)));
    
    particles = (Particle *) malloc( sizeof(Particle) * n );
    for (int i = 0; i < n; i++){
        double x = (static_cast<float>(rand())/(static_cast<float>(RAND_MAX/2.0)))-1.0;
        double y = (static_cast<float>(rand())/(static_cast<float>(RAND_MAX/2.0)))-1.0;
        
        particles[i] = Particle(x, y, 0.0, 1.0, 0.0);//xyz mass radius
        particles[i].setVelocity(0.0, 6.0, 0.0);
        
    }
    
    // Initalise interaction class
    Interaction interactions = Interaction(particles, &n);
    // Rendering Loop and Simulation Thread
    std::thread worker = std::thread(simThread, interactions);
    

#ifdef WIN32
    std::string testwavroot = DATA_ROOT + "glossy.wav";
    PlaySound((LPCSTR)testwavroot.c_str(), NULL, SND_ASYNC);
#endif


    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // move light position over time
         lightPos.z = cos(glfwGetTime() /4.0 - 0.25) * 9.0;
         lightPos.x = sin(glfwGetTime() /4.0 - 0.25) * 9.0;
        // render
        // ------
 
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);




        //-1. render Watertexture to water
        glBindFramebuffer(GL_FRAMEBUFFER, waterFBO);
        glViewport(0, 0, 1024, 1024);
        glClear(GL_DEPTH_BUFFER_BIT);
        watershader.use();
        waveradius = currentFrame * 1000000.0f;
        waveradius = int(waveradius)%10000000;
        waveradius = waveradius * 0.000000018;
       
        watershader.setFloat("radius", waveradius);
        renderWaves(watershader.ID);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        
        // 0. create depth cubemap transformation matrices
        // -----------------------------------------------
        float near_plane = 1.0f;
        float far_plane  = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        
        //https://stackoverflow.com/questions/21830340/understanding-glmlookat
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
        
        
        
        // 1. render scene to depth cubemap
        // --------------------------------
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        simpleDepthShader.use();
        for (unsigned int i = 0; i < shadowTransforms.size(); ++i)
            simpleDepthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        simpleDepthShader.setFloat("far_plane", far_plane);
        simpleDepthShader.setVec3("lightPos", lightPos);
        renderDepth(simpleDepthShader,geoarr);//render model depth with this shader
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        
        
        
        // 2. render scene as normal
        // -------------------------
      

        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

        //controls the render window size
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        //glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 view = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        //-------------camera----------------
        float radius = 10.0f; //distance from obj
        float camX   = sin(glfwGetTime()/4) * radius;
        float camZ   = cos(glfwGetTime()/4) * radius;
        view = glm::lookAt(glm::vec3(camX, 0.0f, camZ), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        //-------------camera----------------
        
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        // set lighting uniforms
        shader.setVec3("lightPos", lightPos);
        shader.setVec3("viewPos", camera.Position);
        shader.setInt("shadows", shadows); // enable/disable shadows by pressing 'SPACE'
        shader.setFloat("far_plane", far_plane);
        shader.setBool("water", 0);
        //particle rendering
        for (int i = 0; i < n; i++) {
            //Draw Particle
            drawParticle(obj, shader, particles[i], view);
        }

        // -------------------------------------------------------------------------------

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, image_tex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, water);
        
        renderScene(shaderarr,geoarr);

        
        // 3. render 2D quad HDR -> LDR
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        hdrShader.setInt("hdr", hdr);
        hdrShader.setInt("hdr_algo_selection", hdr_algo_selection);
        hdrShader.setFloat("exposure", exposure);
        renderQuad();
        


        glfwSwapBuffers(window);
        glfwPollEvents();

    }
    
    std::cout << "Ending Simulation..." << std::endl;
    std::cout << "\tStopping threads..." << std::endl;
    shouldClose = true;
    worker.join();
    std::cout << "\tFreeing memory..." << std::endl;
    free(particles);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// render Depth map
void renderDepth(const Shader &shader, Model* geoarr[])
{
    Shader sha =shader;
    
    // room cube
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(10.0f));
    sha.setMat4("model", model);
    glDisable(GL_CULL_FACE); // note that we disable culling here since we render 'inside' the cube instead of the usual 'outside' which throws off the normal culling methods.
    sha.setInt("reverse_normals", 1); // A small little hack to invert normals when drawing cube from the inside so lighting still works.
    renderCube();
    sha.setInt("reverse_normals", 0); // and of course disable it
    glEnable(GL_CULL_FACE);


    // cubes
    Model model1 = *geoarr[0];
    //add model1
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
    model = glm::rotate(model, glm::radians(0.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(0.5f));
    sha.setMat4("model", model);
    model1.Draw(sha);

    model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
    model = glm::scale(model, glm::vec3(20.0f));
    sha.setBool("water", 1);
    sha.setMat4("model", model);
    renderPlane();
    sha.setBool("water", 0);
   



    
}


// renders the 3D scene with different shader
// --------------------
void renderScene(Shader * shaderarr[], Model* geoarr[])
{
    Shader shader= *shaderarr[0];
    // room cube
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(10.0f));
    shader.setMat4("model", model);
    glDisable(GL_CULL_FACE); // note that we disable culling here since we render 'inside' the cube instead of the usual 'outside' which throws off the normal culling methods.
    shader.setInt("reverse_normals", 1); // A small little hack to invert normals when drawing cube from the inside so lighting still works.
    renderCube();
    shader.setInt("reverse_normals", 0); // and of course disable it
    glEnable(GL_CULL_FACE);
    // cubes

    
    Model model1 = *geoarr[0];
    //add model1
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
    model = glm::rotate(model, glm::radians(0.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    model1.Draw(shader);
   
    model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
    model = glm::scale(model, glm::vec3(20.0f));
    shader.setBool("water", 1);

    shader.setMat4("model", model);
    renderPlane();
    shader.setBool("water", 0);

}


// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;

void renderCube()
{
    
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
}
unsigned int planeVAO = 0;
unsigned int planeVBO = 0;
unsigned int planeIBO = 0;
void renderPlane()
{

    // initialize (if necessary)
    if (planeVAO == 0)
    {
        float vertices[] = {
        -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
         1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
         -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
         1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
         -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
         1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f
        };
        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

         // link vertex attributes
        glBindVertexArray(planeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

        glBindVertexArray(0);

    }
    glBindVertexArray(planeVAO);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}

void renderWaves(unsigned int shaderProgram) {


    // vertex data
    GLuint vbo;
    glGenBuffers(1, &vbo);

    float points[] = {
        //  Coordinates  Color             Sides
            -0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
             0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
             0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
            -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 32.0f,//, 0.4f#
           -0.25f,  0.25f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
             0.25f,  0.25f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
             0.25f, -0.25f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
            -0.25f, -0.25f, 1.0f, 1.0f, 1.0f, 32.0f,//, 0.4f
            -0.75f,  0.75f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
             0.75f,  0.75f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
             0.75f, -0.75f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
            -0.75f, -0.75f, 1.0f, 1.0f, 1.0f, 32.0f,//, 0.4f
            -0.25f,  0.75f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
             0.25f,  0.75f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
             0.25f, -0.75f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
            -0.25f, -0.75f, 1.0f, 1.0f, 1.0f, 32.0f,//, 0.4f
            -0.75f,  0.25f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
             0.75f,  0.25f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
             0.75f, -0.25f, 1.0f, 1.0f, 1.0f, 32.0f, //0.4f,
            -0.75f, -0.25f, 1.0f, 1.0f, 1.0f, 32.0f//, 0.4f
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    GLuint vao;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLint posAttrib = glGetAttribLocation(shaderProgram, "pos");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);

    GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE,
        6 * sizeof(float), (void*)(2 * sizeof(float)));
    GLint sidesAttrib = glGetAttribLocation(shaderProgram, "sides");
    glEnableVertexAttribArray(sidesAttrib);
    glVertexAttribPointer(sidesAttrib, 1, GL_FLOAT, GL_FALSE,
        6 * sizeof(float), (void*)(5 * sizeof(float)));

    /*
    GLint radiusAttrib = glGetAttribLocation(shaderProgram, "radius");
    glEnableVertexAttribArray(radiusAttrib);
    glVertexAttribPointer(radiusAttrib, 1, GL_FLOAT, GL_FALSE,
       7 * sizeof(float), (void*)(6 * sizeof(float)));
    */

    glClearColor(0.011904f, 0.673015f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_POINTS, 0, 20);
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !shadowsKeyPressed)
    {
        shadows = !shadows;
        shadowsKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        shadowsKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
        hdr = !hdr;

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (exposure > 0.0f)
            exposure -= 0.001f;
        else
            exposure = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        exposure += 0.001f;
    }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        hdr_algo_selection = 1;

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        hdr_algo_selection = 2;

    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        hdr_algo_selection = 3;

    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        hdr_algo_selection = 4;

    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
        hdr_algo_selection = 5;

    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
        hdr_algo_selection = 6;

    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
        hdr_algo_selection = 7;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
