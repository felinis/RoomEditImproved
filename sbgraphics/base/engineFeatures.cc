#include "engineFeatures.h"

bool AreAllEngineFeaturesSupported(ID3D12Device *device, HWND hWnd)
{
	//check bindless textures support
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS options;
		if FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)))
			return false;
		if (options.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_2)
		{
			MessageBox(hWnd, "Your graphics device does not support bindless resources.", "Sabre Engine", MB_ICONERROR);
			return false;
		}
	}

	//check Shader Model 6.4 support (since all shaders are compiled for it and it is necessary for RT)
	{
		D3D12_FEATURE_DATA_SHADER_MODEL sm;
		sm.HighestShaderModel = D3D_SHADER_MODEL_6_4;
		if FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm)))
			return false;
		if (sm.HighestShaderModel < D3D_SHADER_MODEL_6_4)
		{
			MessageBox(hWnd, "Your graphics device does not support shader model 6.4.", "Sabre Engine", MB_ICONERROR);
			return false;
		}
	}

#ifdef RAY_TRACING
	{
		//check RT support
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 o5;
		if FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &o5, sizeof(o5)))
			return false;
		if (o5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
		{
			MessageBox(hWnd, "Your graphics device does not support ray-tracing.", "Sabre Engine", MB_ICONERROR);
			return false;
		}
	}
#endif

	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE rs = {};
		rs.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rs, sizeof(rs)))
		{
			MessageBox(hWnd, "Your graphics device does not support root signature 1.1, which is required for more efficient GPU processing.", "Sabre Engine", MB_ICONERROR);
			return false;
		}
	}

	return true;
}
