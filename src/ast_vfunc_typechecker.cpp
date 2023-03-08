#include "ast.h"
#include "typechecker.h"
namespace protolang
{

void VarDecl::check_type(TypeChecker* context)
{
	// 检查 init 和 声明的类别相等

}

uptr<Type> BinaryExpr::get_type(TypeChecker *context)
{

}
} // namespace protolang