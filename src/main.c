#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <glm/glm.h>

#define WIDTH 960
#define HEIGHT 544

#define WS_ENGLISH_CHARS U"\x20!\x22#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\x5C]^_`abcdefghijklmnopqrstuvwxyz{|}~"
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

size_t read_entire_file(const char *path, uint8_t **data)
{
	FILE *f = fopen(path, "rb");

	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		long len = ftell(f);
		fseek(f, 0, SEEK_SET);

		*data = malloc(len);
		fread(*data, 1, len, f);
		return len;
	}

	return 0;
}

//#define STB_RECT_PACK_IMPLEMENTATION
//#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_RASTERIZER_VERSION 1
#define STBTT_STATIC
#include "stb_truetype.h"
#include "nes_cpu.h"
#include "debugger.h"

/*
typedef struct font_glyph
{
	GLuint tex;
	uint32_t codePoint;
} font_glyph;
*/

enum GlyphStyle {
	EXTRA_LIGHT,
	LIGHT,
	MEDIUM,
	SEMI_BOLD,
	BOLD,
};

typedef struct Glyph
{
	float u0, v0, u1, v1;
	unsigned int codepoint;

} Glyph;


typedef struct OpenGLShader { GLuint id; } OpenGLShader;

typedef struct FontAtlas
{
	GLuint textureID;
	Glyph *glyphs;
	short glyphCount;
	short width;
	short height;
	short pixelHeight;
	stbtt_fontinfo font;
} FontAtlas;

/*
FontAtlas CreateFontAtlas(stbtt_fontinfo *font, const wchar_t *chars, short pixelHeight, short width, short height, bool filter) {
	FontAtlas fontAtlas;

	short len = wcslen(chars);
	fontAtlas.glyphCount = len;
	fontAtlas.glyphs = malloc(len * sizeof(Glyph));
	float scale = stbtt_ScaleForPixelHeight(font, pixelHeight);
	float scale_x =scale;
	float scale_y = scale;
	fontAtlas.pixelHeight = pixelHeight;
	fontAtlas.font = *font;

	stbrp_context ctx;

	stbrp_node nodes[width];
	//stbrp_setup_allow_out_of_mem(&ctx, 1);

	stbrp_init_target(&ctx, width, height, nodes, ARRAY_SIZE(nodes));

	stbrp_rect rects[len];

	for (short i = 0; i < len; ++i) {
		wchar_t cp = chars[i];

		int x0, y0, x1, y1;
		stbtt_GetCodepointBitmapBox(font, cp, scale_x, scale_y, &x0, &y0, &x1, &y1);
		int w = x1 - x0;
		int h = y1 - y0;

		rects[i].id = cp;
		rects[i].w = w;
		rects[i].h = h;
	}

	bool packSuccess = false;
	short attempt = 1;

	while (attempt <= 5)
		if (stbrp_pack_rects(&ctx, rects, ARRAY_SIZE(rects))) {
			packSuccess = true;
			break;
		}

	if (packSuccess != true)
	{
		fprintf(stderr, "%s\n", "Couldn't pack font!");
		exit(EXIT_FAILURE);
	}

	printf("Packed font with success in %d attemps!\n", attempt);


	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &fontAtlas.textureID);
	glBindTexture(GL_TEXTURE_2D, fontAtlas.textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	for (short i = 0; i < len; ++i) {
		wchar_t cp = chars[i];

		stbrp_rect *rect = &rects[i];

		int stride = width;
		int index = width * rect->y + rect->x;

		int w, h, xoff, yoff;
		unsigned char *bm = stbtt_GetCodepointBitmap(font, scale_x, scale_y, cp, &w, &h, &xoff, &yoff);
		glTexSubImage2D(GL_TEXTURE_2D, 0, rect->x, rect->y, rect->w, rect->h, GL_RED, GL_UNSIGNED_BYTE, bm);

		fontAtlas.glyphs[i].codepoint = cp;
		fontAtlas.glyphs[i].u0 = (float)rect->x / (float)width;
		fontAtlas.glyphs[i].v0 = (float)rect->y / (float)height;
		fontAtlas.glyphs[i].u1 = (float)(rect->x + rect->w) / (float)width;
		fontAtlas.glyphs[i].v1 = (float)(rect->y + rect->h) / (float)height;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	return fontAtlas;
}*/

unsigned long long wwcslen(const unsigned int *s)
{
	unsigned long long i = 0;
	while (s[i] != U'\0') ++i;
	return i;
}

FontAtlas CreateFontAtlas(stbtt_fontinfo *font, const unsigned int *chars, short pixelHeight, short width, short height, bool filter) {
	FontAtlas fontAtlas;

	unsigned long long len = wwcslen(chars);
	fontAtlas.glyphCount = len;
	fontAtlas.glyphs = malloc(len * sizeof(Glyph));
	float scale = stbtt_ScaleForPixelHeight(font, pixelHeight);
	float scale_x = scale;
	float scale_y = scale;
	fontAtlas.pixelHeight = pixelHeight;
	fontAtlas.font = *font;

	unsigned char *pixels = malloc(width * height);

	stbtt_pack_context spc;
	stbrp_rect rects[len];
	memset(rects, 0, len * sizeof(stbrp_rect));

	stbtt_pack_range range = {};
	range.first_unicode_codepoint_in_range = (int)chars[0];
	range.num_chars = (int)len;
	range.font_size = (float)pixelHeight;
	range.array_of_unicode_codepoints = (int *)chars;

	// NOTE: This took me hours to figure out that I have to allocate this array manually.
	// stb_truetype documentation is shit!
	range.chardata_for_range = malloc(len * sizeof(stbtt_packedchar));

	stbtt_PackBegin(&spc, pixels, width, height, 0, 4, NULL);

	stbtt_PackSetOversampling(&spc, 4, 4);

	stbtt_PackFontRangesGatherRects(&spc, font,  &range, 1, rects);
	stbtt_PackFontRangesPackRects(&spc, rects, (int)len);
	stbtt_PackFontRangesRenderIntoRects(&spc, font,  &range, 1, rects);

	stbtt_PackEnd(&spc);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &fontAtlas.textureID);
	glBindTexture(GL_TEXTURE_2D, fontAtlas.textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	//glGenerateMipmap(GL_TEXTURE_2D);

	for (unsigned long long i = 0; i < len; ++i)
	{
		Glyph *g = &fontAtlas.glyphs[i];
		g->codepoint = chars[i];
		g->u0 = (float)rects[i].x / (float)width;
		g->v0 = (float)rects[i].y / (float)height;
		g->u1 = (float)(rects[i].x + rects[i].w) / (float)width;
		g->v1 = (float)(rects[i].y + rects[i].h) / (float)height;
	}

	return fontAtlas;
}


/*
font_glyph create_font_glyph(stbtt_fontinfo *font, int codepoint, float scale_x, float scale_y, bool linear_filter)
{
	font_glyph glyph;
	int x0, y0, x1, y1;
	unsigned char *glyph_data;

	stbtt_GetCodepointBitmapBox(font, codepoint, scale_x, scale_y, &x0, &y0, &x1, &y1);

	int w = x1 - x0;
	int h = y1 - y0;

	glyph_data = malloc(w * h);

	stbtt_MakeCodepointBitmap(font, glyph_data, w, h, w, scale_x, scale_y, codepoint);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &glyph.tex);
	glBindTexture(GL_TEXTURE_2D, glyph.tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, glyph_data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear_filter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear_filter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glBindTexture(GL_TEXTURE_2D, 0);

	free(glyph_data);

	return glyph;
}

FontAtlas create_ascii_font_atlas(stbtt_fontinfo *font)
{

}
*/

GLuint create_shader_nzt(const char *string, int len, GLenum type) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar **)&string, (GLint *)&len);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLsizei maxLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
		GLchar messageLog[maxLength];
		glGetShaderInfoLog(shader, maxLength, NULL, messageLog);
		fprintf(stderr, "Failed to compile shader:\n %s\n", messageLog);
	}

	return shader;
}

OpenGLShader create_shader_program(const char *vert_path, const char *frag_path) {
	OpenGLShader shader;

	char *vert_string = NULL;
	size_t vert_string_len = read_entire_file(vert_path, (uint8_t **)&vert_string);
	GLuint vert_shader = create_shader_nzt(vert_string, vert_string_len, GL_VERTEX_SHADER);
	free(vert_string);

	char *frag_string = NULL;
	size_t frag_string_len = read_entire_file(frag_path, (uint8_t **)&frag_string);
	GLuint frag_shader = create_shader_nzt(frag_string, frag_string_len, GL_FRAGMENT_SHADER);
	free(frag_string);

	shader.id = glCreateProgram();

	glAttachShader(shader.id, vert_shader);
	glAttachShader(shader.id, frag_shader);
	glLinkProgram(shader.id);

	glDetachShader(shader.id, vert_shader);
	glDetachShader(shader.id, frag_shader);
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	return shader;
}

typedef struct AxisAlignedBoundingBox2D
{
	float x0;
	float y0;
	float x1;
	float y1;
} AxisAlignedBoundingBox2D;

glm_mat4 Matrix_From_AxisAlignedBoundingBox2D(AxisAlignedBoundingBox2D *box)
{
	glm_mat4 m;

	float sx = (box->x1 - box->x0) * 0.5f;
	float sy = (box->y1 - box->y0) * 0.5f;
	float tx = 0.5f * (box->x0 + box->x1);
	float ty = 0.5f * (box->y0 + box->y1);
	
	m.elem[0][0] = sx;
	m.elem[1][0] = 0.0f;
	m.elem[2][0] = 0.0f;
	m.elem[3][0] = tx;

	m.elem[0][1] = 0.0f;
	m.elem[1][1] = sy;
	m.elem[2][1] = 0.0f;
	m.elem[3][1] = ty;

	m.elem[0][2] = 0.0f;
	m.elem[1][2] = 0.0f;
	m.elem[2][2] = 1.0f;
	m.elem[3][2] = 0.0f;

	m.elem[0][3] = 0.0f;
	m.elem[1][3] = 0.0f;
	m.elem[2][3] = 0.0f;
	m.elem[3][3] = 1.0f;

	return m;
}

typedef struct Vertex {
	glm_vec3 pos;
	glm_vec2 uv;
	glm_vec3 color;
} Vertex;
/*
typedef struct mesh {
	GLuint vao;
	GLuint vbo;
	long drawCount;
} mesh;

mesh create_text_mesh(const text_vertex *verts, long len)
{
	mesh m;

	glGenVertexArrays(1, &m.vao);
	glBindVertexArray(m.vao);

	glGenBuffers(1, &m.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
	glBufferData(GL_ARRAY_BUFFER, len * sizeof(text_vertex), verts, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(text_vertex), (void *)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, sizeof(text_vertex), (void *)offsetof(text_vertex, uv));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, sizeof(text_vertex), (void *)offsetof(text_vertex, color));

	m.drawCount = len;

	return m;
}
void render_mesh(mesh *m)
{
	glBindVertexArray(m->vao);
	glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
	glDrawArrays(GL_TRIANGLES, 0, m->drawCount);
}

*/

bool FontAtlas_GetGlyph(const FontAtlas *atlas, const wchar_t c, Glyph *glyph) {
	for (short i = 0; i < atlas->glyphCount; ++i) {
		if (atlas->glyphs[i].codepoint == c) {
			*glyph = atlas->glyphs[i];
			return true;
		}
	}

	return false;
}

void RenderText_FontAtlas(const FontAtlas *atlas, const OpenGLShader *shader, const wchar_t *text, glm_vec2 start, glm_vec3 color) {
	unsigned long long slen = wcslen(text);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, atlas->textureID);

	glUniform1i(glGetUniformLocation(shader->id, "inFontAtlas"), 0);

	Vertex vertices[slen * 4];
	unsigned int indices[slen * 6];
	Glyph glyph;
	float scale = stbtt_ScaleForPixelHeight(&atlas->font, atlas->pixelHeight);
	float xpos = 2.0f; // Leave a little padding in case the character extends left
	int i,j,ascent,baseline;

	stbtt_GetFontVMetrics(&atlas->font, &ascent,0,0);
	baseline = (int) (ascent*scale);

	for (long ch = 0; ch < slen; ++ch) {
		long vi = ch * 4, f = ch * 6;
		wchar_t c = text[ch];

		FontAtlas_GetGlyph(atlas, c, &glyph);
		int advance, lsb,x0,y0,x1,y1;
		float x_shift = xpos - (float) floor(xpos);
		stbtt_GetCodepointHMetrics(&atlas->font, c, &advance, &lsb);
		stbtt_GetCodepointBitmapBoxSubpixel(&atlas->font, c, scale, scale, x_shift, 0, &x0,&y0,&x1,&y1);

		float u0 = glyph.u0;
		float v0 = glyph.v0;
		float u1 = glyph.u1;
		float v1 = glyph.v1;
		
		vertices[vi + 0] = (Vertex) { { start.x + (float)x0 + xpos, start.y + -(float)y0, 0.0f }, { u0, v0 }, color };
		vertices[vi + 1] = (Vertex) { { start.x + (float)x1 + xpos,start.y +  -(float)y0, 0.0f }, { u1, v0 }, color };
		vertices[vi + 2] = (Vertex) { { start.x + (float)x0 + xpos, start.y + -(float)y1, 0.0f }, { u0, v1 }, color };
		vertices[vi + 3] = (Vertex) { { start.x + (float)x1 + xpos, start.y + -(float)y1, 0.0f }, { u1, v1 }, color };
		indices[f + 0] = vi + 0;
		indices[f + 1] = vi + 1;
		indices[f + 2] = vi + 2;
		indices[f + 3] = vi + 2;
		indices[f + 4] = vi + 1;
		indices[f + 5] = vi + 3;

		xpos += (advance * scale);
		if (text[ch+1])
			xpos += scale*stbtt_GetCodepointKernAdvance(&atlas->font, text[ch],text[ch+1]);
	}

	// Create OpenGL temporary VAO and render it
	GLuint vao, vbos[2];
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(2, &vbos[0]);

	// VBO
	glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, slen * 4 * sizeof(Vertex), vertices, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void *)offsetof(Vertex, uv));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void *)offsetof(Vertex, color));

	// IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, slen * 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	// Render the VAO
	glDrawElements(GL_TRIANGLES, slen * 6, GL_UNSIGNED_INT, NULL);
	glFlush();

	glDeleteBuffers(2, &vbos[0]);
	glDeleteVertexArrays(1, &vao);
}


size_t cslen_nospace(const char *s)
{
	size_t n;
	while (*s != '\0')
	{
		if (*s != ' ' && *s != '\t' && *s != '\n')
			++ n;
	}

	return n;
}

typedef struct VertexArray
{
	Vertex *vertices;
	size_t vertexCount;
	unsigned int *indices;
	size_t indexCount;
} VertexArray;

void mesh_push_text_quad(VertexArray *va, AxisAlignedBoundingBox2D *box, glm_vec3 color)
{
	va->vertices = realloc(va->vertices, va->vertexCount + 4);
	va->indices = realloc(va->indices, va->indexCount + 6);


}

/*
void render_code_line()
{

}
*/

void RenderText_FontAtlas_ASCII(const FontAtlas *atlas, const OpenGLShader *shader, const char *text, glm_vec2 start, glm_vec3 color)
{
	unsigned long long slen = strlen(text);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, atlas->textureID);

	glUniform1i(glGetUniformLocation(shader->id, "inFontAtlas"), 0);

	Vertex vertices[slen * 4];
	unsigned int indices[slen * 6];
	Glyph glyph;
	float scale = stbtt_ScaleForPixelHeight(&atlas->font, atlas->pixelHeight);
	float xpos = 0.0f; // Leave a little padding in case the character extends left
	int i,j,ascent,baseline;

	stbtt_GetFontVMetrics(&atlas->font, &ascent,0,0);
	baseline = (int) (ascent*scale);

	for (long ch = 0; ch < slen; ++ch) {
		long vi = ch * 4, f = ch * 6;
		char c = text[ch];

		FontAtlas_GetGlyph(atlas, c, &glyph);
		int advance, lsb,x0,y0,x1,y1;
		float x_shift = xpos - (float) floor(xpos);
		stbtt_GetCodepointHMetrics(&atlas->font, c, &advance, &lsb);
		stbtt_GetCodepointBitmapBoxSubpixel(&atlas->font, c, scale, scale, x_shift, 0, &x0,&y0,&x1,&y1);

		float u0 = glyph.u0;
		float v0 = glyph.v0;
		float u1 = glyph.u1;
		float v1 = glyph.v1;

		vertices[vi + 0] = (Vertex) { { floor(start.x) + (float)x0 + xpos, -(float)y0 + floor(start.y), 0.0f }, { u0, v0 }, color };
		vertices[vi + 3] = (Vertex) { { floor(start.x) + (float)x1 + xpos, -(float)y1 + floor(start.y), 0.0f }, { u1, v1 }, color };
		vertices[vi + 1] = (Vertex) { { floor(start.x) + (float)x1 + xpos, -(float)y0 + floor(start.y), 0.0f }, { u1, v0 }, color };
		vertices[vi + 2] = (Vertex) { { floor(start.x) + (float)x0 + xpos, -(float)y1 + floor(start.y), 0.0f }, { u0, v1 }, color };
		indices[f + 0] = vi + 0;
		indices[f + 1] = vi + 1;
		indices[f + 2] = vi + 2;
		indices[f + 3] = vi + 2;
		indices[f + 4] = vi + 1;
		indices[f + 5] = vi + 3;

		xpos += (advance * scale);
		if (text[ch+1])
			xpos += scale*stbtt_GetCodepointKernAdvance(&atlas->font, text[ch],text[ch+1]);
	}

	// Create OpenGL temporary VAO and render it
	GLuint vao, vbos[2];
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(2, &vbos[0]);

	// VBO
	glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, slen * 4 * sizeof(Vertex), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void *)offsetof(Vertex, uv));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void *)offsetof(Vertex, color));

	// IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, slen * 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	// Render the VAO
	glDrawElements(GL_TRIANGLES, slen * 6, GL_UNSIGNED_INT, NULL);
	glFlush();

	glDeleteBuffers(2, &vbos[0]);
	glDeleteVertexArrays(1, &vao);
}

enum OpenGLMesh_VBO
{
	VERTEX_BUFFER,
	INDEX_BUFFER
};

typedef struct OpenGLMesh
{
	GLuint vao;
	GLuint vbos[2];
	size_t drawCount;
} OpenGLMesh;

OpenGLMesh create_mesh(size_t vertexCount, size_t indexCount, Vertex *vertices, uint32_t *indices)
{
	OpenGLMesh mesh;

	mesh.drawCount = indexCount;

	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);
	glGenBuffers(2, &mesh.vbos[0]);

	// VBO
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[VERTEX_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void *)offsetof(Vertex, uv));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void *)offsetof(Vertex, color));

	// IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[INDEX_BUFFER]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	return mesh;
}

void render_mesh(OpenGLMesh *mesh)
{
	glBindVertexArray(mesh->vao);
	glDrawElements(GL_TRIANGLES, mesh->drawCount, GL_UNSIGNED_INT, NULL);
}

glm_mat4 GetOrthographicMatrix(float left, float right, float bottom, float top, float far, float near)
{
	glm_mat4 m;

	float tx = - (right + left) / (right - left);
	float ty = - (top + bottom) / (top - bottom);
	float tz = - (far + near) / (far - near);

	float sx = 2.0f / (right - left);
	float sy = 2.0f / (top - bottom);
	float sz = -2.0f / (far - near);

	m.elem[0][0] = sx;
	m.elem[1][0] = 0.0f;
	m.elem[2][0] = 0.0f;
	m.elem[3][0] = tx;

	m.elem[0][1] = 0.0f;
	m.elem[1][1] = sy;
	m.elem[2][1] = 0.0f;
	m.elem[3][1] = ty;

	m.elem[0][2] = 0.0f;
	m.elem[1][2] = 0.0f;
	m.elem[2][2] = sz;
	m.elem[3][2] = tz;

	m.elem[0][3] = 0.0f;
	m.elem[1][3] = 0.0f;
	m.elem[2][3] = 0.0f;
	m.elem[3][3] = 1.0f;

	return m;
}

static float scrollY = 0.0f;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	scrollY += (float)yoffset * 2.5f;
}

#define SPACING_COEFF_X 1.25f
#define DEBUGGER_ADDR_PANEL_WIDTH 80.0f
#define DEBUGGER_CODE_MARGIN_X 16.0f

const glm_vec3 DEBUGGER_SELECTION_COLOR = (glm_vec3){0.7f, 0.7f, 0.7f};
const glm_vec3 DEBUGGER_STEP_COLOR = (glm_vec3){0.75f, 0.3f, 0.3f};
const glm_vec3 DEBUGGER_BYTECODE_COLOR = (glm_vec3){0.62f, 0.58f, 0.45f};
const glm_vec3 DEBUGGER_ADDR_PANEL_COLOR = (glm_vec3){0.16f, 0.16f, 0.16f};
const glm_vec3 DEBUGGER_CODE_BACKGROUND_COLOR = (glm_vec3){0.18f,0.18f,0.18f};

AxisAlignedBoundingBox2D calculate_mos6502_assembly_line_bounds(
	const AxisAlignedBoundingBox2D *extents,
	const size_t lineIndex,
	const float pxRowHeight,
	const float pxOffsetY)
{
	AxisAlignedBoundingBox2D box;

	float y = 0.5f * (extents->y0 + extents->y1) + 0.25f * pxRowHeight -/*((float)lineIndex - 0.5f) * pxRowHeight -*/ pxOffsetY;

	float x0 = extents->x0 + DEBUGGER_ADDR_PANEL_WIDTH;

	box.x0 = x0;
	box.y0 = y - 0.5f * pxRowHeight;
	box.y1 = y + 0.5f * pxRowHeight;
	box.x1 = extents->x1;

	return box;
}

int main(int argc, char *argv[])
{
	/* Zero out registers, init CPU */
	nes_init_cpu();

	/* Check if only one argument after file name */
	if (argc != 2)
	{
		fprintf(stderr, "error: Invalid usage. USAGE:\n./nes_cpu [FILE]\n");
		return -1;
	}
	else
	{
		/* Load the rom into NES memory */
		if (nes_load_rom(argv[1], &nes_cartridge) != 0)
		{
			return -1;
		}
	}

	init_debugger(0x8000U, 0xFFFFU);
	debugger_disassemble();

	glfwInit();

	int width = WIDTH, height = HEIGHT;

	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

	GLFWwindow *window = glfwCreateWindow(width, height, "anime tiddies (.Y.)", NULL, NULL);

	if (window == NULL)
	{
		fprintf(stderr, "Failed to create window\n");
		return EXIT_FAILURE;
	}

	printf("Created GLFW window.\n");

	glfwSetScrollCallback(window, scroll_callback);

	glfwMakeContextCurrent(window);

	if (!gladLoadGL())
	{
		fprintf(stderr, "Failed to load GLAD.\n");
	}

	printf("Initialized OpenGL.\n");
	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("OpenGL vendor: %s\n", glGetString(GL_VENDOR));
	printf("OpenGL renderer: %s\n", glGetString(GL_RENDERER));

	// Load fonts
	//font_glyph g;
	FontAtlas fontAtlas;
	stbtt_fontinfo font;
	uint8_t *ttf_buffer;
	size_t ttf_buffer_len = read_entire_file("../data/fonts/IBMPlexSans-Light.ttf", &ttf_buffer);

	if (ttf_buffer_len > 0) {
		// Read font succesfully
		printf("Loaded truetype font.\n");
		stbtt_InitFont(&font, ttf_buffer, 0);
		//g = create_font_glyph(&font, 'g', 0.2f, 0.2f, true);

		fontAtlas = CreateFontAtlas(&font, WS_ENGLISH_CHARS, 20, 512, 512, true);
	}

	//free(ttf_buffer);

	Vertex quadVertices[4] = {
		(Vertex){ {-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f} },
		(Vertex){ {1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f} },
		(Vertex){ {-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f} },
		(Vertex){ {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f} },
	};

	unsigned int quadIndices[6] = { 0, 1, 2, 2, 1, 3 };
	
	OpenGLMesh quadMesh = create_mesh(4, 6, quadVertices, quadIndices);

	OpenGLShader fontShader = create_shader_program("../data/shaders/truetype_render.vert",
		"../data/shaders/truetype_render.frag");

	OpenGLShader colorShader = create_shader_program("../data/shaders/color.vert", "../data/shaders/color.frag");

	printf("Created shader truetype_render.\n");

/*
	// Create quad mesh
	static const text_vertex quad_vertices[6] = {
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f } },
		{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f } },
	};

	mesh quad_mesh = create_text_mesh(&quad_vertices[0], 6);
*/

	glm_vec3 background = glm_vec3(0.24f);

	glClearColor(background.r, background.g, background.b, 1.0f);

	static uint16_t lineIndex = 0;
	static int curr_state = GLFW_RELEASE, prev_state;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0,0,width,height);

		prev_state = curr_state;
		curr_state = glfwGetKey(window, GLFW_KEY_SPACE);

		if (curr_state == GLFW_RELEASE && prev_state == GLFW_PRESS)
		{
			// TO-DO: debugger_step() the debugger
		    //curr_line ++;
			debugger_step(&lineIndex);
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Render shit
		glm_mat4 proj = GetOrthographicMatrix(0.0f, width, 0.0f, height, 0.0f, 1.0f);//{2.0f/(float)WIDTH, 0.0f, 0.0f, 0.0f, 2.0f/(float)HEIGHT, 0.0f, 0.0f, 0.0f, 1.0f};//Matrix_From_AxisAlignedBoundingBox2D(&box);

		AxisAlignedBoundingBox2D codeBoxExtents = { 8, 8, width - 8, height - (64 + 8) };

		{
			// Render addr numbering gutter
			glm_vec4 color;
			glm_mat4 model;

			AxisAlignedBoundingBox2D gutterBox;
			gutterBox.x0 = codeBoxExtents.x0;
			gutterBox.y0 = codeBoxExtents.y0;
			gutterBox.x1 = codeBoxExtents.x0 + DEBUGGER_ADDR_PANEL_WIDTH;
			gutterBox.y1 = codeBoxExtents.y1;

			model = Matrix_From_AxisAlignedBoundingBox2D(&gutterBox);

			glUseProgram(colorShader.id);
			glUniformMatrix4fv(glGetUniformLocation(colorShader.id, "uProjection"), 1, GL_FALSE, &proj.elem[0][0]);
			glUniformMatrix4fv(glGetUniformLocation(colorShader.id, "uModel"), 1, GL_FALSE, &model.elem[0][0]);

			color.rgb = DEBUGGER_ADDR_PANEL_COLOR;
			color.a = 1.0f;

			glUniform4fv(glGetUniformLocation(colorShader.id, "uColor"), 1, &color.elem[0]);
			render_mesh(&quadMesh);

			AxisAlignedBoundingBox2D codeBox;
			codeBox.x0 = codeBoxExtents.x0 + DEBUGGER_ADDR_PANEL_WIDTH;
			codeBox.x1 = codeBoxExtents.x1;
			codeBox.y0 = codeBoxExtents.y0;
			codeBox.y1 = codeBoxExtents.y1;

			model = Matrix_From_AxisAlignedBoundingBox2D(&codeBox);
			color.rgb = DEBUGGER_CODE_BACKGROUND_COLOR;
			color.a = 1.0f;

			glUniform4fv(glGetUniformLocation(colorShader.id, "uColor"), 1, &color.elem[0]);

			glUniformMatrix4fv(glGetUniformLocation(colorShader.id, "uModel"), 1, GL_FALSE, &model.elem[0][0]);
			render_mesh(&quadMesh);
		}


		{
			// Render text
			glUseProgram(fontShader.id);

			//float aspect = (float)HEIGHT / (float)WIDTH;
			//AxisAlignedBoundingBox2D box = { -aspect, -1.0f, aspect, 1.0f };
			
			glUniformMatrix4fv(glGetUniformLocation(fontShader.id, "inProjection"), 1, GL_FALSE, &proj.elem[0][0]);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fontAtlas.textureID);
			glUniform1i(glGetUniformLocation(fontShader.id, "uFontAtlas"), 0);

			char tmp[32];

			sprintf(tmp, "A: 0x%02X", nes_cpu_registers.A);
			RenderText_FontAtlas_ASCII(&fontAtlas, &fontShader, tmp, (glm_vec2) {10, height - fontAtlas.pixelHeight}, (glm_vec3){1.0f, 1.0f, 1.0f});

			sprintf(tmp, "X: 0x%02X", nes_cpu_registers.X);

			RenderText_FontAtlas_ASCII(&fontAtlas, &fontShader, tmp, (glm_vec2) {110, height - fontAtlas.pixelHeight}, (glm_vec3){1.0f, 1.0f, 1.0f});
			
			sprintf(tmp, "Y: 0x%02X", nes_cpu_registers.Y);

			RenderText_FontAtlas_ASCII(&fontAtlas, &fontShader, tmp, (glm_vec2) {210, height - fontAtlas.pixelHeight}, (glm_vec3){1.0f, 1.0f, 1.0f});

			sprintf(tmp, "S: 0x%02X", nes_cpu_registers.S);

			RenderText_FontAtlas_ASCII(&fontAtlas, &fontShader, tmp, (glm_vec2) {310, height - fontAtlas.pixelHeight}, (glm_vec3){1.0f, 1.0f, 1.0f});

			sprintf(tmp, "SP: 0x%02X", nes_cpu_registers.SP);

			RenderText_FontAtlas_ASCII(&fontAtlas, &fontShader, tmp, (glm_vec2) {410, height - fontAtlas.pixelHeight}, (glm_vec3){1.0f, 1.0f, 1.0f});

			sprintf(tmp, "PC: 0x%04X", nes_cpu_registers.PC);

			RenderText_FontAtlas_ASCII(&fontAtlas, &fontShader, tmp, (glm_vec2) {510, height - fontAtlas.pixelHeight}, (glm_vec3){1.0f, 1.0f, 1.0f});
		}

		glEnable(GL_SCISSOR_TEST);
		glScissor(codeBoxExtents.x0, codeBoxExtents.y0, codeBoxExtents.x1 - codeBoxExtents.x0, codeBoxExtents.y1 - codeBoxExtents.y0);

		{
			float boxHeight = codeBoxExtents.y1 - codeBoxExtents.y0;


			long rows = (long)boxHeight / (long)fontAtlas.pixelHeight;
			long rows_rem = (long)boxHeight % (long)fontAtlas.pixelHeight;

			long hh = (rows/2+1)/2;

			long lowLine = (long) lineIndex - hh;
			long highLine = (long) lineIndex + hh;

			for (long i = lowLine; i <= highLine; ++i)
			{
				if (i >= 0 && i < debugger_line_count())
				{
					const MOS6502_CodeLine *codeLine = debugger_get_line(i);

					if (codeLine != NULL)
					{
						uint16_t addr = codeLine->addr;
						float byteCodeOffsetY = (float) boxHeight/2.0f - ((float)(i) - (float)lineIndex - 0.5f) * 2.0f * (float)fontAtlas.pixelHeight - scrollY + codeBoxExtents.y0;
						float asmOffsetY = (float) boxHeight/2.0f - ((float)(i) - (float)lineIndex) * 2.0f * (float)fontAtlas.pixelHeight - scrollY  + codeBoxExtents.y0;

						float addrTextWidth = 0.5f * (float)fontAtlas.pixelHeight * 4.0f;
						float addrOffsetX = 0.5f * (DEBUGGER_ADDR_PANEL_WIDTH - addrTextWidth);

						bool isStepLine = i == lineIndex;

						char addr_str[25];
						sprintf(addr_str, "%04X", addr);

						{
							// Render address of instruction
							glm_vec3 color = isStepLine ? DEBUGGER_STEP_COLOR : (glm_vec3){0.4f,0.4f,0.4f};
							RenderText_FontAtlas_ASCII(&fontAtlas, &fontShader, addr_str, (glm_vec2) {codeBoxExtents.x0 + addrOffsetX, asmOffsetY}, color);
						}

						float offsetX = codeBoxExtents.x0 + DEBUGGER_ADDR_PANEL_WIDTH + DEBUGGER_CODE_MARGIN_X;

						for (int i = 0; i < codeLine->instructionSize; ++i)
						{
							uint8_t b = codeLine->bytes[i];
							char byte[3] = { HEX_CHARS[(b & 0xF0U) >> 4U], HEX_CHARS[b & 0x0FU], '\0' };

							glm_vec3 color = DEBUGGER_BYTECODE_COLOR;//glm_muls_float3(DEBUGGER_BYTECODE_SELECTION_COLOR, isStepLine ? 1.0f : 0.5f);

							RenderText_FontAtlas_ASCII(&fontAtlas, &fontShader, byte, (glm_vec2) {offsetX, byteCodeOffsetY},
								color);

							offsetX += (float) fontAtlas.pixelHeight * 2.0f * 0.5f * SPACING_COEFF_X;
						}
						
						glm_vec3 color = glm_vec3(1.0f);
						RenderText_FontAtlas_ASCII(&fontAtlas, &fontShader, codeLine->assembly, (glm_vec2) {codeBoxExtents.x0 + DEBUGGER_ADDR_PANEL_WIDTH + DEBUGGER_CODE_MARGIN_X, asmOffsetY}, color);
					}
				}
			}

		}

		{
			// Highlight step assembly line

			AxisAlignedBoundingBox2D box = calculate_mos6502_assembly_line_bounds(
				&codeBoxExtents,
				lineIndex,
				fontAtlas.pixelHeight,
				scrollY);

			//.x0 += 8;
			//box.x1 -= 8;

			glm_mat4 model = Matrix_From_AxisAlignedBoundingBox2D(&box);

			glUseProgram(colorShader.id);
			glUniformMatrix4fv(glGetUniformLocation(colorShader.id, "uProjection"), 1, GL_FALSE, &proj.elem[0][0]);
			glUniformMatrix4fv(glGetUniformLocation(colorShader.id, "uModel"), 1, GL_FALSE, &model.elem[0][0]);

			glm_vec4 color;
			color.rgb = DEBUGGER_STEP_COLOR;
			color.a = 0.2f;

			glUniform4fv(glGetUniformLocation(colorShader.id, "uColor"), 1, &color.elem[0]);
			render_mesh(&quadMesh);
		}

		glDisable(GL_SCISSOR_TEST);

		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return EXIT_SUCCESS;
}