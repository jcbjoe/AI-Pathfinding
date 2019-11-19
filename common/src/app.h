struct display_context
{
	HWND	wnd;	// Windows API window handle
	int32_t	width;	// Width of usable window
	int32_t	height;	// Height of usable window
};

struct d3d_context
{
	display_context*		display;
	IDXGIDevice4*			dxgi_device;
	IDXGIAdapter4*			dxgi_adapter;		// Current GFX card
	IDXGIFactory5*			dxgi_factory;
	IDXGISwapChain3*		dxgi_swap_chain;	// Represents front and back buffers
	ID3D11Device5*			device;				// Used for creating D3D objects
	ID3D11DeviceContext4*	context;			// D3D immediate context for submitting draw calls
	ID3D11RenderTargetView*	back_buffer;		// Back buffer render target
};

enum shader_type
{
	shader_type_vertex,
	shader_type_pixel
};

struct texture
{
	uint32_t					width;
	uint32_t					height;
	ID3D11Texture2D*			buffer;
	ID3D11ShaderResourceView*	srv;
};

void create_display(display_context* display, const char* title, int32_t x, int32_t y, int32_t width, int32_t height, bool resizable);
void init_d3d(d3d_context* d3d, display_context* display, bool debug);
void term_d3d(d3d_context* d3d);
void begin_frame(d3d_context* d3d);
void end_frame(d3d_context* d3d);

ID3D11VertexShader*	compile_vertex_shader(ID3DBlob** blob, d3d_context* d3d, const char* shader_text, size_t size, const char* entry_function);
ID3D11PixelShader*	compile_pixel_shader(d3d_context* d3d, const char* shader_text, size_t size, const char* entry_function);

void*	read_entire_file(const char* path, size_t* size);
void	load_png(d3d_context* d3d, texture* t, void* data, size_t size);