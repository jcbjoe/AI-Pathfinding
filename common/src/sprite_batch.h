struct sprite;

struct sprite_batch
{
	d3d_context*				d3d;
	ID3D11VertexShader*			vertex_shader;
	ID3D11PixelShader*			pixel_shader;
	ID3D11Buffer*				sprite_buffer;
	ID3D11ShaderResourceView*	srv;
	ID3D11SamplerState*			sampler;
	sprite*						mapped_begin;
	sprite*						mapped_end;
	sprite*						mapped_current;
	texture*					current_texture;
};

void sprite_batch_init(sprite_batch* sb, d3d_context* d3d);
void sprite_batch_term(sprite_batch* sb);
void sprite_batch_begin(sprite_batch* sb);
void sprite_batch_end(sprite_batch* sb);
void sprite_batch_draw(sprite_batch* sb, texture* t, int32_t dst_x, int32_t dst_y, int32_t dst_w, int32_t dst_h, int32_t src_x, int32_t src_y, int32_t src_w, int32_t src_h);