#include "Renderer.h"
#include "Scene.h"
#include "State.h"
#include "Control.h"
#include "FrameRateMonitor.h"
#include "InputHandler.h"
#include "UIManager.h"




int main() {
    int initial_width = 1920;
    int initial_height = 1080;
 
    // --------------------------------- create camera object ----------------------------------
    Camera * camera = new Camera(glm::vec3(0, 2, 5));
    // modify camera infos before render loop starts
    camera->MovementSpeed = 1.0f;
    camera->Front = glm::normalize(glm::vec3(0, 2, 0) - camera->Position);
    camera->ProcessMouseMovement(0, 0); // to update the right and up vector
    // -----------------------------------------------------------------------------------------
    // ---- create renderer, which is used for rendering ---------------------------------------
    Renderer renderer(initial_width, initial_height, camera);
    RenderContext * renderContext = renderer.context; // get the render context from the renderer
    // -----------------------------------------------------------------------------------------
    // ------------------------------ create Scene object --------------------------------------
    Scene Scene(renderContext,SCENE1); // create Scene object, which contains all the objects in the Scene
    std::cout<<"scene created"<<std::endl;
    // -----------------------------------------------------------------------------------------
    // ------------------------------ create camera controller object --------------------------
    // it is used to control the camera movement and rotation
    // it also update the GPU ray tracer manager's camera (CPU one is updated when new CPU ray tracing starts)
    CameraController * cameraController = new CameraController(camera); // create camera controller object
    // -----------------------------------------------------------------------------------------
    // ------------------------------ create State machine object ------------------------------
    ExampleStateMachine state_machine; // create state machine object, which contains all the states and transitions, and handles the state changes
    state_machine.print_state(); // initial state is Default render state
    // -----------------------------------------------------------------------------------------
    // ------------------------------ create Frame rate monitor object -------------------------
    FrameRateMonitor * frameRateMonitor = new FrameRateMonitor();
    renderer.frameRateMonitor = frameRateMonitor; // attach the frame rate monitor to the renderer for displaying fps
    // -------------- register user input callbacks and loop check functions -------------------
    // create input handler object, which contains the input handling functions
    InputHandler * inputHandler = new InputHandler(renderer.window); 
    // set the input handling functions via callbacks and lambda functions
    inputHandler->setMouseMovementCallback([cameraController](double xpos, double ypos) {
        cameraController->rotateCamera(xpos, ypos);
    });
    inputHandler->setScrollCallback([cameraController](double yoffset) {
        cameraController->adjustfov(yoffset);
    });
    inputHandler->setFramebufferSizeCallback([&renderer](int width, int height) {
        renderer.resize(width, height);
    });
    // set the keyboard action execution function in while loop update (it will be called in the main loop other than callbacks)
    inputHandler->setKeyboardActionExecution([&renderer, &state_machine, cameraController,frameRateMonitor]() {
        float deltaTime = frameRateMonitor->getFrameDeltaTime();
        GLFWwindow * window = renderer.window;
        keyboardActions(&state_machine, cameraController, deltaTime, window);
    });
    // ----------------create UI manager object for managing the user interface-----------------
    UIManager uiManager;
    // very important to initialize imgui after you register the callbacks, otherwise imgui's callbacks will be overwritten
    uiManager.Initialize(renderer.window); // initialize the UI manager, it will also initialize the ImGui library
    // ------------------------------ create log window object ---------------------------------
    bool enable_scene_tick = false; // enable/disable ticking of scene objects
    LogWindow * logWindow = new LogWindow(frameRateMonitor, camera, &enable_scene_tick);
    DestructiveCSSceneObject * destructiveCSSyetem = nullptr;
    // try find the DestructiveCSSceneObject in the scene objects
    for (SceneObject * sceneObject : Scene.sceneObjects) {
        DestructiveCSSceneObject * destructiveCSSceneObject = dynamic_cast<DestructiveCSSceneObject*>(sceneObject);
        if (destructiveCSSceneObject) {
            destructiveCSSyetem = destructiveCSSceneObject;
            break;
        }
    }
    if (!destructiveCSSyetem) {
        std::cerr << "Error: [main] failed to find DestructiveCSSceneObject in the scene objects" << std::endl;
    }

    DestructiveCSUI_left * destructiveCSUI_left = new DestructiveCSUI_left(destructiveCSSyetem);
    uiManager.RegisterWindow(logWindow); // register the log window to the UI manager
    uiManager.RegisterWindow(destructiveCSUI_left);
    // -----------------------------------------------------------------------------------------
    // ------------------- get the list of scene objects from the scene ------------------------
    // they will be ticked in the main loop (some will also be rendered in the rendering loop)
    std::vector<SceneObject*> & sceneObjects = Scene.sceneObjects;
    
    // main loop
    while (!glfwWindowShouldClose(renderer.window))
    {
        // -------------------------- logic part of the main loop --------------------------------
        // update the frame rate monitor
        frameRateMonitor->update();
        float deltaTime = frameRateMonitor->getFrameDeltaTime();
        // update the state machine
        std::string last_state = state_machine.get_current_state()->name;
        state_machine.update();
        std::string current_state = state_machine.get_current_state()->name;
        // print the current state
        if (last_state != current_state) {
            state_machine.print_state();
        }
        // process keyboard input (checking all the keys every frame)
        inputHandler->processKeyboardInput(); // process input will be disabled in ray tracing state by disabling renderer's keyboard input


        // Tick all the scene objects
        if (enable_scene_tick){
            for (SceneObject * sceneObject : sceneObjects) {
                sceneObject->Tick(deltaTime);
            }
        }


        // -----------------------------------------------------------------------------------------

        // -------------------------- rendering part of the main loop ------------------------------

        bool draw_imgui = true;
        renderer.render(Scene,draw_imgui); // render the scene using openGL rasterization pipeline

        // draw UIs
        if (draw_imgui) {
            uiManager.enableAllWindows();
        }
        else {
            uiManager.disableAllWindows();
        }
        uiManager.Render();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        renderer.swap_buffers();
        renderer.poll_events();
        
        // -----------------------------------------------------------------------------------------

	}

    
    // imgui: terminate, clearing all previously allocated imgui resources.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();

     
    return 0;
}


