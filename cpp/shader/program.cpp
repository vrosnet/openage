// Copyright 2013-2014 the openage authors. See copying.md for legal info.

#include "program.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "../log.h"
#include "../util/file.h"
#include "../util/strings.h"
#include "../util/error.h"

namespace openage {
namespace shader {

Program::Program() : is_linked(false), vert(nullptr), frag(nullptr), geom(nullptr) {
	this->id = glCreateProgram();
}

Program::Program(Shader *s0, Shader *s1) : Program{} {
	this->attach_shader(s0);
	this->attach_shader(s1);
}

Program::~Program() {
	glDeleteProgram(this->id);
}

void Program::attach_shader(Shader *s) {
	switch (s->type) {
	case GL_VERTEX_SHADER:
		this->vert = s;
		break;
	case GL_FRAGMENT_SHADER:
		this->frag = s;
		break;
	case GL_GEOMETRY_SHADER:
		this->geom = s;
		break;
	}
	glAttachShader(this->id, s->id);
}

void Program::link() {
	glLinkProgram(this->id);
	this->check(GL_LINK_STATUS);
	glValidateProgram(this->id);
	this->check(GL_VALIDATE_STATUS);
	this->is_linked = true;
	this->post_link_hook();

	if (this->vert != nullptr) {
		glDetachShader(this->id, this->vert->id);
	}
	if (this->frag != nullptr) {
		glDetachShader(this->id, this->frag->id);
	}
	if (this->geom != nullptr) {
		glDetachShader(this->id, this->geom->id);
	}
}

/**
checks a given status for this program.

@param what_to_check GL_LINK_STATUS GL_VALIDATE_STATUS GL_COMPILE_STATUS
*/
void Program::check(GLenum what_to_check) {
	GLint status;
	glGetProgramiv(this->id, what_to_check, &status);

	if (status != GL_TRUE) {
		GLint loglen;
		glGetProgramiv(this->id, GL_INFO_LOG_LENGTH, &loglen);
		char *infolog = new char[loglen];
		glGetProgramInfoLog(this->id, loglen, NULL, infolog);

		const char *what_str;
		switch(what_to_check) {
		case GL_LINK_STATUS:
			what_str = "linking";
			break;
		case GL_VALIDATE_STATUS:
			what_str = "validation";
			break;
		case GL_COMPILE_STATUS:
			what_str = "compiliation";
			break;
		default:
			what_str = "<unknown task>";
			break;
		}

		util::Error e("Program %s failed\n%s", what_str, infolog);
		delete[] infolog;
		throw e;
	}
}

void Program::use() {
	glUseProgram(this->id);
}

void Program::stopusing() {
	glUseProgram((GLuint) 0);
}

GLint Program::get_uniform_id(const char *name) {
	return glGetUniformLocation(this->id, name);
}

GLint Program::get_attribute_id(const char *name) {
	if (this->is_linked) {
		GLint aid = glGetAttribLocation(this->id, name);
		if (aid == -1) {
			this->dump_active_attributes();
			throw util::Error("queried attribute '%s' not found or not active (pwnt by the compiler).", name);
		} else {
			return aid;
		}
	}
	else {
		throw util::Error("queried attribute '%s' id before program was linked.", name);
	}
}

void Program::set_attribute_id(const char *name, GLuint id) {
	if (!this->is_linked) {
		glBindAttribLocation(this->id, id, name);
	}
	else {
		//TODO: maybe enable overwriting, but after that relink the program
		throw util::Error("assigned attribute '%s' = %u after program was linked!", name, id);
	}
}

void Program::dump_active_attributes() {
	log::imp("dumping shader program active attribute list:");

	GLint num_attribs;
	glGetProgramiv(this->id, GL_ACTIVE_ATTRIBUTES, &num_attribs);

	GLint attrib_max_length;
	glGetProgramiv(this->id, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &attrib_max_length);

	for (int i = 0; i < num_attribs; i++) {
		GLsizei attrib_length;
		GLint attrib_size;
		GLenum attrib_type;
		char *attrib_name = new char[attrib_max_length];
		glGetActiveAttrib(this->id, i, attrib_max_length, &attrib_length, &attrib_size, &attrib_type, attrib_name);

		log::imp("-> attribute %s : type=%d, size=%u", attrib_name, attrib_type, attrib_size);
		delete[] attrib_name;
	}
}


void Program::post_link_hook() {
	this->pos_id  = this->get_attribute_id("vertex_position");
	this->mvpm_id = this->get_uniform_id("mvp_matrix");
}

} //namespace shader
} //namespace openage
