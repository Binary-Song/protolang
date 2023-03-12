#pragma once
#include "ast.h"
#include "env.h"
namespace protolang
{

class SemanticAnalyzer
{
	ast::Program *m_program;
	const Env    *m_env;

public:
	SemanticAnalyzer(ast::Program *program, const Env *env)
	    : m_program(program)
	    , m_env(env)
	{}

	void analyze() const { m_program->analyze_semantics(); }
};

} // namespace protolang
