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
#include <algorithm>
#include <array>
#include <memory>


//--------------------------------------------------------------------------------------
// Scene Data
//--------------------------------------------------------------------------------------

// Constants controlling speed of movement/rotation (measured in units per second because we're using frame time)
const float ROTATION_SPEED = 1.5f;  // Radians per second for rotation
const float MOVEMENT_SPEED = 50.0f; // Units per second for movement (what a unit of length is depends on 3D model - i.e. an artist decision usually)

// Lock FPS to monitor refresh rate, which will typically set it to 60fps. Press 'p' to toggle to full fps
bool lockFPS = true;


// Meshes, models and cameras, same meaning as TL-Engine. Meshes prepared in InitGeometry function, Models & camera in InitScene
Mesh* gStarsMesh;
Mesh* gGroundMesh;
Mesh* gCubeMesh;
Mesh* gCrateMesh;
Mesh* gLightMesh;

Model* gStars;
Model* gGround;
Model* gCube;
Model* gCrate;

Camera* gCamera;

// Variables controlling light1's orbiting of the particle emitter
const float gCameraOrbitRadius = 60.0f;
const float gCameraOrbitSpeed = 1.2f;


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
const float gLightOrbitRadius = 20.0f;
const float gLightOrbitSpeed = 0.7f;



//*****************************************************************************
// Particle Data
//*****************************************************************************

//----
// Setup

const int NumParticles = 200;

// Particle system emitter location
CVector3 ParticleEmitterPos = {34, -2, -13};


//----
// Particle data

// C++ data structure for rendering a particle (stored as a single point)
struct ParticlePoint
{
    CVector3 position; // World position of particle, the geometry shader will expand it into a camera-facing quad
    float    alpha;    // Overall transparency of particle (the particle texture can also contain per-pixel transparency)
	float    scale;    // Size of the quad created by the geometry shader from the particle point
	float    rotation;  // Rotation of the quad created by the geometry shader
};

// An array of element descriptions to create the vertex buffer, *** This must match the Particle structure above ***
// Contents explained using the third line below as an example: that line indicates that 16 bytes into each
// vertex (into the particle structure) is a "scale" value which is a single float
D3D11_INPUT_ELEMENT_DESC ParticleElts[] =
{
	// Semantic  &  Index,   Type (eg. 1st one is float3),  Slot,   Byte Offset,  Instancing or not (not here), --"--
	{ "position",   0,       DXGI_FORMAT_R32G32B32_FLOAT,   0,      0,            D3D11_INPUT_PER_VERTEX_DATA,    0 },
	{ "alpha",      0,       DXGI_FORMAT_R32_FLOAT,         0,      12,           D3D11_INPUT_PER_VERTEX_DATA,    0 },
	{ "scale",      0,       DXGI_FORMAT_R32_FLOAT,         0,      16,           D3D11_INPUT_PER_VERTEX_DATA,    0 },
	{ "rotation",   0,       DXGI_FORMAT_R32_FLOAT,         0,      20,           D3D11_INPUT_PER_VERTEX_DATA,    0 },
};
const unsigned int NumParticleElts = sizeof(ParticleElts) / sizeof(D3D11_INPUT_ELEMENT_DESC);



// Data required to update a particle - used on the CPU only
struct ParticleUpdate
{
	CVector3 velocity;
    float    rotationSpeed;
};


// Arrays of particle data - note the use of std::array which is a more modern version of a fixed size array - you get
// various container class features such as access to std algorithms and use of range-based for loops etc.
std::array<ParticlePoint,  NumParticles>  ParticlePoints;  // Particle rendering data passed from CPU to GPU every frame as a vertex buffer
std::array<ParticleUpdate, NumParticles>  ParticleUpdates; // Particle update data - remains in CPU memory 



//----
// Particle sorting


// We want to sort the particles by depth each frame, but since sorting involves lots of data movement so it can be faster to sort
// references to the data rather than the data itself. So here we have a structure containing the index of a particle and the depth of that
// particle from camera. Then we store an array of this structure - one for each particle. This array is simple and quick to sort on depth.
// The indexes can then be used to find the sorted version of the original particles. Note that pointers could be used instead of indexes.
// This technique can be useful in other sorting situations where performance is important and the amount of per-object data may be large
struct SParticleDepth
{
	int   index;
	float depth;
};

std::array<SParticleDepth, NumParticles> ParticleDepths;



//----
// DirectX Data

// Vertex layout and buffer for the particles (rendering data only, we are not doing update on the GPU in this example)
ID3D11InputLayout* ParticleLayout;
ID3D11Buffer* ParticleVertexBuffer;

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
ID3D11Resource*           gStarsDiffuseSpecularMap     = nullptr;
ID3D11ShaderResourceView* gStarsDiffuseSpecularMapSRV  = nullptr;
ID3D11Resource*           gGroundDiffuseSpecularMap    = nullptr;
ID3D11ShaderResourceView* gGroundDiffuseSpecularMapSRV = nullptr;
ID3D11Resource*           gCrateDiffuseSpecularMap     = nullptr;
ID3D11ShaderResourceView* gCrateDiffuseSpecularMapSRV  = nullptr;
ID3D11Resource*           gCubeDiffuseSpecularMap      = nullptr;
ID3D11ShaderResourceView* gCubeDiffuseSpecularMapSRV   = nullptr;

ID3D11Resource*           gParticleDiffuseMap    = nullptr;
ID3D11ShaderResourceView* gParticleDiffuseMapSRV = nullptr;

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
		gGroundMesh = new Mesh("Hills.x");
		gCubeMesh   = new Mesh("Cube.x");
		gCrateMesh  = new Mesh("CargoContainer.x");
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
	if (!LoadTexture("Stars.jpg",                &gStarsDiffuseSpecularMap,  &gStarsDiffuseSpecularMapSRV ) ||
        !LoadTexture("GrassDiffuseSpecular.dds", &gGroundDiffuseSpecularMap, &gGroundDiffuseSpecularMapSRV) ||
        !LoadTexture("StoneDiffuseSpecular.dds", &gCubeDiffuseSpecularMap,   &gCubeDiffuseSpecularMapSRV) ||
        !LoadTexture("CargoA.dds",               &gCrateDiffuseSpecularMap,  &gCrateDiffuseSpecularMapSRV ) ||
        !LoadTexture("Smoke3.png",               &gParticleDiffuseMap,       &gParticleDiffuseMapSRV      ) ||
		!LoadTexture("Flare.jpg",                &gLightDiffuseMap,          &gLightDiffuseMapSRV         ))
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
    gCube   = new Model(gCubeMesh);
    gCrate  = new Model(gCrateMesh);

	// Initial positions
    gCube->SetPosition({ 42, 5, -10} );
    gCube->SetRotation({ 0.0f, ToRadians(-110.0f), 0.0f });
	gCube->SetScale(1.5f);
    gCrate->SetPosition({ -10, 0, 90 });
    gCrate->SetRotation({ 0.0f, ToRadians(40.0f), 0.0f });
	gCrate->SetScale(6.0f);
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
    gCamera->SetPosition({ 25, 18, -45 });
    gCamera->SetRotation({ ToRadians(10.0f), ToRadians(7.0f), 0.0f });


	
	//*************************************************************************
	// Initialise particles

	// Create the vertex layout for the particle data structure declared near the top of the file. This step creates
	// an object (ParticleLayout) that is used to describe to the GPU the data used for each particle
	auto signature = CreateSignatureForVertexLayout(ParticleElts, NumParticleElts);
	gD3DDevice->CreateInputLayout(ParticleElts, NumParticleElts, signature->GetBufferPointer(), signature->GetBufferSize(), &ParticleLayout);


	// Set up the initial particle data
	for (auto& particle : ParticlePoints)
	{
		particle.position = ParticleEmitterPos;
		particle.alpha    = Random(0.0f, 1.0f);
		particle.scale    = 5.0f;
		particle.rotation = Random(ToRadians(0), ToRadians(360));
    }
	for (auto& particleUpdate : ParticleUpdates)
    {
        particleUpdate.velocity = { Random(-1.0f, 1.0f), Random(2.5f, 5.0f), Random(-1.0f, 1.0f) };
        particleUpdate.rotationSpeed = Random(ToRadians(-10), ToRadians(10));
	}


	// Create the particle vertex buffer in GPU memory and copy over the contents just created (from CPU-memory)
	// We are going to update this vertex buffer every frame, so it must be defined as "dynamic" and writable (D3D11_USAGE_DYNAMIC & D3D11_CPU_ACCESS_WRITE)
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = NumParticles * sizeof(ParticlePoint); // Buffer size
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA initData; // Initial data
	initData.pSysMem = &ParticlePoints[0];
	if (FAILED( gD3DDevice->CreateBuffer( &bufferDesc, &initData, &ParticleVertexBuffer )))
	{
		return false;
	}

	//*************************************************************************
	
	return true;
}


// Release the geometry and scene resources created above
void ReleaseResources()
{
    ReleaseStates();

	if (ParticleLayout)       ParticleLayout->Release();
	if (ParticleVertexBuffer) ParticleVertexBuffer->Release();

    if (gLightDiffuseMapSRV)           gLightDiffuseMapSRV         ->Release();
    if (gLightDiffuseMap)              gLightDiffuseMap            ->Release();
    if (gParticleDiffuseMapSRV)        gParticleDiffuseMapSRV      ->Release();
    if (gParticleDiffuseMap)           gParticleDiffuseMap         ->Release();
    if (gCrateDiffuseSpecularMapSRV)   gCrateDiffuseSpecularMapSRV ->Release();
    if (gCrateDiffuseSpecularMap)      gCrateDiffuseSpecularMap    ->Release();
    if (gCubeDiffuseSpecularMapSRV)    gCubeDiffuseSpecularMapSRV  ->Release();
    if (gCubeDiffuseSpecularMap)       gCubeDiffuseSpecularMap     ->Release();
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
    delete gCrate;   gCrate  = nullptr;
    delete gCube;    gCube   = nullptr;
    delete gGround;  gGround = nullptr;
	delete gStars;   gStars  = nullptr;

    delete gLightMesh;   gLightMesh  = nullptr;
    delete gCrateMesh;   gCrateMesh  = nullptr;
    delete gCubeMesh;    gCubeMesh   = nullptr;
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
    gD3DContext->VSSetShader(gPixelLightingVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gPixelLightingPixelShader,  nullptr, 0);
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  ////// Switch off geometry shader when not using it (pass nullptr for first parameter)

	// States - no blending, normal depth buffer and back-face culling (standard set-up for opaque models)
	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gUseDepthBufferState, 0);
	gD3DContext->RSSetState(gCullBackState);

	// Render lit models, only change textures for each onee
	gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);

    gD3DContext->PSSetShaderResources(0, 1, &gGroundDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gGround->Render();

    gD3DContext->PSSetShaderResources(0, 1, &gCrateDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gCrate->Render();

    gD3DContext->PSSetShaderResources(0, 1, &gCubeDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gCube->Render();


	////--------------- Render sky ---------------////

	// Select which shaders to use next
	gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gTintedTexturePixelShader,   nullptr, 0);

	// Using a pixel shader that tints the texture - don't need a tint on the sky so set it to white
	gPerModelConstants.objectColour = { 1, 1, 1 }; 

    // Stars point inwards
    gD3DContext->RSSetState(gCullNoneState);

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

	// Unbind the depth buffer (the NULL) as we're now going to use it as a texture instead of writing to it normally
    // Then allow access to the depth buffer as a texture in the pixel shader
	gD3DContext->OMSetRenderTargets( 1, &gBackBufferRenderTarget, nullptr );
	gD3DContext->PSSetShaderResources(1, 1, &gDepthShaderView);
	gD3DContext->PSSetSamplers(1, 1, &gPointSampler);

    // Set shaders for particle rendering - the vertex shader just passes the data to the 
	// geometry shader, which generates a camera-facing 2D quad from the particle world position 
	// The pixel shader is very simple and just draws a tinted texture for each particle
	gD3DContext->VSSetShader(gParticlePassThruVertexShader, nullptr, 0);
	gD3DContext->GSSetShader(gParticleGeometryShader,       nullptr, 0);
	gD3DContext->PSSetShader(gSoftParticlePixelShader,      nullptr, 0);

	// Select the texture and sampler to use in the pixel shader
	gD3DContext->PSSetShaderResources(0, 1, &gParticleDiffuseMapSRV);

    // States - alpha blending and no culling
	gD3DContext->OMSetBlendState(gAlphaBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
	gD3DContext->RSSetState(gCullNoneState);

	// Set up particle vertex buffer / layout
	unsigned int particleVertexSize = sizeof(ParticlePoint);
	unsigned int offset = 0;
	gD3DContext->IASetVertexBuffers(0, 1, &ParticleVertexBuffer, &particleVertexSize, &offset);
	gD3DContext->IASetInputLayout(ParticleLayout);

	// Indicate that this is a point list and render it
	gD3DContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	gD3DContext->Draw(NumParticles, 0);

    // Detach depth buffer from shader and set it back to its normal usage
    ID3D11ShaderResourceView* nullSRV = nullptr;
    gD3DContext->PSSetShaderResources(1, 1, &nullSRV);
	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);


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
	
	gPerFrameConstants.viewportWidth  = static_cast<float>(gViewportWidth);  // The depth buffer work for soft particles requires the dimensions of the viewport
	gPerFrameConstants.viewportHeight = static_cast<float>(gViewportHeight);

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

//*****************************************************************************
// Particle sorting / update
//*****************************************************************************

// Update the particles each frame, also sort them into depth order
void UpdateParticles(float frameTime)
{
    //----
    // Particle update
	
    // Update particle instance data
	for (int i = 0; i < NumParticles; ++i)
	{
		// Decrease alpha, reset particle to new starting position when it disappears (i.e. number of particles stays constant)
		ParticlePoints[i].alpha -= 0.08f * frameTime;
		if (ParticlePoints[i].alpha <= 0.0f)
		{
			ParticlePoints[i].position = ParticleEmitterPos;
			ParticlePoints[i].alpha = Random(0.5f, 1.0f);
			ParticlePoints[i].scale = 5.0f;
		    ParticlePoints[i].rotation = Random(ToRadians(0), ToRadians(360));
		}

		// Increase scale, the particles expand as they fade
		ParticlePoints[i].scale *= pow( 1.15f, frameTime );

		// Slowly rotate
		ParticlePoints[i].rotation += ParticleUpdates[i].rotationSpeed * frameTime;

        // Move particle
		ParticlePoints[i].position += ParticleUpdates[i].velocity * frameTime;
	}


	//----
    // Sort particles on camera depth
	
    // Recalculate the array of particle camera depths
	CVector3 cameraFacing = gCamera->WorldMatrix().GetZAxis(); // Facing direction of camera
    for( int i = 0; i < NumParticles; ++i)
    {
		// Depth of particle is distance from camera to particle in the direction that the camera is facing
		// Calculate this with dot product of (vector from camera position to particle position) and (camera facing vector - calculated above)
		//**** MISSING calculate particle depth using above comment
        CVector3 cameraToParticle = ParticlePoints[i].position - gCamera->Position();
        ParticleDepths[i].depth = Dot(cameraFacing, cameraToParticle);

		// Store index of each particle, these will be reordered when we sort the depths and will then provide the correct order to render the particles
		ParticleDepths[i].index = i;
	}

    // Sort the particle depths. Using std::sort from <algorithm> header file. Also using a lambda function to specify the order
    // that we want things sorted in. Here putting largest depths first (draw back to front).
    // This is a modern style of C++ programming. We'll cover lambda functions in the C++11 lectures
    std::sort(ParticleDepths.begin(), ParticleDepths.end(), [](auto& a, auto& b) { return a.depth > b.depth; }); // The lambda function for a sort is given two objects
                                                                                                                 // a and b (particles here) and it returns true if a
                                                                                                                 // should come first, or false if b should come first


	//----
    // Pass updated particles over to GPU
	
	// The function "Map" gives us CPU access to the GPU vertex buffer. "Map" gives access to the buffer in CPU-memory so we can update it,
	// then when we call "Unmap" the buffer is copied back into GPU memory. We can speed this operation up since we don't actually need
	// a copy of the old contents of the buffer (just want to overwrite all the particle positions). So specify D3D11_MAP_WRITE_DISCARD,
	// which indicates that the old buffer can be discarded (no copy from GPU to CPU), and we will only write new data.
	// The buffer must have been created as "dynamic" to use "Map". Always code dynamic buffers and "Map" carefully for best performance.
	D3D11_MAPPED_SUBRESOURCE mappedData;
    gD3DContext->Map( ParticleVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData );

	// Copy current particle instance data, in order indicated by depth sorted array
	// Keep this process as fast as possible since the GPU may stall (have to wait, doing nothing) during this period
    ParticlePoint* vertexBufferData = (ParticlePoint*)mappedData.pData;
    for( int i = 0; i < NumParticles; ++i)
    {
		*vertexBufferData++ = ParticlePoints[ParticleDepths[i].index];
	}

	// Unlock the particle vertex buffer again so it can be used for rendering
    gD3DContext->Unmap( ParticleVertexBuffer, 0 );
}


//*************************************************************************


// Update models and camera. frameTime is the time passed since the last frame
void UpdateScene(float frameTime)
{
    UpdateParticles(frameTime);

    // Orbit one light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float lightRotate = 0.0f;
    static bool go = false;
	gLights[0].model->SetPosition(CVector3{ cos(lightRotate) * gLightOrbitRadius, 10, sin(lightRotate) * gLightOrbitRadius });
    if (go)  lightRotate -= gLightOrbitSpeed * frameTime;
    if (KeyHit(Key_1))  go = !go;

    // Orbit camera on < and > keys
    static float cameraOrbit = 0;
    if (KeyHeld(Key_Period) || KeyHeld(Key_Comma))
    {
        if (KeyHeld(Key_Comma))   cameraOrbit -= gCameraOrbitSpeed * frameTime;
        if (KeyHeld(Key_Period))  cameraOrbit += gCameraOrbitSpeed * frameTime;

    	gCamera->SetPosition(ParticleEmitterPos + CVector3{ cos(cameraOrbit) * gCameraOrbitRadius, 20, sin(cameraOrbit) * gCameraOrbitRadius });
        auto cameraMatrix = gCamera->WorldMatrix();
        cameraMatrix.FaceTarget(ParticleEmitterPos + CVector3{ 0, 20, 0 });
        gCamera->SetPosition(cameraMatrix.GetPosition());
        gCamera->SetRotation(cameraMatrix.GetEulerAngles());
    }
    else
    {
	    // Otherwise free control of camera
	    gCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D );
    }

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
        std::string windowTitle = "CO3303 Week 9: Soft Particles - Frame Time: " + frameTimeMs.str() +
                                  "ms, FPS: " + std::to_string(static_cast<int>(1 / avgFrameTime + 0.5f));
        SetWindowTextA(gHWnd, windowTitle.c_str());
        totalFrameTime = 0;
        frameCount = 0;
    }
}
