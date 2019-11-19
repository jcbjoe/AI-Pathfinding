static const char sprite_shader[] = R"(
struct sprite
{
	float x0;
	float y0;
	float x1;
	float y1;
	float u0;
	float v0;
	float u1;
	float v1;
};

StructuredBuffer<sprite>	sprites			: register(t0);
Texture2D					sprite_texture	: register(t0);
SamplerState				sprite_sampler	: register(s0);

struct vertex_output
{
	float4 position	: SV_Position;
	float2 uv		: TEXCOORD;
};

vertex_output mainVS(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	vertex_output output;

	if (vertex_id == 0)
	{
		output.position = float4(sprites[instance_id].x0, sprites[instance_id].y0, 0.0f, 1.0f);
		output.uv = float2(sprites[instance_id].u0, sprites[instance_id].v0);
	}
	else if (vertex_id == 1)
	{
		output.position = float4(sprites[instance_id].x1, sprites[instance_id].y0, 0.0f, 1.0f);
		output.uv = float2(sprites[instance_id].u1, sprites[instance_id].v0);
	}
	else if (vertex_id == 2)
	{
		output.position = float4(sprites[instance_id].x1, sprites[instance_id].y1, 0.0f, 1.0f);
		output.uv = float2(sprites[instance_id].u1, sprites[instance_id].v1);
	}
	else if (vertex_id == 3)
	{
		output.position = float4(sprites[instance_id].x1, sprites[instance_id].y1, 0.0f, 1.0f);
		output.uv = float2(sprites[instance_id].u1, sprites[instance_id].v1);
	}
	else if (vertex_id == 4)
	{
		output.position = float4(sprites[instance_id].x0, sprites[instance_id].y1, 0.0f, 1.0f);
		output.uv = float2(sprites[instance_id].u0, sprites[instance_id].v1);
	}
	else
	{
		output.position = float4(sprites[instance_id].x0, sprites[instance_id].y0, 0.0f, 1.0f);
		output.uv = float2(sprites[instance_id].u0, sprites[instance_id].v0);
	}

	return output;
}

float4 mainPS(vertex_output input) : SV_TARGET
{
	return sprite_texture.Sample(sprite_sampler, input.uv);
}
)";

struct sprite
{
	float x0;
	float y0;
	float x1;
	float y1;
	float u0;
	float v0;
	float u1;
	float v1;
};

constexpr size_t sprite_buffer_size		= 64 * 1024;
constexpr size_t sprite_buffer_count	= sprite_buffer_size / sizeof(sprite);

static void sprite_batch_map(sprite_batch* sb)
{
	D3D11_MAPPED_SUBRESOURCE mapped_resource;
	check_hresult(sb->d3d->context->Map(sb->sprite_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource));

	sb->mapped_begin = sb->mapped_current = (sprite*)mapped_resource.pData;
	sb->mapped_end = sb->mapped_begin + sprite_buffer_count;
}

static void sprite_batch_unmap(sprite_batch* sb)
{
	sb->d3d->context->Unmap(sb->sprite_buffer, 0);
	sb->mapped_begin = sb->mapped_end = sb->mapped_current = nullptr;
}

static void sprite_batch_flush(sprite_batch* sb)
{
	if (sb->mapped_current == sb->mapped_begin)
		return;

	const uint32_t sprite_count = (uint32_t)(sb->mapped_current - sb->mapped_begin);

	if (sprite_count > 0)
	{
		sprite_batch_unmap(sb);

		sb->d3d->context->VSSetShaderResources(0, 1, &sb->srv);
		sb->d3d->context->PSSetShaderResources(0, 1, &sb->current_texture->srv);
		sb->d3d->context->PSSetSamplers(0, 1, &sb->sampler);

		sb->d3d->context->DrawInstanced(6, sprite_count, 0, 0);
	}
}

void sprite_batch_init(sprite_batch* sb, d3d_context* d3d)
{
	sb->d3d = {d3d};

	// Compile vertex and pixel shaders
	ID3DBlob* vs_blob;
	sb->vertex_shader = compile_vertex_shader(&vs_blob, d3d, sprite_shader, sizeof(sprite_shader), "mainVS");
	sb->pixel_shader = compile_pixel_shader(d3d, sprite_shader, sizeof(sprite_shader), "mainPS");
	vs_blob->Release();

	D3D11_BUFFER_DESC sprite_buffer_desc = {};
	sprite_buffer_desc.ByteWidth = sprite_buffer_size;
	sprite_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	sprite_buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	sprite_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	sprite_buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sprite_buffer_desc.StructureByteStride = sizeof(sprite);

	check_hresult(d3d->device->CreateBuffer(&sprite_buffer_desc, nullptr, &sb->sprite_buffer));

	D3D11_SHADER_RESOURCE_VIEW_DESC sprite_buffer_srv_desc = {};
	sprite_buffer_srv_desc.Format = DXGI_FORMAT_UNKNOWN;
	sprite_buffer_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	sprite_buffer_srv_desc.Buffer.FirstElement = 0;
	sprite_buffer_srv_desc.Buffer.NumElements = sprite_buffer_count;

	check_hresult(d3d->device->CreateShaderResourceView(sb->sprite_buffer, &sprite_buffer_srv_desc, &sb->srv));

	D3D11_SAMPLER_DESC sampler_desc = {};
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.MipLODBias = 0.0f;
	sampler_desc.MaxAnisotropy = 1;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	//sampler_desc.BorderColor = {1.0f, 1.0f, 1.0f, 1.0f};
	sampler_desc.MinLOD = 1;
	sampler_desc.MaxLOD = 1;

	check_hresult(d3d->device->CreateSamplerState(&sampler_desc, &sb->sampler));
}

void sprite_batch_term(sprite_batch* sb)
{
	sb->vertex_shader->Release();
	sb->pixel_shader->Release();
	sb->sprite_buffer->Release();
	sb->srv->Release();
	sb->sampler->Release();
}

void sprite_batch_begin(sprite_batch* sb)
{
	sprite_batch_map(sb);

	// Bind the shaders to the Direct3D context
	sb->d3d->context->VSSetShader(sb->vertex_shader, 0, 0);
	sb->d3d->context->PSSetShader(sb->pixel_shader, 0, 0);

	// Tell the input assembler how to interpret input data
	sb->d3d->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void sprite_batch_end(sprite_batch* sb)
{
	sprite_batch_flush(sb);
}

void sprite_batch_draw(sprite_batch* sb, texture* t, int32_t dst_x, int32_t dst_y, int32_t dst_w, int32_t dst_h, int32_t src_x, int32_t src_y, int32_t src_w, int32_t src_h)
{
	if (sb->mapped_current == sb->mapped_end || t != sb->current_texture)
	{
	 	sprite_batch_flush(sb);
	 	sprite_batch_map(sb);
	 }

	sb->current_texture = t;

	const float display_width = (float)sb->d3d->display->width;
	const float display_height = (float)sb->d3d->display->height;
	const float half_width = display_width / 2;
	const float half_height = display_height / 2;
	const float texture_width = (float)t->width;
	const float texture_height = (float)t->height;

	sprite* new_sprite = sb->mapped_current++;
	new_sprite->x0 = ((float)dst_x / half_width) - 1.0f;
	new_sprite->y0 = (((float)dst_y / half_height) * -1.0f) + 1.0f;
	new_sprite->x1 = new_sprite->x0 + ((float)dst_w / half_width);
	new_sprite->y1 = new_sprite->y0 - ((float)dst_h / half_height);
	new_sprite->u0 = (float)src_x / texture_width;
	new_sprite->v0 = (float)src_y / texture_height;
	new_sprite->u1 = new_sprite->u0 + ((float)src_w / texture_width);
	new_sprite->v1 = new_sprite->v0 + ((float)src_h / texture_height);
}