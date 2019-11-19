/*
	Callback function for the window. Handles the quit message and passes all other messages onto
	the message queue to be handled by the application.
*/
static LRESULT wnd_proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
		PostQuitMessage(0);
	else
		return DefWindowProcW(wnd, msg, wparam, lparam);

	return 0;
}

static RECT get_client_rect(HWND wnd)
{
	RECT client_rect;
	const BOOL result = GetClientRect(wnd, &client_rect);
	assert(result);

	return client_rect;
}

void create_display(display_context* display, const char* title, int32_t x, int32_t y, int32_t width, int32_t height, bool resizable)
{
	uint32_t window_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	if (resizable)
		window_style |= WS_THICKFRAME | WS_MAXIMIZEBOX;

	// Initialise window description
	const WNDCLASSEXW window_class = {
		sizeof(WNDCLASSEX),
		CS_HREDRAW | CS_VREDRAW,
		&wnd_proc,
		0,
		0,
		GetModuleHandleW(nullptr),
		LoadIcon(nullptr, IDI_APPLICATION),
		LoadCursor(nullptr, IDC_ARROW),
		nullptr,
		nullptr,
		L"WindowClass",
		nullptr
	};

	const ATOM register_result = RegisterClassExW(&window_class);
	assert(register_result);

	const int wide_buffer_size = 256;
	wchar_t wide_buffer[wide_buffer_size];
	MultiByteToWideChar(CP_UTF8, 0, title, -1, wide_buffer, wide_buffer_size);

	// Calculate total window size (including title bar and edges) from the requested viewable size
	if (width != -1 && height != -1)
	{
		RECT rect = {0, 0, width, height};
		if (AdjustWindowRect(&rect, window_style, false))
		{
			width = rect.right - rect.left;
			height = rect.bottom - rect.top;
		}
	}

	// Create the window
	HWND wnd = CreateWindowExW(
		0,
		L"WindowClass",
		wide_buffer,
		window_style,
		x,
		y,
		width,
		height,
		nullptr,
		nullptr,
		GetModuleHandleW(nullptr),
		nullptr
	);
	assert(wnd);

	// Show the window
	ShowWindow(wnd, SW_SHOWNORMAL);

	// Get the viewable display size
	const RECT window_size = get_client_rect(wnd);

	// Setup our display object
	display->wnd = wnd;
	display->width = window_size.right - window_size.left;
	display->height = window_size.bottom - window_size.top;
}

/*
	Creates a DXGI swap chain which represents the display front and back buffers.
*/
static void create_swap_chain(d3d_context* d3d, display_context* display)
{
	// Save display for in order to access display width and height
	d3d->display = display;

	// Convert D3D device into a DXGI device
	d3d->dxgi_device = com_cast<IDXGIDevice4>(d3d->device);

	// Get DXGI adapter (GFX card)
	IDXGIAdapter* adapter;
	check_hresult(d3d->dxgi_device->GetAdapter(&adapter));
	d3d->dxgi_adapter = com_cast<IDXGIAdapter4>(adapter);

	// Get DXGI factory from adapter
	check_hresult(d3d->dxgi_adapter->GetParent(__uuidof(IDXGIFactory5), (void**)&d3d->dxgi_factory));

	// Initialise swap chain description
	const DXGI_SWAP_CHAIN_DESC1 desc = {
		(uint32_t)display->width,
		(uint32_t)display->height,
		DXGI_FORMAT_B8G8R8A8_UNORM,
		false,
		{1, 0},
		DXGI_USAGE_RENDER_TARGET_OUTPUT,
		2,													// 2 buffers (front and back)
		DXGI_SCALING_NONE,
		DXGI_SWAP_EFFECT_FLIP_DISCARD,
		DXGI_ALPHA_MODE_IGNORE,
		DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
	};

	// Borderless fullscreen windows provide a better user experience as they can be switched faster
	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc = {};
	fullscreen_desc.Windowed = true;

	// Create swap chain (front and back buffers)
	IDXGISwapChain1* swap_chain;
	check_hresult(d3d->dxgi_factory->CreateSwapChainForHwnd(d3d->device, display->wnd, &desc, &fullscreen_desc, nullptr, &swap_chain));

	// Cast COM pointer to newer version of API
	d3d->dxgi_swap_chain = com_cast<IDXGISwapChain3>(swap_chain);
	swap_chain->Release();

	// Tell DXGI to not queue up multiple frames
	check_hresult(d3d->dxgi_swap_chain->SetMaximumFrameLatency(1));

	// Prevent exclusive fullscreen mode and ignore ALT+ENTER shortcut as we will fullscreen
	check_hresult(d3d->dxgi_factory->MakeWindowAssociation(display->wnd, DXGI_MWA_NO_ALT_ENTER));

	// Create back buffer render target
	ID3D11Texture2D* bb_texture;
	check_hresult(d3d->dxgi_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb_texture));
	check_hresult(d3d->device->CreateRenderTargetView(bb_texture, nullptr, &d3d->back_buffer));

	// Decrement reference counts of temporary objects
	adapter->Release();
	bb_texture->Release();
}

/*
	Initialises D3D and creates the swap chain.
*/
void init_d3d(d3d_context* d3d, display_context* display, bool debug)
{
	// Initialise the D3D device creation flags
	uint32_t creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT; // Allows interfacing with Direct2D
	if (debug)
		creation_flags |= D3D11_CREATE_DEVICE_DEBUG;			// Enabled D3D debug output

	// Describe the GFX features we want (currently set to use latest features)
	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_1;

	// Create a hardware accelerated D3D device and immediate context that supports latest GFX cards
	ID3D11Device* device;
	ID3D11DeviceContext* context;
	check_hresult(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creation_flags, &feature_level, 1, D3D11_SDK_VERSION, &device, nullptr, &context));

	// Cast COM pointers to newer vrsions of the API
	d3d->device = com_cast<ID3D11Device5>(device);
	d3d->context = com_cast<ID3D11DeviceContext4>(context);

	// Decrement reference counts of temporary objects
	device->Release();
	context->Release();

	create_swap_chain(d3d, display);
}

/*
	Decrements all D3D and DXGI onject reference counts to enable a clean shutdown.
*/
void term_d3d(d3d_context* d3d)
{
	d3d->dxgi_device->Release();
	d3d->dxgi_adapter->Release();
	d3d->dxgi_factory->Release();
	d3d->dxgi_swap_chain->Release();
	d3d->device->Release();
	d3d->context->Release();
	d3d->back_buffer->Release();
}

static ID3DBlob* compile_shader(const char* shader_text, size_t size, shader_type type, const char* entry_function)
{
	const char* target;
	switch (type)
	{
	case shader_type_vertex:
		target = "vs_5_0";
		break;
	case shader_type_pixel:
		target = "ps_5_0";
		break;
	}

	ID3DBlob* shader_blob;
	ID3DBlob* error_blob;
	const HRESULT hr = D3DCompile2(
		shader_text,
		size,
		"HLSL",
		nullptr,
		nullptr,
		entry_function,
		target,
		0,
		0,
		0,
		nullptr,
		0,
		&shader_blob,
		&error_blob
	);

	if (hr != 0)
	{
		debug_printf("Shader compilation error: %s\n", (const char*)error_blob->GetBufferPointer());
		debug_break();

		error_blob->Release();
	}

	return shader_blob;
}

ID3D11VertexShader* compile_vertex_shader(ID3DBlob** blob, d3d_context* d3d, const char* shader_text, size_t size, const char* entry_function)
{
	// Compile the shader into byte code
	*blob = compile_shader(shader_text, size, shader_type_vertex, entry_function);

	// Create the vertex shader object
	ID3D11VertexShader* vs;
	check_hresult(d3d->device->CreateVertexShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize(), nullptr, &vs));

	// Compiled shader no longer required so clean up by decrementing its reference count
	//blob->Release();

	return vs;
}

ID3D11PixelShader* compile_pixel_shader(d3d_context* d3d, const char* shader_text, size_t size, const char* entry_function)
{
	// Compile the shader into byte code
	ID3DBlob* blob = compile_shader(shader_text, size, shader_type_pixel, entry_function);

	// Create the vertex shader object
	ID3D11PixelShader* ps;
	check_hresult(d3d->device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &ps));

	// Compiled shader no longer required so clean up by decrementing its reference count
	blob->Release();

	return ps;
}

void* read_entire_file(const char* path, size_t* size)
{
	HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

	if (file != INVALID_HANDLE_VALUE)
	{
		FILE_STANDARD_INFO standard_info;
		GetFileInformationByHandleEx(file, FileStandardInfo, &standard_info, sizeof(standard_info));

		const uint32_t file_size = (uint32_t)standard_info.EndOfFile.QuadPart;

		void* data = malloc(file_size);

		ReadFile(file, data, file_size, nullptr, nullptr);

		CloseHandle(file);

		if (size)
			*size = file_size;

		return data;
	}

	return nullptr;
}

void load_png(d3d_context* d3d, texture* t, void* data, size_t size)
{
	IWICImagingFactory* factory;
	check_hresult(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));

	IWICStream* stream;
	check_hresult(factory->CreateStream(&stream));

	check_hresult(stream->InitializeFromMemory((uint8_t*)data, (uint32_t)size));

	IWICBitmapDecoder* decoder;
	check_hresult(factory->CreateDecoderFromStream(stream, &GUID_VendorMicrosoftBuiltIn, WICDecodeMetadataCacheOnLoad, &decoder));

	IWICBitmapFrameDecode* frame;
	check_hresult(decoder->GetFrame(0, &frame));

	check_hresult(frame->GetSize(&t->width, &t->height));

	const uint32_t mem_size = t->width * t->height * 4;
	uint8_t* decoded_data = (uint8_t*)malloc(mem_size);

	check_hresult(frame->CopyPixels(nullptr, t->width * 4, mem_size, decoded_data));

	D3D11_TEXTURE2D_DESC texture_desc = {};
	texture_desc.Width = t->width;
	texture_desc.Height = t->height;
	texture_desc.MipLevels = 1;
	texture_desc.ArraySize = 1;
	texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texture_desc.SampleDesc.Count = 1;
	texture_desc.SampleDesc.Quality = 0;
	texture_desc.Usage = D3D11_USAGE_IMMUTABLE;
	texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texture_desc.CPUAccessFlags = 0;
	texture_desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA texture_data = {};
	texture_data.pSysMem = decoded_data;
	texture_data.SysMemPitch = t->width * 4;

	check_hresult(d3d->device->CreateTexture2D(&texture_desc, &texture_data, &t->buffer));

	free(decoded_data);

	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = texture_desc.Format;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.MipLevels = (uint32_t)-1;

	check_hresult(d3d->device->CreateShaderResourceView(t->buffer, &srv_desc, &t->srv));
}

/*
	Call each frame before submitting render commands.
*/
void begin_frame(d3d_context* d3d)
{
	// Set the current render target to the back buffer
	d3d->context->OMSetRenderTargets(1, &d3d->back_buffer, nullptr);

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (float)d3d->display->width;
	viewport.Height = (float)d3d->display->height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	d3d->context->RSSetViewports(1, &viewport);
}

/*
	Specifies the number of completed frames to buffer up before forcing the CPU to wait for the
	vertical syncronisation signal (VSYNC) from the display monitor.

	Waiting zero frames will provide the lowest latency but causes rendering artefacts because new
	frames will be displayed before the monitor has finished displaying the previous frame. Waiting
	one or more frames will cause the CPU to wait until the monitor has finished displaying the
	previous frame by waiting for a VSYNC signal. Using more than one frame allows for potentially
	higher GPU utilisation and can smooth out frame spikes, but can add latency between the time
	when a button is pressed to seeing the results on the screen.
*/
enum vsync_wait_frames : uint32_t
{
	vsync_wait_frames_0,	// Present frame immediately without synchronisation
	vsync_wait_frames_1,	// Present frame after waiting for next VSYNC
	vsync_wait_frames_2,	// Present frame after waiting 2 VSYNCs
	vsync_wait_frames_3,	// Present frame after waiting 3 VSYNCs
	vsync_wait_frames_4		// Present frame after waiting 4 VSYNCs
};

/*
	Call each frame after submitting the lsst render command.
*/
void end_frame(d3d_context* d3d)
{
	// Swap front and back buffers after waiting for next VSYNC
	d3d->dxgi_swap_chain->Present(vsync_wait_frames_1, 0);
}