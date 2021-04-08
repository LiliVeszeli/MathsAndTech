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

#include "External/imgui-master/imgui.h"

#include "External/imgui-master/examples/imgui_impl_win32.h"
#include "External/imgui-master/examples/imgui_impl_dx11.h"

#include <array>
#include <sstream>
#include <memory>


//--------------------------------------------------------------------------------------
// Scene Data
//--------------------------------------------------------------------------------------

//********************
// Available post-processes
enum class PostProcess
{
	None,
	Copy,
	Tint,
	GreyNoise,
	Burn,
	Distort,
	Spiral,
	HeatHaze,
	Blur,
	Water,
	GaussianVertical,
	GaussianHorizontal,
	Pixelated, 
	Negative,
	Posterization,
	ChromaticAberration,
	Edge,
	Neon,
	Bloom,
	BloomSampler,
	Paint,
	Frost
};

enum class PostProcessMode
{
	Fullscreen,
	Area,
	Polygon,
};

//auto gCurrentPostProcess     = PostProcess::None; //make it a vector
std::vector<PostProcess> gCurrentPostProcess{};
std::vector<PostProcess> gCurrentPostProcessPoly{};

auto gCurrentPostProcessMode = PostProcessMode::Fullscreen;

//********************


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
Mesh* gWall1Mesh;
Mesh* gWall2Mesh;

Model* gStars;
Model* gGround;
Model* gCube;
Model* gCrate;
Model* gWall1;
Model* gWall2;

Camera* gCamera;


// Store lights in an array in this exercise
const int NUM_LIGHTS = 3;
struct Light
{
	Model*   model;
	CVector3 colour;
	float    strength;
};
Light gLights[NUM_LIGHTS];


// Additional light information
CVector3 gAmbientColour = { 0.5f, 0.5f, 0.6f }; // Background level of light (slightly bluish to match the far background, which is dark blue)
float    gSpecularPower = 256; // Specular power controls shininess - same for all models in this app

ColourRGBA gBackgroundColor = { 0.3f, 0.3f, 0.4f, 1.0f };

// Variables controlling light1's orbiting of the cube
const float gLightOrbitRadius = 20.0f;
const float gLightOrbitSpeed = 0.7f;

//fullscreen
bool tint = false;
bool blur = false;
bool gaussian = false;
bool noise = false;
bool burn = false;
bool distort = false;
bool spiral = false;
bool water = false;
bool pixel = false;
bool negative = false;
bool posterization = false;
bool chromatic = false;
bool edge = false;
bool neon = false;
bool bloom = false;
bool paint = false;
bool frost = false;

//polygon
bool polytint = false;
bool polyblur = false;
bool polygaussian = false;
bool polynoise = false;
bool polyburn = false;
bool polydistort = false;
bool polyspiral = false;
bool polywater = false;
bool polypixel = false;
bool polynegative = false;
bool polyposterization = false;
bool polychromatic = false;
bool polyedge = false;
bool polyneon = false;
bool polybloom = false;
bool polypaint = false;
bool polyfrost = false;

//imGUI tickbox fullscreen
bool tintBox = false;
bool blurBox = false;
bool gaussianBox = false;
bool noiseBox = false;
bool burnBox = false;
bool distortBox = false;
bool spiralBox = false;
bool waterBox = false;
bool pixelBox = false;
bool negativeBox = false;
bool posterizationBox = false;
bool chromaticBox = false;
bool edgeBox = false;
bool neonBox = false;
bool bloomBox = false;
bool paintBox = false;
bool frostBox = false;

//imGUI tickbox polygon
bool polytintBox = false;
bool polyblurBox = false;
bool polygaussianBox = false;
bool polynoiseBox = false;
bool polyburnBox = false;
bool polydistortBox = false;
bool polyspiralBox = false;
bool polywaterBox = false;
bool polypixelBox = false;
bool polynegativeBox = false;
bool polyposterizationBox = false;
bool polychromaticBox = false;
bool polyedgeBox = false;
bool polyneonBox = false;
bool polybloomBox = false;
bool polypaintBox = false;
bool polyfrostBox = false;

int blurCount = 0;
int gaussianCount = 0;

bool area = false;
bool fullscreen = true;
bool polygon = true;

bool motionBlur = false;
float pix[2];


//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
// Variables sent over to the GPU each frame
// The structures are now in Common.h
// IMPORTANT: Any new data you add in C++ code (CPU-side) is not automatically available to the GPU
//            Anything the shaders need (per-frame or per-model) needs to be sent via a constant buffer

PerFrameConstants gPerFrameConstants;      // The constants (settings) that need to be sent to the GPU each frame (see common.h for structure)
ID3D11Buffer*     gPerFrameConstantBuffer; // The GPU buffer that will recieve the constants above

PerModelConstants gPerModelConstants;      // As above, but constants (settings) that change per-model (e.g. world matrix)
ID3D11Buffer*     gPerModelConstantBuffer; // --"--

//**************************
PostProcessingConstants gPostProcessingConstants;       // As above, but constants (settings) for each post-process
ID3D11Buffer*           gPostProcessingConstantBuffer; // --"--
//**************************


//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// DirectX objects controlling textures used in this lab
ID3D11Resource*           gStarsDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gStarsDiffuseSpecularMapSRV = nullptr;
ID3D11Resource*           gGroundDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gGroundDiffuseSpecularMapSRV = nullptr;
ID3D11Resource*           gCrateDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gCrateDiffuseSpecularMapSRV = nullptr;
ID3D11Resource*           gCubeDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gCubeDiffuseSpecularMapSRV = nullptr;
ID3D11Resource* gWall1DiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gWall1DiffuseSpecularMapSRV = nullptr;
ID3D11Resource* gWall2DiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gWall2DiffuseSpecularMapSRV = nullptr;


ID3D11Resource*           gLightDiffuseMap = nullptr;
ID3D11ShaderResourceView* gLightDiffuseMapSRV = nullptr;



//****************************
// Post processing textures

// This texture will have the scene renderered on it. Then the texture is then used for post-processing
ID3D11Texture2D*          gSceneTexture      = nullptr; // This object represents the memory used by the texture on the GPU
ID3D11RenderTargetView*   gSceneRenderTarget = nullptr; // This object is used when we want to render to the texture above
ID3D11ShaderResourceView* gSceneTextureSRV   = nullptr; // This object is used to give shaders access to the texture above (SRV = shader resource view)

// This texture will have the scene renderered on it. Then the texture is then used for post-processing
ID3D11Texture2D* gSceneTexture2 = nullptr; // This object represents the memory used by the texture on the GPU
ID3D11RenderTargetView* gSceneRenderTarget2 = nullptr; // This object is used when we want to render to the texture above
ID3D11ShaderResourceView* gSceneTextureSRV2 = nullptr; // This object is used to give shaders access to the texture above (SRV = shader resource view)

ID3D11Texture2D* gPostProcessTextures[2];
ID3D11RenderTargetView* gPostProcessRenderTargets[2];
ID3D11ShaderResourceView* gPostProcessTextureSRVs[2];

ID3D11Texture2D* gBloomTexture = nullptr; // This object represents the memory used by the texture on the GPU
ID3D11RenderTargetView* gBloomRenderTarget = nullptr; // This object is used when we want to render to the texture above
ID3D11ShaderResourceView* gBloomTextureSRV = nullptr; // This object is used to give shaders access to the texture above (SRV = shader resource view)


int gCurrentPostProcessIndex;

// Additional textures used for specific post-processes
ID3D11Resource*           gNoiseMap = nullptr;
ID3D11ShaderResourceView* gNoiseMapSRV = nullptr;
ID3D11Resource*           gBurnMap = nullptr;
ID3D11ShaderResourceView* gBurnMapSRV = nullptr;
ID3D11Resource*           gDistortMap = nullptr;
ID3D11ShaderResourceView* gDistortMapSRV = nullptr;


ID3D11Resource* gFrostMap = nullptr;
ID3D11ShaderResourceView* gFrostMapSRV = nullptr;

//****************************



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
		gWall1Mesh = new Mesh("Wall1.x");
		gWall2Mesh = new Mesh("Wall2.x");
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
	if (!LoadTexture("Stars.jpg",                &gStarsDiffuseSpecularMap,  &gStarsDiffuseSpecularMapSRV) ||
		!LoadTexture("GrassDiffuseSpecular.dds", &gGroundDiffuseSpecularMap, &gGroundDiffuseSpecularMapSRV) ||
		!LoadTexture("holo.jpg"/*"StoneDiffuseSpecular.dds"*/, &gCubeDiffuseSpecularMap,   &gCubeDiffuseSpecularMapSRV) ||
		!LoadTexture("CargoA.dds",               &gCrateDiffuseSpecularMap,  &gCrateDiffuseSpecularMapSRV) ||
		!LoadTexture("Flare.jpg",                &gLightDiffuseMap,          &gLightDiffuseMapSRV) ||
		!LoadTexture("Noise.png",                &gNoiseMap,   &gNoiseMapSRV) ||
		!LoadTexture("Burn.png",                 &gBurnMap,    &gBurnMapSRV) ||
		!LoadTexture("Frost.jpg",                &gFrostMap, &gFrostMapSRV) ||
		!LoadTexture("Distort.png",              &gDistortMap, &gDistortMapSRV) ||
		!LoadTexture("brick_35epsilon.jpg", &gWall1DiffuseSpecularMap, &gWall1DiffuseSpecularMapSRV) ||
		!LoadTexture("brick_35epsilonzero2.jpg", &gWall2DiffuseSpecularMap, &gWall2DiffuseSpecularMapSRV))
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
	gPerFrameConstantBuffer       = CreateConstantBuffer(sizeof(gPerFrameConstants));
	gPerModelConstantBuffer       = CreateConstantBuffer(sizeof(gPerModelConstants));
	gPostProcessingConstantBuffer = CreateConstantBuffer(sizeof(gPostProcessingConstants));
	if (gPerFrameConstantBuffer == nullptr || gPerModelConstantBuffer == nullptr || gPostProcessingConstantBuffer == nullptr)
	{
		gLastError = "Error creating constant buffers";
		return false;
	}



	//********************************************
	//**** Create Scene Texture

	// We will render the scene to this texture instead of the back-buffer (screen), then we post-process the texture onto the screen
	// This is exactly the same code we used in the graphics module when we were rendering the scene onto a cube using a texture

	// Using a helper function to load textures from files above. Here we create the scene texture manually
	// as we are creating a special kind of texture (one that we can render to). Many settings to prepare:
	D3D11_TEXTURE2D_DESC sceneTextureDesc = {};
	sceneTextureDesc.Width = gViewportWidth;  // Full-screen post-processing - use full screen size for texture
	sceneTextureDesc.Height = gViewportHeight;
	sceneTextureDesc.MipLevels = 1; // No mip-maps when rendering to textures (or we would have to render every level)
	sceneTextureDesc.ArraySize = 1;
	sceneTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA texture (8-bits each)
	sceneTextureDesc.SampleDesc.Count = 1;
	sceneTextureDesc.SampleDesc.Quality = 0;
	sceneTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	sceneTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // IMPORTANT: Indicate we will use texture as render target, and pass it to shaders
	sceneTextureDesc.CPUAccessFlags = 0;
	sceneTextureDesc.MiscFlags = 0;
	if (FAILED(gD3DDevice->CreateTexture2D(&sceneTextureDesc, NULL, &gSceneTexture)))
	{
		gLastError = "Error creating scene texture";
		return false;
	}
	if (FAILED(gD3DDevice->CreateTexture2D(&sceneTextureDesc, NULL, &gSceneTexture2)))
	{
		gLastError = "Error creating scene texture";
		return false;
	}
	if (FAILED(gD3DDevice->CreateTexture2D(&sceneTextureDesc, NULL, &gBloomTexture)))
	{
		gLastError = "Error creating scene texture";
		return false;
	}
	gPostProcessTextures[0] = gSceneTexture;
	gPostProcessTextures[1] = gSceneTexture2;


	// We created the scene texture above, now we get a "view" of it as a render target, i.e. get a special pointer to the texture that
	// we use when rendering to it (see RenderScene function below)
	if (FAILED(gD3DDevice->CreateRenderTargetView(gSceneTexture, NULL, &gSceneRenderTarget)))
	{
		gLastError = "Error creating scene render target view";
		return false;
	}
	if (FAILED(gD3DDevice->CreateRenderTargetView(gSceneTexture2, NULL, &gSceneRenderTarget2)))
	{
		gLastError = "Error creating scene render target view";
		return false;
	}
	if (FAILED(gD3DDevice->CreateRenderTargetView(gBloomTexture, NULL, &gBloomRenderTarget)))
	{
		gLastError = "Error creating scene render target view";
		return false;
	}
	gPostProcessRenderTargets[0] = gSceneRenderTarget;
	gPostProcessRenderTargets[1] = gSceneRenderTarget2;

	// We also need to send this texture (resource) to the shaders. To do that we must create a shader-resource "view"
	D3D11_SHADER_RESOURCE_VIEW_DESC srDesc = {};
	srDesc.Format = sceneTextureDesc.Format;
	srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srDesc.Texture2D.MostDetailedMip = 0;
	srDesc.Texture2D.MipLevels = 1;
	if (FAILED(gD3DDevice->CreateShaderResourceView(gSceneTexture, &srDesc, &gSceneTextureSRV)))
	{
		gLastError = "Error creating scene shader resource view";
		return false;
	}
	if (FAILED(gD3DDevice->CreateShaderResourceView(gSceneTexture2, &srDesc, &gSceneTextureSRV2)))
	{
		gLastError = "Error creating scene shader resource view";
		return false;
	}
	if (FAILED(gD3DDevice->CreateShaderResourceView(gBloomTexture, &srDesc, &gBloomTextureSRV)))
	{
		gLastError = "Error creating scene shader resource view";
		return false;
	}

	gPostProcessTextureSRVs[0] = gSceneTextureSRV;
	gPostProcessTextureSRVs[1] = gSceneTextureSRV2;

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
	gWall1  = new Model(gWall1Mesh);
	gWall2 = new Model(gWall2Mesh);

	// Initial positions
	gCube->SetPosition({ 12, 15, 20 });
	gCube->SetRotation({ 0.0f, ToRadians(-30.0f),ToRadians(40.0f) });
	gCube->SetScale(1.5f);
	gCrate->SetPosition({ -10, 0, 90 });
	gCrate->SetRotation({ 0.0f, ToRadians(40.0f), 0.0f });
	gCrate->SetScale(6.0f);
	gStars->SetScale(8000.0f);
	gWall1->SetPosition({ 62, 2, -40 });
	gWall1->SetScale(28.0f);
	gWall2->SetPosition({ 62, 0, -10 });
	gWall2->SetScale(25.0f);

	// Light set-up - using an array this time
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		gLights[i].model = new Model(gLightMesh);
	}

	gLights[0].colour = { 0.8f, 0.8f, 1.0f };
	gLights[0].strength = 10;
	gLights[0].model->SetPosition({ 65, 10, -15 });
	gLights[0].model->SetScale(pow(gLights[0].strength, 1.0f)); // Convert light strength into a nice value for the scale of the light - equation is ad-hoc.

	gLights[1].colour = { 0.5f, 0.0f, 1.0f };
	gLights[1].strength = 30;
	gLights[1].model->SetPosition({ -40, 30, 80 });
	gLights[1].model->SetScale(50);

	gLights[2].colour = { 0.1f, 0.3f, 0.5f };
	gLights[2].strength = 30;
	gLights[2].model->SetPosition({ 80, 20, -45 });
	gLights[2].model->SetScale(pow(gLights[0].strength, 1.0f));


	////--------------- Set up camera ---------------////

	gCamera = new Camera();
	gCamera->SetPosition({ 25, 18, -100 });
	gCamera->SetRotation({ ToRadians(10.0f), ToRadians(7.0f), 0.0f });

	gPostProcessingConstants.blurStrength = 3.5;
	gPostProcessingConstants.gaussianStrength = 2;
	gPostProcessingConstants.pixelSize = 512;
	gPostProcessingConstants.numColours = 7;
	gPostProcessingConstants.threshold = 0.4f;
	gPostProcessingConstants.radius = 5;
	gPostProcessingConstants.freq = 0.115;

	gPostProcessingConstants.pixX = 2;
	gPostProcessingConstants.pixY = 2;
	pix[0] = gPostProcessingConstants.pixX;
	pix[1] = gPostProcessingConstants.pixY;

	gPostProcessingConstants.isArea = false;
	gPostProcessingConstants.isMotionBlur = false;

	return true;
}


// Release the geometry and scene resources created above
void ReleaseResources()
{
	ReleaseStates();

	if (gSceneTextureSRV)              gSceneTextureSRV->Release();
	if (gSceneRenderTarget)            gSceneRenderTarget->Release();
	if (gSceneTexture)                 gSceneTexture->Release();

	if (gSceneTextureSRV2)              gSceneTextureSRV2->Release();
	if (gSceneRenderTarget2)            gSceneRenderTarget2->Release();
	if (gSceneTexture2)                 gSceneTexture2->Release();

	if (gBloomTextureSRV)              gBloomTextureSRV->Release();
	if (gBloomRenderTarget)            gBloomRenderTarget->Release();
	if (gBloomTexture)                 gBloomTexture->Release();

	if (gDistortMapSRV)                gDistortMapSRV->Release();
	if (gDistortMap)                   gDistortMap->Release();
	if (gBurnMapSRV)                   gBurnMapSRV->Release();
	if (gBurnMap)                      gBurnMap->Release();
	if (gFrostMapSRV)                  gFrostMapSRV->Release();
	if (gFrostMap)                     gFrostMap->Release();
	if (gNoiseMapSRV)                  gNoiseMapSRV->Release();
	if (gNoiseMap)                     gNoiseMap->Release();

	if (gLightDiffuseMapSRV)           gLightDiffuseMapSRV->Release();
	if (gLightDiffuseMap)              gLightDiffuseMap->Release();
	if (gCrateDiffuseSpecularMapSRV)   gCrateDiffuseSpecularMapSRV->Release();
	if (gCrateDiffuseSpecularMap)      gCrateDiffuseSpecularMap->Release();
	if (gCubeDiffuseSpecularMapSRV)    gCubeDiffuseSpecularMapSRV->Release();
	if (gCubeDiffuseSpecularMap)       gCubeDiffuseSpecularMap->Release();
	if (gWall1DiffuseSpecularMapSRV)    gWall1DiffuseSpecularMapSRV->Release();
	if (gWall1DiffuseSpecularMap)       gWall1DiffuseSpecularMap->Release();
	if (gGroundDiffuseSpecularMapSRV)  gGroundDiffuseSpecularMapSRV->Release();
	if (gGroundDiffuseSpecularMap)     gGroundDiffuseSpecularMap->Release();
	if (gStarsDiffuseSpecularMapSRV)   gStarsDiffuseSpecularMapSRV->Release();
	if (gStarsDiffuseSpecularMap)      gStarsDiffuseSpecularMap->Release();

	if (gPostProcessingConstantBuffer)  gPostProcessingConstantBuffer->Release();
	if (gPerModelConstantBuffer)        gPerModelConstantBuffer->Release();
	if (gPerFrameConstantBuffer)        gPerFrameConstantBuffer->Release();

	ReleaseShaders();

	// See note in InitGeometry about why we're not using unique_ptr and having to manually delete
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		delete gLights[i].model;  gLights[i].model = nullptr;
	}
	delete gCamera;  gCamera = nullptr;
	delete gCrate;   gCrate = nullptr;
	delete gCube;    gCube = nullptr;
	delete gWall1;    gWall1 = nullptr;
	delete gWall2;    gWall2 = nullptr;
	delete gGround;  gGround = nullptr;
	delete gStars;   gStars = nullptr;
	
	delete gLightMesh;   gLightMesh = nullptr;
	delete gCrateMesh;   gCrateMesh = nullptr;
	delete gCubeMesh;    gCubeMesh = nullptr;
	delete gWall1Mesh;    gWall1Mesh = nullptr;
	delete gWall2Mesh;    gWall2Mesh = nullptr;
	delete gGroundMesh;  gGroundMesh = nullptr;
	delete gStarsMesh;   gStarsMesh = nullptr;
}



//--------------------------------------------------------------------------------------
// Scene Rendering
//--------------------------------------------------------------------------------------

// Render everything in the scene from the given camera
void RenderSceneFromCamera(Camera* camera)
{
	// Set camera matrices in the constant buffer and send over to GPU
	gPerFrameConstants.cameraMatrix = camera->WorldMatrix();
	gPerFrameConstants.viewMatrix = camera->ViewMatrix();
	gPerFrameConstants.projectionMatrix = camera->ProjectionMatrix();
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
	gD3DContext->PSSetShader(gPixelLightingPixelShader, nullptr, 0);
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  // Switch off geometry shader when not using it (pass nullptr for first parameter)

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

	gD3DContext->PSSetShaderResources(0, 1, &gWall1DiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gWall1->Render();

	gD3DContext->PSSetShaderResources(0, 1, &gWall2DiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gWall2->Render();

	
	////--------------- Render sky ---------------////

	// Select which shaders to use next
	gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gTintedTexturePixelShader, nullptr, 0);

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
	gD3DContext->PSSetShader(gTintedTexturePixelShader, nullptr, 0);

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
}



//**************************

// Select the appropriate shader plus any additional textures required for a given post-process
// Helper function shared by full-screen, area and polygon post-processing functions below
void SelectPostProcessShaderAndTextures(PostProcess postProcess)
{
	if (postProcess == PostProcess::Copy)
	{
		gD3DContext->PSSetShader(gCopyPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::Tint)
	{
		gD3DContext->PSSetShader(gTintPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::Blur)
	{
		gD3DContext->PSSetShader(gBlurPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::GaussianVertical)
	{
		gD3DContext->PSSetShader(gGaussianVerticalPostProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::GaussianHorizontal)
	{
		gD3DContext->PSSetShader(gGaussianHorizontalPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::Water)
	{
		gD3DContext->PSSetShader(gWaterPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::Paint)
	{
		gD3DContext->PSSetShader(gPaintPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::Frost)
	{
		gD3DContext->PSSetShader(gFrostPostProcess, nullptr, 0);
		gD3DContext->PSSetShaderResources(1, 1, &gFrostMapSRV);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}

	else if (postProcess == PostProcess::Neon)
	{
		gD3DContext->PSSetShader(gNeonPostProcess, nullptr, 0);
	}


	else if (postProcess == PostProcess::ChromaticAberration)
	{
		gD3DContext->PSSetShader(gChromaticAberrationPostProcess, nullptr, 0);
	}


	else if (postProcess == PostProcess::Edge)
	{
		gD3DContext->PSSetShader(gEdgePostProcess, nullptr, 0);
	}

	
	else if (postProcess == PostProcess::Posterization)
	{
		gD3DContext->PSSetShader(gPosterizationPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::Negative)
	{
		gD3DContext->PSSetShader(gNegativePostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::Pixelated)
	{
		gD3DContext->PSSetShader(gPixelatedPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::GreyNoise)
	{
		gD3DContext->PSSetShader(gGreyNoisePostProcess, nullptr, 0);

		// Give pixel shader access to the noise texture
		gD3DContext->PSSetShaderResources(1, 1, &gNoiseMapSRV);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}

	else if (postProcess == PostProcess::BloomSampler)
	{
		gD3DContext->PSSetShader(gBloomSamplerPostProcess, nullptr, 0);

		// Give pixel shader access to the noise texture
		//gD3DContext->PSSetShaderResources(1, 1, &gBloomTextureSRV);
	}

	else if (postProcess == PostProcess::Bloom)
	{
		gD3DContext->PSSetShader(gBloomPostProcess, nullptr, 0);

		// Give pixel shader access to the noise texture
		gD3DContext->PSSetShaderResources(1, 1, &gBloomTextureSRV);
	}

	else if (postProcess == PostProcess::Burn)
	{
		gD3DContext->PSSetShader(gBurnPostProcess, nullptr, 0);

		// Give pixel shader access to the burn texture (basically a height map that the burn level ascends)
		gD3DContext->PSSetShaderResources(1, 1, &gBurnMapSRV);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}

	else if (postProcess == PostProcess::Distort)
	{
		gD3DContext->PSSetShader(gDistortPostProcess, nullptr, 0);

		// Give pixel shader access to the distortion texture (containts 2D vectors (in R & G) to shift the texture UVs to give a cut-glass impression)
		gD3DContext->PSSetShaderResources(1, 1, &gDistortMapSRV);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}

	else if (postProcess == PostProcess::Spiral)
	{
		gD3DContext->PSSetShader(gSpiralPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::HeatHaze)
	{
		gD3DContext->PSSetShader(gHeatHazePostProcess, nullptr, 0);
	}
}



// Perform a full-screen post process from "scene texture" to back buffer
void FullScreenPostProcess(PostProcess postProcess, ID3D11RenderTargetView* renderTarget)
{
	// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
	//gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);

	gD3DContext->OMSetRenderTargets(1, &renderTarget, gDepthStencil);

	

	
	// Give the pixel shader (post-processing shader) access to the scene texture 
	//gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV);
	gD3DContext->PSSetShaderResources(0, 1, &gPostProcessTextureSRVs[gCurrentPostProcessIndex % 2]);
	gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)


	// Using special vertex shader that creates its own data for a 2D screen quad
	gD3DContext->VSSetShader(g2DQuadVertexShader, nullptr, 0);
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  // Switch off geometry shader when not using it (pass nullptr for first parameter)


	// States - no blending, don't write to depth buffer and ignore back-face culling
	gD3DContext->OMSetBlendState(gAlphaBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
	gD3DContext->RSSetState(gCullNoneState);


	// No need to set vertex/index buffer (see 2D quad vertex shader), just indicate that the quad will be created as a triangle strip
	gD3DContext->IASetInputLayout(NULL); // No vertex data
	gD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);


	// Select shader and textures needed for the required post-processes (helper function above)
	SelectPostProcessShaderAndTextures(postProcess);


	// Set 2D area for full-screen post-processing (coordinates in 0->1 range)
	gPostProcessingConstants.area2DTopLeft = { 0, 0 }; // Top-left of entire screen
	gPostProcessingConstants.area2DSize    = { 1, 1 }; // Full size of screen
	gPostProcessingConstants.area2DDepth   = 0;        // Depth buffer value for full screen is as close as possible


	// Pass over the above post-processing settings (also the per-process settings prepared in UpdateScene function below)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->VSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);


	// Draw a quad
	gD3DContext->Draw(4, 0);
	gCurrentPostProcessIndex++;
}


// Perform an area post process from "scene texture" to back buffer at a given point in the world, with a given size (world units)
void AreaPostProcess(PostProcess postProcess, CVector3 worldPoint, CVector2 areaSize, float offset)
{
	// First perform a full-screen copy of the scene to back-buffer
	FullScreenPostProcess(PostProcess::Copy, gPostProcessRenderTargets[(gCurrentPostProcessIndex + 1) % 2]);
	

	// Now perform a post-process of a portion of the scene to the back-buffer (overwriting some of the copy above)
	// Note: The following code relies on many of the settings that were prepared in the FullScreenPostProcess call above, it only
	//       updates a few things that need to be changed for an area process. If you tinker with the code structure you need to be
	//       aware of all the work that the above function did that was also preparation for this post-process area step

	// Select shader/textures needed for required post-process
	SelectPostProcessShaderAndTextures(postProcess);

	// Enable alpha blending - area effects need to fade out at the edges or the hard edge of the area is visible
	// A couple of the shaders have been updated to put the effect into a soft circle
	// Alpha blending isn't enabled for fullscreen and polygon effects so it doesn't affect those (except heat-haze, which works a bit differently)
	gD3DContext->OMSetBlendState(gAlphaBlendingState, nullptr, 0xffffff);


	// Use picking methods to find the 2D position of the 3D point at the centre of the area effect
	auto worldPointTo2D = gCamera->PixelFromWorldPt(worldPoint, gViewportWidth, gViewportHeight);
	CVector2 area2DCentre = { worldPointTo2D.x, worldPointTo2D.y };
	float areaDistance = worldPointTo2D.z - offset;
	
	// Nothing to do if given 3D point is behind the camera
	if (areaDistance < gCamera->NearClip())  return;
	
	// Convert pixel coordinates to 0->1 coordinates as used by the shader
	area2DCentre.x /= gViewportWidth;
	area2DCentre.y /= gViewportHeight;



	// Using new helper function here - it calculates the world space units covered by a pixel at a certain distance from the camera.
	// Use this to find the size of the 2D area we need to cover the world space size requested
	CVector2 pixelSizeAtPoint = gCamera->PixelSizeInWorldSpace(areaDistance, gViewportWidth, gViewportHeight);
	CVector2 area2DSize = { areaSize.x / pixelSizeAtPoint.x, areaSize.y / pixelSizeAtPoint.y };

	// Again convert the result in pixels to a result to 0->1 coordinates
	area2DSize.x /= gViewportWidth;
	area2DSize.y /= gViewportHeight;



	// Send the area top-left and size into the constant buffer - the 2DQuad vertex shader will use this to create a quad in the right place
	gPostProcessingConstants.area2DTopLeft = area2DCentre - 0.5f * area2DSize; // Top-left of area is centre - half the size
	gPostProcessingConstants.area2DSize = area2DSize;

	// Manually calculate depth buffer value from Z distance to the 3D point and camera near/far clip values. Result is 0->1 depth value
	// We've never seen this full calculation before, it's occasionally useful. It is derived from the material in the Picking lecture
	// Having the depth allows us to have area effects behind normal objects
	gPostProcessingConstants.area2DDepth = gCamera->FarClip() * (areaDistance - gCamera->NearClip()) / (gCamera->FarClip() - gCamera->NearClip());
	gPostProcessingConstants.area2DDepth /= areaDistance;

	// Pass over this post-processing area to shaders (also sends the per-process settings prepared in UpdateScene function below)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->VSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);


	// Draw a quad
	gD3DContext->Draw(4, 0);
	//gCurrentPostProcessIndex++;
}


// Perform an post process from "scene texture" to back buffer within the given four-point polygon and a world matrix to position/rotate/scale the polygon
void PolygonPostProcess(PostProcess postProcess, const std::array<CVector3, 4>& points, const CMatrix4x4& worldMatrix)
{
	// First perform a full-screen copy of the scene to back-buffer
	FullScreenPostProcess(PostProcess::Copy, gPostProcessRenderTargets[(gCurrentPostProcessIndex + 1) % 2]);


	// Now perform a post-process of a portion of the scene to the back-buffer (overwriting some of the copy above)
	// Note: The following code relies on many of the settings that were prepared in the FullScreenPostProcess call above, it only
	//       updates a few things that need to be changed for an area process. If you tinker with the code structure you need to be
	//       aware of all the work that the above function did that was also preparation for this post-process area step

	// Select shader/textures needed for required post-process
	SelectPostProcessShaderAndTextures(postProcess);


	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	// Loop through the given points, transform each to 2D (this is what the vertex shader normally does in most labs)
	for (unsigned int i = 0; i < points.size(); ++i)
	{
		CVector4 modelPosition = CVector4(points[i], 1);
		CVector4 worldPosition = modelPosition * worldMatrix;
		CVector4 viewportPosition = worldPosition * gCamera->ViewProjectionMatrix();

		gPostProcessingConstants.polygon2DPoints[i] = viewportPosition;
	}

	// Pass over the polygon points to the shaders (also sends the per-process settings prepared in UpdateScene function below)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->VSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);

	// Select the special 2D polygon post-processing vertex shader and draw the polygon
	gD3DContext->VSSetShader(g2DPolygonVertexShader, nullptr, 0);
	gD3DContext->Draw(4, 0);
	//gCurrentPostProcessIndex++;
}


//**************************
//std::array<CVector3, 4> pointsG = { { {0,5,0}, {-3,0,0}, {3,0,0} , {0,-5,0} } };;
CVector3 pos = { 20, 15, 0 };

// Rendering the scene
void RenderScene()
{
	//// Common settings ////
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	gCurrentPostProcessIndex = 0;



	// Set up the light information in the constant buffer
	// Don't send to the GPU yet, the function RenderSceneFromCamera will do that
	gPerFrameConstants.light1Colour   = gLights[0].colour * gLights[0].strength;
	gPerFrameConstants.light1Position = gLights[0].model->Position();
	gPerFrameConstants.light2Colour   = gLights[1].colour * gLights[1].strength;
	gPerFrameConstants.light2Position = gLights[1].model->Position();
	gPerFrameConstants.light3Colour = gLights[2].colour * gLights[2].strength;
	gPerFrameConstants.light3Position = gLights[2].model->Position();

	gPerFrameConstants.ambientColour  = gAmbientColour;
	gPerFrameConstants.specularPower  = gSpecularPower;
	gPerFrameConstants.cameraPosition = gCamera->Position();

	gPerFrameConstants.viewportWidth  = static_cast<float>(gViewportWidth);
	gPerFrameConstants.viewportHeight = static_cast<float>(gViewportHeight);



	////--------------- Main scene rendering ---------------////

	// Set the target for rendering and select the main depth buffer.
	// If using post-processing then render to the scene texture, otherwise to the usual back buffer
	// Also clear the render target to a fixed colour and the depth buffer to the far distance
	if (!gCurrentPostProcess.empty() || polygon || area)
	{
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget, gDepthStencil);
		gD3DContext->ClearRenderTargetView(gSceneRenderTarget, &gBackgroundColor.r);
	}
	else
	{
		gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);
		gD3DContext->ClearRenderTargetView(gBackBufferRenderTarget, &gBackgroundColor.r);
	}
	gD3DContext->ClearDepthStencilView(gDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Setup the viewport to the size of the main window
	D3D11_VIEWPORT vp;
	vp.Width = static_cast<FLOAT>(gViewportWidth);
	vp.Height = static_cast<FLOAT>(gViewportHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	gD3DContext->RSSetViewports(1, &vp);

	// Render the scene from the main camera
	RenderSceneFromCamera(gCamera);

	

	/*gD3DContext->OMSetRenderTargets(1, &gBloomRenderTarget, gDepthStencil);
	gD3DContext->ClearRenderTargetView(gBloomRenderTarget, &gBackgroundColor.r);

	RenderSceneFromCamera(gCamera);*/

	////--------------- Scene completion ---------------////

	

	// Run any post-processing steps
	if (!gCurrentPostProcess.empty() || polygon || area)
	{
		if (area == true) //gCurrentPostProcessMode == PostProcessMode::Area)
		{
			//for (int i = 0; i < gCurrentPostProcess.size(); i++)
			//{
			//	// Pass a 3D point for the centre of the affected area and the size of the (rectangular) area in world units
			//	//AreaPostProcess(gCurrentPostProcess[i], gCube->Position(), { 22, 22 }, 15);
			//	
			//}
			CVector3 pos = gCube->Position();
			//pos.y += 2;
			gPostProcessingConstants.isArea = true;
			AreaPostProcess(PostProcess::Pixelated, pos, { 32, 30 }, 15);
			AreaPostProcess(PostProcess::Neon, pos, { 32, 30 }, 15);
			gPostProcessingConstants.isArea = false;
		}

		if (polygon == true)
		{
			//gCurrentPostProcessIndex = 0;
			// An array of four points in world space - a tapered square centred at the origin
			std::array<CVector3, 4> points = { { {0,5,0}, {-3,0,0}, {3,0,0} , {0,-5,0} } }; // C++ strangely needs an extra pair of {} here... only for std:array...
			std::array<CVector3, 4> points2 = { { {5,2,0},{-5,2,0},  {5,-10,0} , {-5,-10,0} } };

			std::array<CVector3, 4> pointsSpade = { { {-7.5f,2,0},{-14.5f,2,0},  {-7.5f,-10,0} , {-14.5f,-10,0} } };
			std::array<CVector3, 4> pointsDiamond = { { {0,2,0},{-7,2,0},  {0,-10,0} , {-7,-10,0} } };
			std::array<CVector3, 4> pointsClub = { { {8,2,0},{0,2,0},  {8,-10,0} , {0,-10,0} } };
			std::array<CVector3, 4> pointsHeart = { { {15.0f,2,0},{7.9f,2,0},  {15.0f,-10,0} , {7.9f,-10,0} } };

			// A rotating matrix placing the model above in the scene
			static CMatrix4x4 polyMatrix = MatrixTranslation(pos);
			polyMatrix.e30 = pos.x;
			polyMatrix.e31 = pos.y;
			polyMatrix.e32 = pos.z;
			polyMatrix = MatrixRotationY(ToRadians(1)) * polyMatrix;

			static CMatrix4x4 polyMatrix2 = MatrixTranslation(CVector3{ 62, 13, -40 });

			static CMatrix4x4 polyMatrixSpade = MatrixTranslation(CVector3{ 62, 10, -10 });
			static CMatrix4x4 polyMatrixDiamond = MatrixTranslation(CVector3{ 62, 10, -10 });
			static CMatrix4x4 polyMatrixClub = MatrixTranslation(CVector3{ 62, 10, -10 });
			static CMatrix4x4 polyMatrixHeart = MatrixTranslation(CVector3{ 62, 10, -10 });


			PolygonPostProcess(PostProcess::Copy, points2, polyMatrix2);

			PolygonPostProcess(PostProcess::Distort, pointsSpade, polyMatrixSpade);
			PolygonPostProcess(PostProcess::Tint, pointsDiamond, polyMatrixDiamond);
			PolygonPostProcess(PostProcess::Pixelated, pointsClub, polyMatrixClub);
			PolygonPostProcess(PostProcess::Water, pointsHeart, polyMatrixHeart);

			for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
			{
				// Pass an array of 4 points and a matrix. Only supports 4 points.		
				PolygonPostProcess(gCurrentPostProcessPoly[i], points2, polyMatrix2);
			}
		}

		if (fullscreen == true)
		{
			/*FullScreenPostProcess(PostProcess::Copy, gBloomRenderTarget);
			gCurrentPostProcessIndex--;*/
			
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{	
				if (gCurrentPostProcess[i] == PostProcess::BloomSampler)
				{
					FullScreenPostProcess(PostProcess::Copy, gBloomRenderTarget);
					gCurrentPostProcessIndex--;
				}
			    FullScreenPostProcess(gCurrentPostProcess[i], gPostProcessRenderTargets[(gCurrentPostProcessIndex + 1) % 2]);			
			}
		}

		FullScreenPostProcess(PostProcess::Copy, gBackBufferRenderTarget);
		// These lines unbind the scene texture from the pixel shader to stop DirectX issuing a warning when we try to render to it again next frame
		ID3D11ShaderResourceView* nullSRV = nullptr;
		gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
	}

	//IMGUI controls
	ImGui::Begin("Postprocess Switch", 0, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Checkbox("Fullscreen        ", &fullscreen);
	ImGui::Checkbox("Area", &area);
	ImGui::Checkbox("Polygon", &polygon);
	ImGui::End();

	//ImGui::NewLine();
	ImGui::Begin("Fullscreen Postprocess", 0, ImGuiWindowFlags_AlwaysAutoResize);
	//Tint
	//ImGui::Text("Tint");
	//ImGui::Checkbox("Fullscreen", &tintBox);	
	//ImGui::Checkbox("Polygon", &polytintBox);
	ImGui::Checkbox("Tint", &tintBox);
	


	//Blur
	ImGui::Checkbox("Box Blur", &blurBox);
	//ImGui::Text("Box Blur");
	//ImGui::Checkbox("Fullscreen ", &blurBox);
	
	if (blurBox == true)
	{
		if (motionBlur == false)
		{
		
			ImGui::SameLine();
			if (ImGui::Button("-", ImVec2(20, 20)) == true)
			{
				if (blurCount >= 2)
				{
					for (int i = 0; i < gCurrentPostProcess.size(); i++)
					{
						if (gCurrentPostProcess[i] == PostProcess::Blur)
						{
							gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
							blurCount--;
							break;
						}
					}
				}
			}
			ImGui::SameLine();
			std::string s = std::to_string(blurCount);
			ImGui::Text(&s[0]);
			ImGui::SameLine();
			if (ImGui::Button("+", ImVec2(20, 20)) == true)
			{
				gCurrentPostProcess.push_back(PostProcess::Blur);
				blurCount++;
			}
	    }
		ImGui::Checkbox("Motion Blur", &motionBlur);

		gPostProcessingConstants.isMotionBlur = motionBlur;

		ImGui::SliderFloat("stregth", &gPostProcessingConstants.blurStrength, 2, 8);
	}
	

	//Water
	ImGui::Checkbox("Underwater", &waterBox);
	//ImGui::Text("Underwater");
	//ImGui::Checkbox("Fullscreen  ", &waterBox);
	//ImGui::Checkbox("Polygon  ", &polywaterBox);




	ImGui::Checkbox("Gaussian Blur", &gaussianBox);
	if (gaussianBox == true)
	{
		ImGui::SameLine();
		if (ImGui::Button("-", ImVec2(20, 20)) == true)
		{
			if (gaussianCount >= 2)
			{
				for (int i = 0; i < gCurrentPostProcess.size(); i++)
				{
					if (gCurrentPostProcess[i] == PostProcess::GaussianVertical)
					{
						gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
						break;
					}

				}
				for (int i = 0; i < gCurrentPostProcess.size(); i++)
				{
					if (gCurrentPostProcess[i] == PostProcess::GaussianHorizontal)
					{
						gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
						break;
					}
				}
				gaussianCount--;
			}
		}
		ImGui::SameLine();
		std::string s = std::to_string(gaussianCount);
		ImGui::Text(&s[0]);
		ImGui::SameLine();
		if (ImGui::Button("+", ImVec2(20, 20)) == true)
		{
			gCurrentPostProcess.push_back(PostProcess::GaussianVertical);
			gCurrentPostProcess.push_back(PostProcess::GaussianHorizontal);
			gaussianCount++;
		}
		ImGui::SliderFloat("stregth", &gPostProcessingConstants.gaussianStrength, 1.1, 4);
	}

	ImGui::Checkbox("Pixelated", &pixelBox);
	if (pixel == true)
	{
		ImGui::SliderFloat("size", &gPostProcessingConstants.pixelSize, 15, 2000);
	}

	ImGui::Checkbox("Negative", &negativeBox);

	ImGui::Checkbox("Posterization", &posterizationBox);
	if (posterization == true)
	{
		ImGui::SliderFloat("", &gPostProcessingConstants.numColours, 2, 35);
	}

	ImGui::Checkbox("Chromatic Aberration   ", &chromaticBox);

	ImGui::Checkbox("Edge Detection", &edgeBox);

	ImGui::Checkbox("Neon", &neonBox);

	ImGui::Checkbox("Paint", &paintBox);
	if (paint == true)
	{
		ImGui::SliderFloat("Radius", &gPostProcessingConstants.radius, 0, 10);
	}

	

	ImGui::Checkbox("Frost", &frostBox);
	if (frost == true)
	{
		ImGui::SliderFloat("Frequency", &gPostProcessingConstants.freq, 0.025, 0.8);
		ImGui::SliderFloat2("Pixel", pix, 0, 50);
	}


	ImGui::Checkbox("Bloom", &bloomBox);
	if (bloom == true)
	{
		ImGui::SliderFloat("Threshold", &gPostProcessingConstants.threshold, 0.1, 1.0);
	}
	
	ImGui::Checkbox("Grey Noise", &noiseBox);

	ImGui::Checkbox("Spiral", &spiralBox);

	ImGui::Checkbox("Distort", &distortBox);

	ImGui::Checkbox("Burn", &burnBox);
	ImGui::End();





	ImGui::Begin("Polygon Postprocess", 0, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Checkbox("Tint     ", &polytintBox);

	ImGui::Checkbox("Box blur", &polyblurBox);

	if (polyblurBox == true)
	{
		ImGui::SliderFloat("stregth ", &gPostProcessingConstants.blurStrength, 2, 8);
	}

	ImGui::Checkbox("Underwater", &polywaterBox);

	ImGui::Checkbox("Gaussian Blur", &polygaussianBox);
	ImGui::Checkbox("Pixelated", &polypixelBox);
	if (polypixel == true)
	{
		ImGui::SliderFloat("size", &gPostProcessingConstants.pixelSize, 15, 2000);
	}
	ImGui::Checkbox("Negative", &polynegativeBox);
	ImGui::Checkbox("Posterization", &polyposterizationBox);
	if (polyposterization == true)
	{
		ImGui::SliderFloat("", &gPostProcessingConstants.numColours, 2, 35);
	}
	ImGui::Checkbox("Chromatic Aberration ", &polychromaticBox);
	ImGui::Checkbox("Edge Detection", &polyedgeBox);
	ImGui::Checkbox("Neon", &polyneonBox);
	ImGui::Checkbox("Paint", &polypaintBox);
	if (polypaint == true)
	{
		ImGui::SliderFloat("Radius", &gPostProcessingConstants.radius, 0, 10);
	}
	ImGui::Checkbox("Frost", &polyfrostBox);
	if (polyfrost == true)
	{
		ImGui::SliderFloat("Frequency", &gPostProcessingConstants.freq, 0.025, 0.8);
		ImGui::SliderFloat2("Pixel", pix, 0, 50);
	}
	
	ImGui::Checkbox("Distort", &polydistortBox);

	ImGui::End();

	ImGui::Render();
	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, nullptr);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
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
	//***********


	// Select post process on keys
	if (KeyHit(Key_F1))  fullscreen = true;
	if (KeyHit(Key_F2))  area = true;//gCurrentPostProcessMode = PostProcessMode::Area;
	if (KeyHit(Key_F3))  polygon = true;


	//TINT
	if (tintBox == true)
	{
		if (tint == false)
		{
			gCurrentPostProcess.push_back(PostProcess::Tint);
			tint = true;
		}

	}
	else
	{
		if (tint == true)
		{
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{
				if (gCurrentPostProcess[i] == PostProcess::Tint)
				{
					gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
					break;
				}
			}
			tint = false;
		}		
	}

	if (polytintBox == true)
	{
		if (polytint == false)
		{
			gCurrentPostProcessPoly.push_back(PostProcess::Tint);
			polytint = true;
		}
	}
	else
	{
		if (polytint == true)
		{
			for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
			{
				if (gCurrentPostProcessPoly[i] == PostProcess::Tint)
				{
					gCurrentPostProcessPoly.erase(gCurrentPostProcessPoly.begin() + i);
					break;
				}
			}
			polytint = false;
		}
	}


	//BLUR
	if (blurBox == true)
	{
		if (blur == false)
		{
			gCurrentPostProcess.push_back(PostProcess::Blur);
			blur = true;
			blurCount++;
		}
	}
	else
	{
		if (blur == true)
		{
			while (blurCount != 0)
			{
				for (int i = 0; i < gCurrentPostProcess.size(); i++)
				{
					if (gCurrentPostProcess[i] == PostProcess::Blur)
					{
						gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
						blurCount--;
						break;
					}
				}
			}
			blur = false;
		}
	}

	//BLUR
	if (polyblurBox == true)
	{
		if (polyblur == false)
		{
			gCurrentPostProcessPoly.push_back(PostProcess::Blur);
			polyblur = true;
			//blurCount++;
		}
	}
	else
	{
		if (polyblur == true)
		{
			for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
			{
				if (gCurrentPostProcessPoly[i] == PostProcess::Blur)
				{
					gCurrentPostProcessPoly.erase(gCurrentPostProcessPoly.begin() + i);
					//blurCount--;
					break;
				}
			}
			
			polyblur = false;
		}
	}

	//WATER
	if (waterBox == true)
	{
		if (water == false)
		{		
			gCurrentPostProcess.push_back(PostProcess::Water);
			water = true;
		}
	}
	else
	{
		if (water == true)
		{
			water = false;
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{
				if (gCurrentPostProcess[i] == PostProcess::Water)
				{
					gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
					break;
				}
			}
		}
	}
	//WATER
	if (polywaterBox == true)
	{
		if (polywater == false)
		{
			gCurrentPostProcessPoly.push_back(PostProcess::Water);
			polywater = true;
		}
	}
	else
	{
		if (polywater == true)
		{
			polywater = false;
			for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
			{
				if (gCurrentPostProcessPoly[i] == PostProcess::Water)
				{
					gCurrentPostProcessPoly.erase(gCurrentPostProcessPoly.begin() + i);
					break;
				}
			}
		}
	}


	//PAINT
	if (paintBox == true)
	{
		if (paint == false)
		{
			gCurrentPostProcess.push_back(PostProcess::Paint);
			paint = true;
		}
	}
	else
	{
		if (paint == true)
		{
			paint = false;
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{
				if (gCurrentPostProcess[i] == PostProcess::Paint)
				{
					gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
					break;
				}
			}
		}
	}
	if (polypaintBox == true)
	{
		if (polypaint == false)
		{
			gCurrentPostProcessPoly.push_back(PostProcess::Paint);
			polypaint = true;
		}
	}
	else
	{
		if (polypaint == true)
		{
			polypaint = false;
			for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
			{
				if (gCurrentPostProcessPoly[i] == PostProcess::Paint)
				{
					gCurrentPostProcessPoly.erase(gCurrentPostProcessPoly.begin() + i);
					break;
				}
			}
		}
	}
	

	//FROST
	if (frostBox == true)
	{
		if (frost == false)
		{
			gCurrentPostProcess.push_back(PostProcess::Frost);
			frost = true;
		}
	}
	else
	{
		if (frost == true)
		{
			frost = false;
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{
				if (gCurrentPostProcess[i] == PostProcess::Frost)
				{
					gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
					break;
				}
			}
		}
	}
	if (polyfrostBox == true)
	{
		if (polyfrost == false)
		{
			gCurrentPostProcessPoly.push_back(PostProcess::Frost);
			polyfrost = true;
		}
	}
	else
	{
		if (polyfrost == true)
		{
			polyfrost = false;
			for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
			{
				if (gCurrentPostProcessPoly[i] == PostProcess::Frost)
				{
					gCurrentPostProcessPoly.erase(gCurrentPostProcessPoly.begin() + i);
					break;
				}
			}
		}
	}

    //Posterization
	if (posterizationBox == true)
	{
		if (posterization == false)
		{		
			gCurrentPostProcess.push_back(PostProcess::Posterization);
			posterization = true;
		}
	}
	else
	{
		if (posterization == true)
		{
			posterization = false;
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{
				if (gCurrentPostProcess[i] == PostProcess::Posterization)
				{
					gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
					break;
				}
			}
		}
	}
	if (polyposterizationBox == true)
	{
		if (polyposterization == false)
		{
			gCurrentPostProcessPoly.push_back(PostProcess::Posterization);
			polyposterization = true;
		}
	}
	else
	{
		if (polyposterization == true)
		{
			polyposterization = false;
			for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
			{
				if (gCurrentPostProcessPoly[i] == PostProcess::Posterization)
				{
					gCurrentPostProcessPoly.erase(gCurrentPostProcessPoly.begin() + i);
					break;
				}
			}
		}
	}

	//ChromaticAberration	
	if (chromaticBox == true)
	{
		if (chromatic == false)
		{
			gCurrentPostProcess.push_back(PostProcess::ChromaticAberration);
			chromatic = true;
		}
	}
	else
	{
		if (chromatic == true)
		{
			chromatic = false;
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{
				if (gCurrentPostProcess[i] == PostProcess::ChromaticAberration)
				{
					gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
					break;
				}
			}
		}
	}
	if (polychromaticBox == true)
	{
		if (polychromatic == false)
		{
			gCurrentPostProcessPoly.push_back(PostProcess::ChromaticAberration);
			polychromatic = true;
		}
	}
	else
	{
		if (polychromatic == true)
		{
			polychromatic = false;
			for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
			{
				if (gCurrentPostProcessPoly[i] == PostProcess::ChromaticAberration)
				{
					gCurrentPostProcessPoly.erase(gCurrentPostProcessPoly.begin() + i);
					break;
				}
			}
		}
	}

	//EDGE
	if (edgeBox == true)
	{
		if (edge == false)
		{
			gCurrentPostProcess.push_back(PostProcess::Edge);
			edge = true;
		}
	}
	else
	{
		if (edge == true)
		{
			edge = false;
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{
				if (gCurrentPostProcess[i] == PostProcess::Edge)
				{
					gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
					break;
				}
			}
		}
	}
	if (polyedgeBox == true)
	{
		if (polyedge == false)
		{
			gCurrentPostProcessPoly.push_back(PostProcess::Edge);
			polyedge = true;
		}
	}
	else
	{
		if (polyedge == true)
		{
			polyedge = false;
			for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
			{
				if (gCurrentPostProcessPoly[i] == PostProcess::Edge)
				{
					gCurrentPostProcessPoly.erase(gCurrentPostProcessPoly.begin() + i);
					break;
				}
			}
		}
	}

	//NEON
	if (neonBox == true)
	{
		if (neon == false)
		{
			gCurrentPostProcess.push_back(PostProcess::Neon);
			neon = true;
		}
	}
	else
	{
		if (neon == true)
		{
			neon = false;
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{
				if (gCurrentPostProcess[i] == PostProcess::Neon)
				{
					gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
					break;
				}
			}
		}
	}
	if (polyneonBox == true)
	{
		if (polyneon == false)
		{
			gCurrentPostProcessPoly.push_back(PostProcess::Neon);
			polyneon = true;
		}
	}
	else
	{
		if (polyneon == true)
		{
			polyneon = false;
			for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
			{
				if (gCurrentPostProcessPoly[i] == PostProcess::Neon)
				{
					gCurrentPostProcessPoly.erase(gCurrentPostProcessPoly.begin() + i);
					break;
				}
			}
		}
	}

	//PIXELATED
	if (pixelBox == true)
	{
		if (pixel == false)
		{
			gCurrentPostProcess.push_back(PostProcess::Pixelated);
			pixel = true;
		}
	}
	else
	{
		if (pixel == true)
		{
			pixel = false;
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{
				if (gCurrentPostProcess[i] == PostProcess::Pixelated)
				{
					gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
					break;
				}
			}
		}
	}
	if (polypixelBox == true)
	{
		if (polypixel == false)
		{
			gCurrentPostProcessPoly.push_back(PostProcess::Pixelated);
			polypixel = true;
		}
	}
	else
	{
		if (polypixel == true)
		{
			polypixel = false;
			for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
			{
				if (gCurrentPostProcessPoly[i] == PostProcess::Pixelated)
				{
					gCurrentPostProcessPoly.erase(gCurrentPostProcessPoly.begin() + i);
					break;
				}
			}
		}
	}

	//NEGATIVE
	if (negativeBox == true)
	{
		if (negative == false)
		{
			gCurrentPostProcess.push_back(PostProcess::Negative);
			negative = true;
		}
	}
	else
	{
		if (negative == true)
		{
			negative = false;
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{
				if (gCurrentPostProcess[i] == PostProcess::Negative)
				{
					gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
					break;
				}
			}
		}
	}
	if (polynegativeBox == true)
	{
		if (polynegative == false)
		{
			gCurrentPostProcessPoly.push_back(PostProcess::Negative);
			polynegative = true;
		}
	}
	else
	{
		if (polynegative == true)
		{
			polynegative = false;
			for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
			{
				if (gCurrentPostProcessPoly[i] == PostProcess::Negative)
				{
					gCurrentPostProcessPoly.erase(gCurrentPostProcessPoly.begin() + i);
					break;
				}
			}
		}
	}

	//GAUSSIAN BLUR
	if (gaussianBox == true)
	{
		if (gaussian == false)
		{		
			gCurrentPostProcess.push_back(PostProcess::GaussianVertical);
			gCurrentPostProcess.push_back(PostProcess::GaussianHorizontal);
			gaussian = true;
			gaussianCount++;
		}
	}
	else
	{
		if (gaussian == true)
		{
			gaussian = false;
			while (gaussianCount != 0)
			{
				for (int i = 0; i < gCurrentPostProcess.size(); i++)
				{
					if (gCurrentPostProcess[i] == PostProcess::GaussianVertical)
					{
						gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
						break;
					}

				}
				for (int i = 0; i < gCurrentPostProcess.size(); i++)
				{
					if (gCurrentPostProcess[i] == PostProcess::GaussianHorizontal)
					{
						gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
						break;
					}
				}
				gaussianCount--;
			}
		}
	}
	if (polygaussianBox == true)
	{
		if (polygaussian == false)
		{
			gCurrentPostProcessPoly.push_back(PostProcess::GaussianVertical);
			gCurrentPostProcessPoly.push_back(PostProcess::GaussianHorizontal);
			polygaussian = true;
			//gaussianCount++;
		}
	}
	else
	{
		if (polygaussian == true)
		{
			polygaussian = false;
			
				for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
				{
					if (gCurrentPostProcessPoly[i] == PostProcess::GaussianVertical)
					{
						gCurrentPostProcessPoly.erase(gCurrentPostProcessPoly.begin() + i);
						break;
					}

				}
				for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
				{
					if (gCurrentPostProcessPoly[i] == PostProcess::GaussianHorizontal)
					{
						gCurrentPostProcessPoly.erase(gCurrentPostProcessPoly.begin() + i);
						break;
					}
				}
				
			
		}
	}



	//BLOOM
	if (bloomBox == true)
	{
		if (bloom == false)
		{
			gCurrentPostProcess.push_back(PostProcess::BloomSampler);
			gCurrentPostProcess.push_back(PostProcess::GaussianVertical);
			gCurrentPostProcess.push_back(PostProcess::GaussianHorizontal);
			gCurrentPostProcess.push_back(PostProcess::Bloom);
			bloom = true;
			//gaussianCount++;
		}
	}
	else
	{
		if (bloom == true)
		{
			bloom = false;
			//while (gaussianCount != 0)
			//{
				for (int i = 0; i < gCurrentPostProcess.size(); i++)
				{
					if (gCurrentPostProcess[i] == PostProcess::GaussianVertical)
					{
						gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
						break;
					}

				}
				for (int i = 0; i < gCurrentPostProcess.size(); i++)
				{
					if (gCurrentPostProcess[i] == PostProcess::GaussianHorizontal)
					{
						gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
						break;
					}
				}


				for (int i = 0; i < gCurrentPostProcess.size(); i++)
				{
					if (gCurrentPostProcess[i] == PostProcess::Bloom)
					{
						gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
						break;
					}
				}

				for (int i = 0; i < gCurrentPostProcess.size(); i++)
				{
					if (gCurrentPostProcess[i] == PostProcess::BloomSampler)
					{
						gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
						break;
					}
				}
				//gaussianCount--;
			//}
		}
	}

	//GREY NOISE
	if (noiseBox == true)
	{
		if (noise == false)
		{
			gCurrentPostProcess.push_back(PostProcess::GreyNoise);
			noise = true;
		}
	}
	else
	{
		if (noise == true)
		{
			noise = false;
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{
				if (gCurrentPostProcess[i] == PostProcess::GreyNoise)
				{
					gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
					break;
				}
			}
		}
	}


	//BURN
	if (burnBox == true)
	{
		if (burn == false)
		{
			gCurrentPostProcess.push_back(PostProcess::Burn);
			burn = true;
		}
	}
	else
	{
		if (burn == true)
		{
			burn = false;
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{
				if (gCurrentPostProcess[i] == PostProcess::Burn)
				{
					gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
					break;
				}
			}
		}
	}

	//DISTORT
	if (distortBox == true)
	{
		if (distort == false)
		{
			gCurrentPostProcess.push_back(PostProcess::Distort);
			distort = true;
		}
	}
	else
	{
		if (distort == true)
		{
			distort = false;
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{
				if (gCurrentPostProcess[i] == PostProcess::Distort)
				{
					gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
					break;
				}
			}
		}
	}
	if (polydistortBox == true)
	{
		if (polydistort == false)
		{
			gCurrentPostProcessPoly.push_back(PostProcess::Distort);
			polydistort = true;
		}
	}
	else
	{
		if (polydistort == true)
		{
			polydistort = false;
			for (int i = 0; i < gCurrentPostProcessPoly.size(); i++)
			{
				if (gCurrentPostProcessPoly[i] == PostProcess::Distort)
				{
					gCurrentPostProcessPoly.erase(gCurrentPostProcessPoly.begin() + i);
					break;
				}
			}
		}
	}

	//SPIRAL
	if (spiralBox == true)
	{
		if (spiral == false)
		{
			gCurrentPostProcess.push_back(PostProcess::Spiral);
			spiral = true;
		}
	}
	else
	{
		if (spiral == true)
		{
			spiral = false;
			for (int i = 0; i < gCurrentPostProcess.size(); i++)
			{
				if (gCurrentPostProcess[i] == PostProcess::Spiral)
				{
					gCurrentPostProcess.erase(gCurrentPostProcess.begin() + i);
					break;
				}
			}
		}
	}

	//if (KeyHit(Key_6))   gCurrentPostProcess = PostProcess::HeatHaze;
	//if (KeyHit(Key_9))   gCurrentPostProcess = PostProcess::Copy;
	//if (KeyHit(Key_0))   gCurrentPostProcess = PostProcess::None;

	
	if (KeyHeld(Key_L))
	{
		pos.x+= 5 * frameTime;
	}
	if (KeyHeld(Key_K))
	{
		pos.z -= 5 * frameTime;
	}
	if (KeyHeld(Key_J))
	{
		pos.x -= 5 * frameTime;
	}
	if (KeyHeld(Key_I))
	{
		pos.z += 5 * frameTime;
	}

	// Post processing settings - all data for post-processes is updated every frame whether in use or not (minimal cost)
	
	// Colour for tint shader
	gPostProcessingConstants.tintColour = { 0.6f, 0, 0.5f };
	gPostProcessingConstants.tintColour2 = { 0, 0, 1.0f };

	gPostProcessingConstants.tintColourWater = { 0, 0.75f, 0.7f };
	gPostProcessingConstants.frameTime += frameTime;

	// Noise scaling adjusts how fine the grey noise is.
	const float grainSize = 140; // Fineness of the noise grain
	gPostProcessingConstants.noiseScale  = { gViewportWidth / grainSize, gViewportHeight / grainSize };

	// The noise offset is randomised to give a constantly changing noise effect (like tv static)
	gPostProcessingConstants.noiseOffset = { Random(0.0f, 1.0f), Random(0.0f, 1.0f) };

	// Set and increase the burn level (cycling back to 0 when it reaches 1.0f)
	const float burnSpeed = 0.2f;
	gPostProcessingConstants.burnHeight = fmod(gPostProcessingConstants.burnHeight + burnSpeed * frameTime, 1.0f);

	// Set the level of distortion
	gPostProcessingConstants.distortLevel = 0.03f;

	// Set and increase the amount of spiral - use a tweaked cos wave to animate
	static float wiggle = 0.0f;
	const float wiggleSpeed = 1.0f;
	gPostProcessingConstants.spiralLevel = ((1.0f - cos(wiggle)) * 4.0f );
	wiggle += wiggleSpeed * frameTime;

	gPostProcessingConstants.waterWiggle += frameTime;

	// Update heat haze timer
	gPostProcessingConstants.heatHazeTimer += frameTime;

	//***********


	// Orbit one light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float lightRotate = 0.0f;
	static bool go = true;
	gLights[0].model->SetPosition({ 50 + cos(lightRotate) * gLightOrbitRadius, 10, 35 + sin(lightRotate) * gLightOrbitRadius });
	if (go)  lightRotate -= gLightOrbitSpeed * frameTime;
	if (KeyHit(Key_L))  go = !go;

	static float cubeRotate = 0.0f;
	gCube->SetRotation({ 0.0f, ToRadians(-30.0f + cubeRotate),ToRadians(40.0f) });
	cubeRotate += frameTime *8;
	// Control of camera
	gCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D);

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
		std::string windowTitle = "CO3303 Week 14: Area Post Processing - Frame Time: " + frameTimeMs.str() +
			"ms, FPS: " + std::to_string(static_cast<int>(1 / avgFrameTime + 0.5f));
		SetWindowTextA(gHWnd, windowTitle.c_str());
		totalFrameTime = 0;
		frameCount = 0;
	}
}
