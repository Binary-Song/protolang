#pragma once
#include "ast.h"
namespace protolang
{

class TypeChecker
{
public:
	void check(const Program *program)
	{
		for (auto &&decl : program->decls)
		{
			switch (decl->decl_type())
			{
			case DeclType::Var:
				check_var(dynamic_cast<const DeclVar &>(*decl)) break;
			case DeclType::Func:
				check_func(dynamic_cast<const DeclFunc &>(*decl));
				break;
			}
		}
	}

private:
	void check_var(const DeclVar &decl)
	{
		if(decl.init->)
	}
	void check_func(const DeclFunc &decl) {}
};

} // namespace protolang