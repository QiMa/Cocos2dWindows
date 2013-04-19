#include "pch.h"
#include "HelloXAMLComponent.h"
#include "Direct3DContentProvider.h"

using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Microsoft::WRL;
using namespace Windows::Phone::Graphics::Interop;
using namespace Windows::Phone::Input::Interop;

namespace PhoneDirect3DXamlAppComponent
{

Direct3DBackground::Direct3DBackground() :
	m_timer(ref new BasicTimer())
{
}

IDrawingSurfaceBackgroundContentProvider^ Direct3DBackground::CreateContentProvider()
{
	ComPtr<Direct3DContentProvider> provider = Make<Direct3DContentProvider>(this);
	return reinterpret_cast<IDrawingSurfaceBackgroundContentProvider^>(provider.Detach());
}

// IDrawingSurfaceManipulationHandler
void Direct3DBackground::SetManipulationHost(DrawingSurfaceManipulationHost^ manipulationHost)
{
	manipulationHost->PointerPressed +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DBackground::OnPointerPressed);

	manipulationHost->PointerMoved +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DBackground::OnPointerMoved);

	manipulationHost->PointerReleased +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DBackground::OnPointerReleased);
}

// 事件处理程序
void Direct3DBackground::OnPointerPressed(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	// 在此处插入代码。
}

void Direct3DBackground::OnPointerMoved(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	// 在此处插入代码。
}

void Direct3DBackground::OnPointerReleased(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	// 在此处插入代码。
}

// 与 Direct3DContentProvider 交互
HRESULT Direct3DBackground::Connect(_In_ IDrawingSurfaceRuntimeHostNative* host, _In_ ID3D11Device1* device)
{
	m_renderer = ref new cocos2d::DirectXRender();
//	m_renderer->Initialize(device);
	//m_renderer->UpdateForWindowSizeChange(WindowBounds.Width, WindowBounds.Height);

	// 在呈现器完成初始化后重新启动计时器。
	m_timer->Reset();

	return S_OK;
}

void Direct3DBackground::Disconnect()
{
	m_renderer = nullptr;
}

HRESULT Direct3DBackground::PrepareResources(_In_ const LARGE_INTEGER* presentTargetTime, _Inout_ DrawingSurfaceSizeF* desiredRenderTargetSize)
{
	m_timer->Update();
	//m_renderer->Update(m_timer->Total, m_timer->Delta);

	desiredRenderTargetSize->width = RenderResolution.Width;
	desiredRenderTargetSize->height = RenderResolution.Height;

	return S_OK;
}

HRESULT Direct3DBackground::Draw(_In_ ID3D11Device1* device, _In_ ID3D11DeviceContext1* context, _In_ ID3D11RenderTargetView* renderTargetView)
{
	//m_renderer->UpdateDevice(device, context, renderTargetView);
	m_renderer->Render();

	RequestAdditionalFrame();

	return S_OK;
}

}