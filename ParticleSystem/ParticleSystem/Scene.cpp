//--------------------------------------------------------------------------------------
// Scene geometry and layout preparation
// Scene rendering & update
//--------------------------------------------------------------------------------------

#include "Scene.h"
#include "Mesh.h"
#include "Model.h"
#include "Camera.h"
#include "State.h"
#include "Shader.h"
#include "Input.h"
#include "Common.h"

#include "CVector2.h" 
#include "CVector3.h" 
#include "CMatrix4x4.h"
#include "MathHelpers.h"     // Helper functions for maths
#include "GraphicsHelpers.h" // Helper functions to unclutter the code here
#include "ColourRGBA.h" 

#include <sstream>
#include <memory>
#include <array>


//--------------------------------------------------------------------------------------
// Scene Data
//--------------------------------------------------------------------------------------

// Constants controlling speed of movement/rotation (measured in units per second because we're using frame time)
const float ROTATION_SPEED = 2.0f;  // 2 radians per second for rotation
const float MOVEMENT_SPEED = 50.0f; // 50 units per second for movement (what a unit of length is depends on 3D model - i.e. an artist decision usually)

// Lock FPS to monitor refresh rate, which will typically set it to 60fps. Press 'p' to toggle to full fps
bool lockFPS = false;


// Meshes, models and cameras, same meaning as TL-Engine. Meshes prepared in InitGeometry function, Models & camera in InitScene
Mesh* gStarsMesh;
Mesh* gGroundMesh;
Mesh* gLightMesh;

Model* gStars;
Model* gGround;

Camera* gCamera;


// Store lights in an array in this exercise
const int NUM_LIGHTS = 2;
struct Light
{
    Model*   model;
    CVector3 colour;
    float    strength;
};
Light gLights[NUM_LIGHTS]; 


// Additional light information
CVector3 gAmbientColour = { 0.3f, 0.3f, 0.4f }; // Background level of light (slightly bluish to match the far background, which is dark blue)
float    gSpecularPower = 256; // Specular power controls shininess - same for all models in this app

ColourRGBA gBackgroundColor = { 0.3f, 0.3f, 0.4f, 1.0f };

// Variables controlling light1's orbiting of the cube
const float gLightOrbit = 20.0f;
const float gLightOrbitSpeed = 0.7f;



//*****************************************************************************
// Particle Data
//*****************************************************************************

const int NumParticles = 100;

// The particles are going to be rendered in one draw call as a point list. The points will be expanded to quads in
// the geometry shader. This point list needs to be initialised in a vertex buffer, and will be updated using stream out.
// So the layout of the vertex (particle) data needs to be specified in three ways: for our use in C++, for the creation
// of the vertex buffer, and to indicate what data that will be updated using the stream output stage

// C++ data structure for a particle. Contains both rendering and update information. Contrast this with the instancing
// lab, where there were two seperate structures. The GPU will do all the work in this example
struct Particle
{
    CVector3 position;
    CVector3 velocity;
	float    life;
};

// An array of element descriptions to create the vertex buffer, *** This must match the Particle structure ***
// Contents explained using the third line below as an example: that line indicates that 24 bytes into each
// vertex (into the particle structure) is a "life" value which is a single float
D3D11_INPUT_ELEMENT_DESC ParticleElts[] =
{
	// Semantic  &  Index,   Type (eg. 1st one is float3),  Slot,   Byte Offset,  Instancing or not (not here), --"--
	{ "position",   0,       DXGI_FORMAT_R32G32B32_FLOAT,   0,      0,            D3D11_INPUT_PER_VERTEX_DATA,    0 },
	{ "velocity",   0,       DXGI_FORMAT_R32G32B32_FLOAT,   0,      12,           D3D11_INPUT_PER_VERTEX_DATA,    0 },
	{ "life",       0,       DXGI_FORMAT_R32_FLOAT,         0,      24,           D3D11_INPUT_PER_VERTEX_DATA,    0 },
};
const unsigned int NumParticleElts = sizeof(ParticleElts) / sizeof(D3D11_INPUT_ELEMENT_DESC);

// Vertex layout and buffers described by the above
// We hold two buffers of particles on the GPU. When updating we read from one and write to the other, this is a
// requirement of stream-out usage because you can't write back to the same buffer you are reading from.
// A common technique in many areas of programming - "double buffering"
ID3D11InputLayout* ParticleLayout;
ID3D11Buffer* ParticleBufferFrom;
ID3D11Buffer* ParticleBufferTo;


// This structure defines the data that will be output from the stream out stage. This array indicates which
// outputs of the vertex or geometry shader will be streamed back into GPU memory. Again, in this case the structure
// below must match the SParticle structure above (although more complex stream out arrangements are possible)
D3D11_SO_DECLARATION_ENTRY ParticleStreamOutElts[] =
{
	// Stream number (always 0 here); semantic name & index; start component; component count; output slot (always 0 here)
	{ 0, "position", 0,   0, 3,   0 }, // Output first 3 components of "position" - i.e. position, 3 floats (xyz)
	{ 0, "velocity", 0,   0, 3,   0 }, // Output first 3 components of "velocity" - i.e. velocity, 3 floats (xyz)
	{ 0, "life",     0,   0, 1,   0 }, // Output first component of "life"        - i.e. life, 1 float
};

//*****************************************************************************


//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
// Variables sent over to the GPU each frame
// The structures are now in Common.h
// IMPORTANT: Any new data you add in C++ code (CPU-side) is not automatically available to the GPU
//            Anything the shaders need (per-frame or per-model) needs to be sent via a constant buffer

PerFrameConstants gPerFrameConstants;      // The constants that need to be sent to the GPU each frame (see common.h for structure)
ID3D11Buffer*     gPerFrameConstantBuffer; // The GPU buffer that will recieve the constants above

PerModelConstants gPerModelConstants;      // As above, but constant that change per-model (e.g. world matrix)
ID3D11Buffer*     gPerModelConstantBuffer; // --"--



//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// DirectX objects controlling textures used in this lab
ID3D11Resource*           gStarsDiffuseSpecularMap    = nullptr;
ID3D11ShaderResourceView* gStarsDiffuseSpecularMapSRV = nullptr;

ID3D11Resource*           gGroundDiffuseSpecularMap    = nullptr;
ID3D11ShaderResourceView* gGroundDiffuseSpecularMapSRV = nullptr;

ID3D11Resource*           gLightDiffuseMap    = nullptr;
ID3D11ShaderResourceView* gLightDiffuseMapSRV = nullptr;



//--------------------------------------------------------------------------------------
// Initialise scene geometry, constant buffers and states
//--------------------------------------------------------------------------------------

// Prepare the geometry required for the scene
// Returns true on success
bool InitGeometry()
{
	////--------------- Load meshes ---------------////
	
	// Load mesh geometry data, just like TL-Engine this doesn't create anything in the scene. Create a Model for that.
    try 
    {
        gStarsMesh  = new Mesh("Stars.x");
		gGroundMesh = new Mesh("Ground.x");
		gLightMesh  = new Mesh("Light.x");
    }
    catch (std::runtime_error e)  // Constructors cannot return error messages so use exceptions to catch mesh errors (fairly standard approach this)
    {
        gLastError = e.what(); // This picks up the error message put in the exception (see Mesh.cpp)
        return false;
    }


	////--------------- Load / prepare textures & GPU states ---------------////

	// Load textures and create DirectX objects for them
	// The LoadTexture function requires you to pass a ID3D11Resource* (e.g. &gCubeDiffuseMap), which manages the GPU memory for the
	// texture and also a ID3D11ShaderResourceView* (e.g. &gCubeDiffuseMapSRV), which allows us to use the texture in shaders
	// The function will fill in these pointers with usable data. The variables used here are globals found near the top of the file.
	if (!LoadTexture("Stars.jpg",               &gStarsDiffuseSpecularMap,  &gStarsDiffuseSpecularMapSRV) ||
		!LoadTexture("WoodDiffuseSpecular.dds", &gGroundDiffuseSpecularMap, &gGroundDiffuseSpecularMapSRV) ||
		!LoadTexture("Flare.jpg",               &gLightDiffuseMap,          &gLightDiffuseMapSRV))
	{
		gLastError = "Error loading textures";
		return false;
	}


	// Create all filtering modes, blending modes etc. used by the app (see State.cpp/.h)
	if (!CreateStates())
	{
		gLastError = "Error creating states";
		return false;
	}


	////--------------- Prepare shaders and constant buffers to communicate with them ---------------////

	// Load the shaders required for the geometry we will use (see Shader.cpp / .h)
    if (!LoadShaders())
    {
        gLastError = "Error loading shaders";
        return false;
    }
	// Load the special stream-out geometry shader here since it needs access to data from this file
	gParticleUpdateShader = LoadStreamOutGeometryShader("ParticleUpdate_gs", ParticleStreamOutElts, NumParticleElts, sizeof(Particle));
	if (gParticleUpdateShader == nullptr)
	{
		gLastError = "Error loading stream-out geometry shader";
		return false;
	}

    // Create GPU-side constant buffers to receive the gPerFrameConstants and gPerModelConstants structures above
    // These allow us to pass data from CPU to shaders such as lighting information or matrices
    // See the comments above where these variable are declared and also the UpdateScene function
    gPerFrameConstantBuffer = CreateConstantBuffer(sizeof(gPerFrameConstants));
    gPerModelConstantBuffer = CreateConstantBuffer(sizeof(gPerModelConstants));
    if (gPerFrameConstantBuffer == nullptr || gPerModelConstantBuffer == nullptr)
    {
        gLastError = "Error creating constant buffers";
        return false;
    }


	return true;
}


// Prepare the scene
// Returns true on success
bool InitScene()
{
    ////--------------- Set up scene ---------------////

	gStars  = new Model(gStarsMesh);
    gGround = new Model(gGroundMesh);

	// Initial positions
	gStars->SetScale(8000.0f);


    // Light set-up - using an array this time
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        gLights[i].model = new Model(gLightMesh);
    }
    
	gLights[0].colour = { 0.8f, 0.8f, 1.0f };
	gLights[0].strength = 10;
	gLights[0].model->SetPosition({ 30, 10, 0 });
	gLights[0].model->SetScale(pow(gLights[0].strength, 0.7f)); // Convert light strength into a nice value for the scale of the light - equation is ad-hoc.

	gLights[1].colour = { 1.0f, 0.8f, 0.2f };
	gLights[1].strength = 40;
	gLights[1].model->SetPosition({ -70, 30, 100 });
	gLights[1].model->SetScale(pow(gLights[1].strength, 0.7f));


    ////--------------- Set up camera ---------------////

    gCamera = new Camera();
    gCamera->SetPosition({ -0, 50, -200 });
    gCamera->SetRotation({ 0.0f, 0.0f, 0.0f });


	
	//*************************************************************************
	// Initialise particles

	// Create the vertex layout for the particle data structure declared near the top of the file. This step creates
	// an object (ParticleLayout) that is used to describe to the GPU the data used for each particle
	auto signature = CreateSignatureForVertexLayout(ParticleElts, NumParticleElts);
	gD3DDevice->CreateInputLayout(ParticleElts, NumParticleElts, signature->GetBufferPointer(), signature->GetBufferSize(), &ParticleLayout);


	// Set up some initial particle data. This will be transferred to a GPU vertex buffer and discarded from the CPU side
	// Note the unusual use of a unique_ptr to an array. This has better performance than std::vector when initialising very large amounts
	auto particles = std::make_unique<Particle[]>(NumParticles);
	for (int p = 0; p < NumParticles; ++p)
	{
		particles[p].position = { Random(-10.0f, 10.0f), Random(-50.0f, 50.0f), Random(-10.0f, 10.0f) };
		particles[p].velocity = { Random(-20.0f, 20.0f), Random(0.0f, 60.0f),   Random(-20.0f, 20.0f) };
		particles[p].life = (5.0f * p) / NumParticles;
	}


	// Create / initialise the vertex buffers to hold the particles
	// Need to create two vertex buffers, because we cannot stream output to the same buffer than we are reading
	// the input from. Instead we input from one buffer and stream out to another, and swap the buffers each frame
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT; // Vertex buffer using stream output
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;                  // Default buffer use is read / write from GPU only
	bufferDesc.ByteWidth = NumParticles * sizeof(Particle); // Buffer size in bytes
	bufferDesc.CPUAccessFlags = 0;   // Indicates that CPU won't access this buffer at all after creation
	bufferDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA initData; // Initial data (only needed in one of the buffers to start)
	initData.pSysMem = &particles[0];
	if (FAILED(gD3DDevice->CreateBuffer(&bufferDesc, &initData, &ParticleBufferFrom)) ||
		FAILED(gD3DDevice->CreateBuffer(&bufferDesc, 0, &ParticleBufferTo)))
	{
		gLastError = "Error creating particle vertex buffers";
		return false;
	}

	//*************************************************************************
	
	return true;
}


// Release the geometry and scene resources created above
void ReleaseResources()
{
    ReleaseStates();

	if (ParticleLayout)      ParticleLayout    ->Release();
	if (ParticleBufferFrom)  ParticleBufferFrom->Release();
	if (ParticleBufferTo)    ParticleBufferTo  ->Release();

    if (gLightDiffuseMapSRV)           gLightDiffuseMapSRV         ->Release();
    if (gLightDiffuseMap)              gLightDiffuseMap            ->Release();
    if (gGroundDiffuseSpecularMapSRV)  gGroundDiffuseSpecularMapSRV->Release();
    if (gGroundDiffuseSpecularMap)     gGroundDiffuseSpecularMap   ->Release();
	if (gStarsDiffuseSpecularMapSRV)   gStarsDiffuseSpecularMapSRV ->Release();
	if (gStarsDiffuseSpecularMap)      gStarsDiffuseSpecularMap    ->Release();

    if (gPerModelConstantBuffer)  gPerModelConstantBuffer->Release();
    if (gPerFrameConstantBuffer)  gPerFrameConstantBuffer->Release();

    ReleaseShaders();

    // See note in InitGeometry about why we're not using unique_ptr and having to manually delete
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        delete gLights[i].model;  gLights[i].model = nullptr;
    }
    delete gCamera;  gCamera = nullptr;
    delete gGround;  gGround = nullptr;
	delete gStars;   gStars  = nullptr;

    delete gLightMesh;   gLightMesh  = nullptr;
    delete gGroundMesh;  gGroundMesh = nullptr;
	delete gStarsMesh;   gStarsMesh  = nullptr;
}



//--------------------------------------------------------------------------------------
// Scene Rendering
//--------------------------------------------------------------------------------------

// Render everything in the scene from the given camera
void RenderSceneFromCamera(Camera* camera)
{
    // Set camera matrices in the constant buffer and send over to GPU
	gPerFrameConstants.cameraMatrix         = camera->WorldMatrix();
	gPerFrameConstants.viewMatrix           = camera->ViewMatrix();
    gPerFrameConstants.projectionMatrix     = camera->ProjectionMatrix();
    gPerFrameConstants.viewProjectionMatrix = camera->ViewProjectionMatrix();
    UpdateConstantBuffer(gPerFrameConstantBuffer, gPerFrameConstants);

    // Indicate that the constant buffer we just updated is for use in the vertex shader (VS), geometry shader (GS) and pixel shader (PS)
    gD3DContext->VSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer); // First parameter must match constant buffer number in the shader 
	gD3DContext->GSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);
	gD3DContext->PSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);

	gD3DContext->PSSetShader(gPixelLightingPixelShader, nullptr, 0);


    ////--------------- Render ordinary models ---------------///

    // Select which shaders to use next
    gD3DContext->VSSetShader(gPixelLightingVertexShader, nullptr, 0); // Only need to change the vertex shader from skinning
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  ////// Switch off geometry shader when not using it (pass nullptr for first parameter)
	gD3DContext->PSSetShader(gPixelLightingPixelShader, nullptr, 0);

	// States - no blending, normal depth buffer and back-face culling (standard set-up for opaque models)
	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gUseDepthBufferState, 0);
	gD3DContext->RSSetState(gCullBackState);

	// Render lit models, only change textures for each onee
	gD3DContext->PSSetShaderResources(0, 1, &gGroundDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);

	gGround->Render();



	////--------------- Render sky ---------------////

	// Select which shaders to use next
	gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gTintedTexturePixelShader, nullptr, 0);

	// Using a pixel shader that tints the texture - don't need a tint on the sky so set it to white
	gPerModelConstants.objectColour = { 1, 1, 1 }; 

	// No change to states from previous section

	// Render sky
	gD3DContext->PSSetShaderResources(0, 1, &gStarsDiffuseSpecularMapSRV);
	gStars->Render();



	////--------------- Render lights ---------------////

    // Select which shaders to use next (actually same as before, so we could skip this)
    gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
    gD3DContext->PSSetShader(gTintedTexturePixelShader,   nullptr, 0);

    // Select the texture and sampler to use in the pixel shader
    gD3DContext->PSSetShaderResources(0, 1, &gLightDiffuseMapSRV); // First parameter must match texture slot number in the shaer

    // States - additive blending, read-only depth buffer and no culling (standard set-up for blending)
    gD3DContext->OMSetBlendState(gAdditiveBlendingState, nullptr, 0xffffff);
    gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
    gD3DContext->RSSetState(gCullNoneState);

    // Render all the lights in the array
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        gPerModelConstants.objectColour = gLights[i].colour; // Set any per-model constants apart from the world matrix just before calling render (light colour here)
        gLights[i].model->Render();
    }


	//*************************************************************************
	// Particle rendering/update

	//////////////////////////
	// Rendering

	// Set shaders for particle rendering - the vertex shader just passes the data to the 
	// geometry shader, which generates a camera-facing 2D quad from the particle world position 
	// The pixel shader is very simple and just draws a tinted texture for each particle
	gD3DContext->VSSetShader(gParticlePassThruVertexShader, nullptr, 0);
	gD3DContext->GSSetShader(gParticleRenderShader, nullptr, 0);
	gD3DContext->PSSetShader(gTintedTexturePixelShader, nullptr, 0);

	// Select the texture and sampler to use in the pixel shader
	gD3DContext->PSSetShaderResources(0, 1, &gLightDiffuseMapSRV); // Will use the light flare sprite for the particles

	// States - additive blending, read-only depth buffer and no culling (standard set-up for blending)
	gD3DContext->OMSetBlendState(gAdditiveBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
	gD3DContext->RSSetState(gCullNoneState);

	// Set the colour of all the particles to light blue
	gPerModelConstants.objectColour = { 0.5f, 0.75f, 1 }; 
	UpdateConstantBuffer(gPerModelConstantBuffer, gPerModelConstants);

	// Set up particle vertex buffer / layout
	unsigned int particleVertexSize = sizeof(Particle);
	unsigned int offset = 0;
	gD3DContext->IASetVertexBuffers(0, 1, &ParticleBufferFrom, &particleVertexSize, &offset);
	gD3DContext->IASetInputLayout(ParticleLayout);

	// Indicate that this is a point list and render it
	gD3DContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	gD3DContext->Draw(NumParticles, 0);



	//////////////////////////
	// Update

	// Shaders for particle update. Again the vertex shader does nothing, and this time we explicitly
	// switch off rendering by setting no pixel shader (also need to switch off depth buffer)
	// All the particle update work is done by the geometry shader
	gD3DContext->VSSetShader(gParticlePassThruVertexShader, nullptr, 0);
	gD3DContext->GSSetShader(gParticleUpdateShader, nullptr, 0);
	gD3DContext->PSSetShader(nullptr, nullptr, 0);
	gD3DContext->OMSetDepthStencilState(gNoDepthBufferState, 0);

	// The updated particle data is streamed back to GPU memory, but we can't write to the same buffer
	// we are reading from, so it goes to a second (identical) particle buffer
	gD3DContext->SOSetTargets(1, &ParticleBufferTo, &offset);

	// Calling draw doesn't actually draw anything, just performs the particle update
	gD3DContext->Draw(NumParticles, 0);

	// Switch off stream output
	ID3D11Buffer* nullBuffer = 0;
	gD3DContext->SOSetTargets(1, &nullBuffer, &offset);


	// Swap the two particle buffers
	std::swap(ParticleBufferFrom, ParticleBufferTo);
	
	//*************************************************************************
}



// Rendering the scene
void RenderScene(float frameTime)
{
    //// Common settings ////

    // Set up the light information in the constant buffer
    // Don't send to the GPU yet, the function RenderSceneFromCamera will do that
    gPerFrameConstants.light1Colour   = gLights[0].colour * gLights[0].strength;
    gPerFrameConstants.light1Position = gLights[0].model->Position();
    gPerFrameConstants.light2Colour   = gLights[1].colour * gLights[1].strength;
    gPerFrameConstants.light2Position = gLights[1].model->Position();

    gPerFrameConstants.ambientColour  = gAmbientColour;
    gPerFrameConstants.specularPower  = gSpecularPower;
    gPerFrameConstants.cameraPosition = gCamera->Position();
	
	gPerFrameConstants.frameTime      = frameTime;



    ////--------------- Main scene rendering ---------------////

    // Set the back buffer as the target for rendering and select the main depth buffer.
    // When finished the back buffer is sent to the "front buffer" - which is the monitor.
    gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);

    // Clear the back buffer to a fixed colour and the depth buffer to the far distance
    gD3DContext->ClearRenderTargetView(gBackBufferRenderTarget, &gBackgroundColor.r);
    gD3DContext->ClearDepthStencilView(gDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Setup the viewport to the size of the main window
    D3D11_VIEWPORT vp;
    vp.Width  = static_cast<FLOAT>(gViewportWidth);
    vp.Height = static_cast<FLOAT>(gViewportHeight);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    gD3DContext->RSSetViewports(1, &vp);

    // Render the scene from the main camera
    RenderSceneFromCamera(gCamera);


    ////--------------- Scene completion ---------------////

    // When drawing to the off-screen back buffer is complete, we "present" the image to the front buffer (the screen)
	// Set first parameter to 1 to lock to vsync
    gSwapChain->Present(lockFPS ? 1 : 0, 0);
}


//--------------------------------------------------------------------------------------
// Scene Update
//--------------------------------------------------------------------------------------

// Update models and camera. frameTime is the time passed since the last frame
void UpdateScene(float frameTime)
{
    // Orbit one light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float rotate = 0.0f;
    static bool go = false;
	gLights[0].model->SetPosition(CVector3{ cos(rotate) * gLightOrbit, 10, sin(rotate) * gLightOrbit });
    if (go)  rotate -= gLightOrbitSpeed * frameTime;
    if (KeyHit(Key_1))  go = !go;

	// Control camera (will update its view matrix)
	gCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D );

	// Toggle FPS limiting
	if (KeyHit(Key_P))  lockFPS = !lockFPS;

    // Show frame time / FPS in the window title //
    const float fpsUpdateTime = 0.5f; // How long between updates (in seconds)
    static float totalFrameTime = 0;
    static int frameCount = 0;
    totalFrameTime += frameTime;
    ++frameCount;
    if (totalFrameTime > fpsUpdateTime)
    {
        // Displays FPS rounded to nearest int, and frame time (more useful for developers) in milliseconds to 2 decimal places
        float avgFrameTime = totalFrameTime / frameCount;
        std::ostringstream frameTimeMs;
        frameTimeMs.precision(2);
        frameTimeMs << std::fixed << avgFrameTime * 1000;
        std::string windowTitle = "CO3303 Week 8: GPU Particle System - Frame Time: " + frameTimeMs.str() +
                                  "ms, FPS: " + std::to_string(static_cast<int>(1 / avgFrameTime + 0.5f));
        SetWindowTextA(gHWnd, windowTitle.c_str());
        totalFrameTime = 0;
        frameCount = 0;
    }
}
