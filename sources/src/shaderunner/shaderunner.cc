/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "shaderunner/shaderunner.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <iterator>
#include <vector>

#include <boost/numeric/conversion/cast.hpp>
#include <GL/glew.h>
#include <imgui.h>

#include "utility/file.h"


#define SR_GLSL_VERSION "#version 330 core\n"
#define SR_SL_ENTRY_POINT(entry_point) "#define SR_ENTRY_POINT " entry_point "\n"
#define SR_SL_DUMMY_FKERNEL "void imageMain(inout vec4 frag_color, vec2 frag_coord) { frag_color = vec4(1.0 - float(gl_PrimitiveID), 0.0, 1.0, 1.0); } \n"
#define SR_SL_TIME_UNIFORM "iTime"
#define SR_SL_RESOLUTION_UNIFORM "iResolution"

using StdClock = std::chrono::high_resolution_clock;
template <typename Rep, typename Period>
constexpr float StdDurationToSeconds(std::chrono::duration<Rep, Period> const &_d)
{ return std::chrono::duration_cast<std::chrono::duration<float>>(_d).count(); }

namespace sr {

// =============================================================================

namespace glerror {
std::string const &GetGLErrorString(GLenum const _error);
bool PrintShaderError(std::ostream& _ostream, GLuint const _shader);
bool PrintProgramError(std::ostream& _ostream, GLuint const _program);
GLenum PrintError(std::ostream& _ostream);
bool ClearGLError();
} //namespace glerror

// =============================================================================

template <typename Deleter>
struct GLHandle
{
	GLHandle() = default;
	explicit GLHandle(GLuint _handle) : handle(_handle) {}
	GLHandle(GLHandle const &) = delete;
	GLHandle(GLHandle &&_v) : handle(_v.handle) { _v.handle = 0u; }
	~GLHandle()	{ _delete(); }
	GLHandle &operator=(GLHandle const &_v) = delete;
	GLHandle &operator=(GLHandle &&_v){ reset(_v.handle); _v.handle = 0u; return *this;}
	operator GLuint() const { return handle; }
	operator bool() const { return handle != 0u; }
	void reset(GLuint _h){ _delete(); handle = _h; }
private:
	void _delete() { if (handle) Deleter{}(handle); }
	GLuint handle = 0u;
};

struct GLProgramDeleter
{
	void operator()(GLuint _program)
	{
		std::cout << "gl program deleted" << std::endl;
		glDeleteProgram(_program);
	}
};
struct GLShaderDeleter
{
	void operator()(GLuint _shader)
	{
		std::cout << "gl shader deleted" << std::endl;
		glDeleteShader(_shader);
	}
};
struct GLTextureDeleter
{
	void operator()(GLuint _texture)
	{
		std::cout << "gl texture deleted" << std::endl;
		glDeleteTextures(1, &_texture);
	}
};

using GLProgramPtr = GLHandle<GLProgramDeleter>;
using GLShaderPtr = GLHandle<GLShaderDeleter>;
using GLTexturePtr = GLHandle<GLTextureDeleter>;

using ShaderSources_t = std::vector<char const *>;
using ShaderBinaries_t = std::vector<GLuint>;

GLShaderPtr CompileFKernel(ShaderSources_t const &_kernel_sources);
GLShaderPtr CompileShader(GLenum _type, ShaderSources_t &_sources);
GLProgramPtr LinkProgram(ShaderBinaries_t const &_binaries);

// =============================================================================

using Resolution_t = std::array<float, 2>;

// =============================================================================

class ImGuiContext
{
public:
	ImGuiContext();
	~ImGuiContext();
public:
	void Render() const;
	void SetResolution(Resolution_t const &_resolution);
public:
	GLTexturePtr font_texture_;
	GLProgramPtr shader_program_;
};

ImGuiContext::ImGuiContext() :
	font_texture_{ 0u },
	shader_program_{ 0u }
{
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	unsigned char *pixels = nullptr;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	{
		GLuint texture;
		glGenTextures(1, &texture);
		font_texture_.reset(texture);
	}
	{
		glBindTexture(GL_TEXTURE_2D, font_texture_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
					 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		assert(glGetError() == GL_NO_ERROR);
		io.Fonts->TexID = reinterpret_cast<void*>(static_cast<intptr_t>((GLuint)font_texture_));
		glBindTexture(GL_TEXTURE_2D, 0u);
	}
	{
		static ShaderSources_t vertex_shader_sources = {
			SR_GLSL_VERSION,
R"__SR_SS__(
in vec2 position; in vec2 uv; in vec4 color;
out vec2 vert_uv; out vec4 vert_color;
uniform mat4 uProjMatrix;
void main()
{
	vert_uv = uv; vert_color = color;
	gl_Position = uProjMatrix * vec4(position, 0.0, 1.0);
}
)__SR_SS__"
		};

		static ShaderSources_t fragment_shader_sources = {
			SR_GLSL_VERSION,
R"__SR_SS__(
in vec2 vert_uv; in vec4 vert_color;
out vec4 frag_color;
uniform sampler2D uTexture;
void main()
{
	frag_color = vert_color * texture(uTexture, vert_uv);
}
)__SR_SS__"
		};

		GLShaderPtr vshader = CompileShader(GL_VERTEX_SHADER, vertex_shader_sources);
		assert(vshader);
		GLShaderPtr fshader = CompileShader(GL_FRAGMENT_SHADER, fragment_shader_sources);
		assert(fshader);
		ShaderBinaries_t const shaders {
			vshader, fshader
		};
		shader_program_ = LinkProgram(shaders);
		assert(shader_program_);
	}
}

ImGuiContext::~ImGuiContext()
{
	ImGui::DestroyContext();
}

void
ImGuiContext::Render() const
{
	ImDrawData &im_draw_data = *ImGui::GetDrawData();

	glEnable(GL_BLEND);
	glEnable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glActiveTexture(GL_TEXTURE0);

	ImGuiIO &io = ImGui::GetIO();
	int const viewport_width = boost::numeric_cast<int>(
		im_draw_data.DisplaySize.x * io.DisplayFramebufferScale.x);
	int const viewport_height = boost::numeric_cast<int>(
		im_draw_data.DisplaySize.y * io.DisplayFramebufferScale.y);
	im_draw_data.ScaleClipRects(io.DisplayFramebufferScale);
	assert(viewport_width > 0 && viewport_height > 0);

	glViewport(0, 0, static_cast<GLsizei>(viewport_width), static_cast<GLsizei>(viewport_height));
	float const t = im_draw_data.DisplayPos.y;
	float const b = im_draw_data.DisplayPos.y + im_draw_data.DisplaySize.y;
	float const l = im_draw_data.DisplayPos.x;
	float const r = im_draw_data.DisplayPos.x + im_draw_data.DisplaySize.x;
	float const projection_matrix[4][4] = {
		{ 2.f / (r-l), 0.f, 0.f, 0.f },
		{ 0.f, 2.f / (t-b), 0.f, 0.f },
		{ 0.f, 0.f -1.f, 0.f },
		{ (l+r) / (l-r), (b+t) / (b-t), 0.f, 1.f }
	};

	glUseProgram(shader_program_);
	{
		GLint const proj_mat_loc = glGetUniformLocation(shader_program_, "uProjMatrix");
		assert(proj_mat_loc >= 0);
		glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, &projection_matrix[0][0]);
	}
	{
		GLint const texture_loc = glGetUniformLocation(shader_program_, "uTexture");
		assert(texture_loc >= 0);
		glUniform1i(texture_loc, 0);
	}
	glBindSampler(0, 0);

	GLuint vao = 0u, vbo = 0u, ibo = 0u;
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ibo);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	{
		GLint const position_loc = glGetAttribLocation(shader_program_, "position");
		assert(position_loc >= 0);
		glEnableVertexAttribArray(position_loc);
		GLint const uv_loc = glGetAttribLocation(shader_program_, "uv");
		assert(uv_loc >= 0);
		glEnableVertexAttribArray(uv_loc);
		GLint const color_loc = glGetAttribLocation(shader_program_, "color");
		assert(color_loc >= 0);
		glEnableVertexAttribArray(color_loc);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
							  (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
		glVertexAttribPointer(uv_loc, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
							  (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
		glVertexAttribPointer(color_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
							  (GLvoid*)IM_OFFSETOF(ImDrawVert, col));
		glBindBuffer(GL_ARRAY_BUFFER, 0u);
	}

	for (int cmd_list_index = 0; cmd_list_index < im_draw_data.CmdListsCount; ++cmd_list_index)
	{
		ImDrawList const &cmd_list = *(im_draw_data.CmdLists[cmd_list_index]);
		ImDrawIdx const *idx_buffer_offset = 0;

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER,
					 static_cast<GLsizeiptr>(cmd_list.VtxBuffer.Size) * sizeof(ImDrawVert),
					 static_cast<GLvoid const *>(cmd_list.VtxBuffer.Data),
					 GL_STREAM_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					 static_cast<GLsizeiptr>(cmd_list.IdxBuffer.Size) * sizeof(ImDrawIdx),
					 static_cast<GLvoid const *>(cmd_list.IdxBuffer.Data),
					 GL_STREAM_DRAW);

		for (int cmd_index = 0; cmd_index < cmd_list.CmdBuffer.Size; ++cmd_index)
		{
			ImDrawCmd const &cmd = cmd_list.CmdBuffer[cmd_index];
			if (cmd.UserCallback)
			{
				cmd.UserCallback(&cmd_list, &cmd);
			}
			else
			{
				ImVec4 clip_rect{
					cmd.ClipRect.x - im_draw_data.DisplayPos.x,
					cmd.ClipRect.y - im_draw_data.DisplayPos.y,
					cmd.ClipRect.z - im_draw_data.DisplayPos.x,
					cmd.ClipRect.w - im_draw_data.DisplayPos.y
				};
				if (clip_rect.x < viewport_width && clip_rect.y < viewport_height &&
					clip_rect.z >= 0.f && clip_rect.w >= 0.f)
				{
					glScissor(boost::numeric_cast<int>(clip_rect.x),
							  boost::numeric_cast<int>(viewport_height - clip_rect.w),
							  boost::numeric_cast<int>(clip_rect.z - clip_rect.x),
							  boost::numeric_cast<int>(clip_rect.w - clip_rect.y));

					glBindTexture(GL_TEXTURE_2D,
								  static_cast<GLuint>(reinterpret_cast<intptr_t>(cmd.TextureId)));
					glDrawElements(GL_TRIANGLES,
								   boost::numeric_cast<GLsizei>(cmd.ElemCount),
								   sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
								   idx_buffer_offset);
				}
			}
			idx_buffer_offset += cmd.ElemCount;
		}
	}

	glUseProgram(0u);

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ibo);

	assert(!glerror::ClearGLError());
}

void
ImGuiContext::SetResolution(Resolution_t const &_resolution)
{
	ImGuiIO &io = ImGui::GetIO();
	io.DisplaySize.x = _resolution[0];
	io.DisplaySize.y = _resolution[1];
}


// =============================================================================

struct RenderContext::Impl_
{
public:
	Impl_();
	~Impl_();

public:
	utility::File fkernel_file_;
public:
	ImGuiContext imgui_;
public:
	Resolution_t resolution_;
public:
	bool BuildFKernel(std::string const &_sources);
	GLProgramPtr shader_program_;
	GLShaderPtr cached_vshader_;
	GLShaderPtr cached_fshader_;
public:
	GLuint dummy_vao_;
};

RenderContext::Impl_::Impl_() :
	fkernel_file_{},
	imgui_{},
	resolution_{ 0.f, 0.f },
	shader_program_{ 0u },
	cached_vshader_{ 0u },
	cached_fshader_{ 0u },
	dummy_vao_{ 0u }
{
	{
		static ShaderSources_t vertex_sources{
			SR_GLSL_VERSION,
			#include "shaders/fullscreen_tri.vert.h"
		};
		cached_vshader_ = CompileShader(GL_VERTEX_SHADER, vertex_sources);

		static ShaderSources_t const default_fkernel{
			SR_SL_DUMMY_FKERNEL
		};
		cached_fshader_ = CompileFKernel(default_fkernel);

		assert(cached_vshader_);
		assert(cached_fshader_);
		ShaderBinaries_t const shader_binaries{
			cached_vshader_,
			cached_fshader_
		};
		shader_program_ = LinkProgram(shader_binaries);
		assert(shader_program_);
	}

	{
		glGenVertexArrays(1, &dummy_vao_);
	}
}

RenderContext::Impl_::~Impl_()
{
	glDeleteVertexArrays(1, &dummy_vao_);
}

bool
RenderContext::Impl_::BuildFKernel(std::string const &_sources)
{
	ShaderSources_t kernel_sources{ _sources.c_str() };
	GLShaderPtr compiled_fshader = CompileFKernel(kernel_sources);
	if (!compiled_fshader) return false;

	assert(cached_vshader_);
	ShaderBinaries_t const shader_binaries{
		cached_vshader_,
		compiled_fshader
	};
	GLProgramPtr linked_program = LinkProgram(shader_binaries);
	if (!linked_program) return false;

	cached_fshader_ = std::move(compiled_fshader);
	shader_program_ = std::move(linked_program);
	return true;
}



// =============================================================================

GLShaderPtr CompileFKernel(ShaderSources_t const &_kernel_sources)
{
	static ShaderSources_t const kKernelPrefix{
		SR_GLSL_VERSION,
		"uniform float " SR_SL_TIME_UNIFORM ";\n",
		"uniform vec2 " SR_SL_RESOLUTION_UNIFORM ";\n"
	};
	static ShaderSources_t const kKernelSuffix{
		"\n",
		SR_SL_ENTRY_POINT("imageMain"),
		#include "shaders/entry_point.frag.h"
	};

	ShaderSources_t shader_sources{};
	shader_sources.reserve(kKernelPrefix.size() + _kernel_sources.size() + kKernelSuffix.size());
	std::copy(kKernelPrefix.cbegin(), kKernelPrefix.cend(), std::back_inserter(shader_sources));
	std::copy(_kernel_sources.cbegin(), _kernel_sources.cend(), std::back_inserter(shader_sources));
	std::copy(kKernelSuffix.cbegin(), kKernelSuffix.cend(), std::back_inserter(shader_sources));

	return CompileShader(GL_FRAGMENT_SHADER, shader_sources);
}

GLShaderPtr CompileShader(GLenum _type, ShaderSources_t &_sources)
{
	GLsizei const source_count = boost::numeric_cast<GLsizei>(_sources.size());
	GLShaderPtr result{ glCreateShader(_type) };
	glShaderSource(result, source_count, _sources.data(), NULL);
	glCompileShader(result);
	if (glerror::PrintShaderError(std::cout, result))
	{
		result.reset(0u);
	}
	return result;
}

GLProgramPtr LinkProgram(ShaderBinaries_t const &_binaries)
{
	GLProgramPtr result{ glCreateProgram() };
	std::for_each(_binaries.cbegin(), _binaries.cend(), [&result](GLuint _shader) {
		glAttachShader(result, _shader);
	});
	glLinkProgram(result);
	if (glerror::PrintProgramError(std::cout, result))
	{
		result.reset(0u);
	}
	return result;
}


// =============================================================================

RenderContext::RenderContext() :
	impl_(new RenderContext::Impl_())
{}

RenderContext::~RenderContext()
{ delete impl_; }

bool
RenderContext::RenderFrame()
{
	static StdClock::time_point prev_render_time = StdClock::now();
	static StdClock::time_point begin_time = prev_render_time;
	StdClock::duration const delta_time = StdClock::now() - prev_render_time;
	float const elapsed_time = StdDurationToSeconds(prev_render_time - begin_time);
#if 0
	std::cout << StdDurationToSeconds(delta_time) << std::endl;
#endif

	{
		constexpr float kFKernelUpdatePeriod = 1.f;
		static StdClock::time_point prev_fkernel_update = StdClock::now();
		static bool prev_frame_had_fkernel_file = false;
		StdClock::duration const elapsed = StdClock::now() - prev_fkernel_update;
		if (StdDurationToSeconds(elapsed) > kFKernelUpdatePeriod)
		{
#if 0
			std::cout << "Running fkernel update.." << std::endl;
#endif
			prev_fkernel_update = StdClock::now();
			bool const fkernel_file_available = impl_->fkernel_file_.Exists();
			if (fkernel_file_available && impl_->fkernel_file_.HasChanged())
			{
				std::cout << "fkernel file changed, building.." << std::endl;
				impl_->BuildFKernel(impl_->fkernel_file_.ReadAll());
			}
			else if (!fkernel_file_available)
			{
				std::cout << "fkernel file is either nonexistent, or not a regular file" << std::endl;
			}
			else
			{
#if 0
				std::cout << "compiled fkernel is up to date" << std::endl;
#endif
			}
		}
	}

	bool start_over = true;

	assert(!glerror::ClearGLError());

	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	static GLfloat const clear_color[]{ 0.5f, 0.5f, 0.5f, 1.f };
	glClearBufferfv(GL_COLOR, 0, clear_color);

	glUseProgram(impl_->shader_program_);
	glBindVertexArray(impl_->dummy_vao_);

	{
		int const time_loc = glGetUniformLocation(impl_->shader_program_, SR_SL_TIME_UNIFORM);
		if (time_loc >= 0)
		{
			glUniform1f(time_loc, elapsed_time);
		}
		glerror::PrintError(std::cout);
	}
	{
		int const resolution_loc = glGetUniformLocation(impl_->shader_program_, SR_SL_RESOLUTION_UNIFORM);
		if (resolution_loc >= 0)
		{
			glUniform2fv(resolution_loc, 1, &(impl_->resolution_[0]));
		}
		glerror::PrintError(std::cout);
	}

	glDrawArrays(GL_TRIANGLES, 0, 3);
	start_over = start_over && (glerror::PrintError(std::cout) == GL_NO_ERROR);

	glBindVertexArray(0u);
	glUseProgram(0u);

#ifdef SR_SINGLE_BUFFERING
	glFlush();
#endif

	prev_render_time += delta_time;

	{
		impl_->imgui_.Render();
	}

	return start_over;
}


void
RenderContext::WatchFKernelFile(char const *_path)
{
	impl_->fkernel_file_ = utility::File{ _path };
	if (impl_->fkernel_file_.Exists())
		impl_->BuildFKernel(impl_->fkernel_file_.ReadAll());
}


void
RenderContext::SetResolution(int _width, int _height)
{
	impl_->resolution_ = { boost::numeric_cast<float>(_width),
						   boost::numeric_cast<float>(_height) };
	impl_->imgui_.SetResolution(impl_->resolution_);
	glViewport(0, 0, _width, _height);
}


// =============================================================================

namespace glerror {

std::string const &GetGLErrorString(GLenum const _error)
{
	static std::string const no_error{"None"};
	static std::string const invalid_enum{"Invalid enum"};
	static std::string const invalid_value{"Invalid value"};
	static std::string const invalid_op{"Invalid operation"};
	static std::string const invalid_fb_op{"Invalid framebuffer operation"};
	static std::string const out_of_memory{"Out of memory"};
	static std::string const stack_underflow{"Stack underflow"};
	static std::string const stack_overflow{"Stack overflow"};
	static std::string const unknown{"Unknown error"};

	switch(_error)
	{
	case GL_NO_ERROR:
		return no_error;
		break;
	case GL_INVALID_ENUM:
		return invalid_enum;
		break;
	case GL_INVALID_VALUE:
		return invalid_value;
		break;
	case GL_INVALID_OPERATION:
		return invalid_op;
		break;
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return invalid_fb_op;
		break;
	case GL_OUT_OF_MEMORY:
		return out_of_memory;
		break;
	case GL_STACK_UNDERFLOW:
		return stack_underflow;
		break;
	case GL_STACK_OVERFLOW:
		return stack_overflow;
		break;
	default:
		return unknown;
		break;
	}
}

bool PrintShaderError(std::ostream& _ostream, GLuint const _shader)
{
	bool result = true;
	GLint success = GL_FALSE;
	glGetShaderiv(_shader, GL_COMPILE_STATUS, &success);
	result = success != GL_TRUE;
	if (result)
	{
		_ostream << "Shader compile error : " << std::endl;
		{ // TODO: make sure char[] allocation is safe enough
			GLsizei log_size = 0;
			glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &log_size);
			assert(log_size != 0);
			char* const info_log = new char[log_size];
			glGetShaderInfoLog(_shader, log_size, NULL, info_log);
			_ostream << info_log << std::endl;
			delete[] info_log;
		}
	}
	return result;
}

bool PrintProgramError(std::ostream& _ostream, GLuint const _program)
{
	bool result = true;
	GLint success = GL_FALSE;
	glGetProgramiv(_program, GL_LINK_STATUS, &success);
	result = success != GL_TRUE;
	if (result)
	{
		_ostream << "Shader link error : " << std::endl;
		{
			GLsizei log_size = 0;
			glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &log_size);
			assert(log_size != 0);
			char* const info_log = new char[log_size];
			glGetShaderInfoLog(_program, log_size, NULL, info_log);
			_ostream << info_log << std::endl;
			delete[] info_log;
		}
	}
	return result;
}

GLenum PrintError(std::ostream& _ostream)
{
	GLenum const error_code = glGetError();
	if (error_code != GL_NO_ERROR)
		_ostream << "OpenGL error : " << GetGLErrorString(error_code).c_str() << std::endl;
	return error_code;
}

bool ClearGLError()
{
	bool const result = (glGetError() != GL_NO_ERROR);
	while(glGetError() != GL_NO_ERROR);
	return result;
}

} //namespace glerror

} //namespace sr
