#include <llvm/ADT/APFloat.h>
#include "ast.h"
#include "code_generator.h"
namespace protolang
{
namespace ast
{

llvm::Value *LiteralExpr::codegen(CodeGenerator &g) {}

} // namespace ast
} // namespace protolang