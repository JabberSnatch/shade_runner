/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "uibase/imguicontext.h"

#include <boost/numeric/conversion/cast.hpp>
#include <imgui.h>

#include "oglbase/error.h"
#include "oglbase/shader.h"


namespace uibase {


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
		glGenTextures(1, font_texture_.get());
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
		static oglbase::ShaderSources_t vertex_shader_sources = {
R"__SR_SS__(
#version 330 core
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

		static oglbase::ShaderSources_t fragment_shader_sources = {
R"__SR_SS__(
#version 330 core
in vec2 vert_uv; in vec4 vert_color;
out vec4 frag_color;
uniform sampler2D uTexture;
void main()
{
	frag_color = vert_color * texture(uTexture, vert_uv);
}
)__SR_SS__"
		};

		oglbase::ShaderPtr vshader = oglbase::CompileShader(GL_VERTEX_SHADER,
															vertex_shader_sources);
		assert(vshader);
		oglbase::ShaderPtr fshader = oglbase::CompileShader(GL_FRAGMENT_SHADER,
															fragment_shader_sources);
		assert(fshader);
		oglbase::ShaderBinaries_t const shaders {
			vshader, fshader
		};
		shader_program_ = oglbase::LinkProgram(shaders);
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

	assert(!oglbase::ClearError());
}

void
ImGuiContext::SetResolution(Resolution_t const &_resolution)
{
	ImGuiIO &io = ImGui::GetIO();
	io.DisplaySize.x = _resolution[0];
	io.DisplaySize.y = _resolution[1];
}


} // namespace uibase
