#ifndef TIGER_TRANSLATE_TRANSLATE_H_
#define TIGER_TRANSLATE_TRANSLATE_H_

#include "tiger/absyn/absyn.h"
#include "tiger/frame/frame.h"

/* Forward Declarations */
namespace A {
class Exp;
}  // namespace A

namespace TR {

class Exp;
class ExpAndTy;
class Level;

Level* Outermost();

F::FragList* TranslateProgram(A::Exp*);

}  // namespace TR

#endif
