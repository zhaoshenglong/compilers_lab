#include "straightline/prog1.h"

// a = 5 + 3; b = (print(a, a-1), 10*a); print b;
A::Stm *prog() {
  // a = 5 + 3;
  A::Stm *assStm1 = new A::AssignStm(
      "a", new A::OpExp(new A::NumExp(5), A::PLUS, new A::NumExp(3)));

  // b = (print(a, a-1), 10*a);
  A::Exp *opExp1 = new A::OpExp(new A::IdExp("a"), A::MINUS, new A::NumExp(1));
  A::Stm *prStm1 = new A::PrintStm(
      new A::PairExpList(new A::IdExp("a"), new A::LastExpList(opExp1)));
  A::Exp *opExp2 = new A::OpExp(new A::NumExp(10), A::TIMES, new A::IdExp("a"));
  A::Stm *assStm2 = new A::AssignStm("b", new A::EseqExp(prStm1, opExp2));

  // print b;
  A::Stm *prStm2 = new A::PrintStm(new A::LastExpList(new A::IdExp("b")));

  // b = (print(a, a-1), 10*a); print b;
  A::Stm *comStm = new A::CompoundStm(assStm2, prStm2);

  return new A::CompoundStm(assStm1, comStm);
}

// a = 5 + 3; b = (print(a, a-1), 10*a); print b;
// a = 5 + b; b = (print(a, a, a-1), 10*a); print b;
A::Stm *prog_prog() {
  // a = 5 + 3; b = (print(a, a-1), 10*a); print b;
  A::Stm *stm1 = prog();

  // a = 5 + b;
  A::Stm *assStm1 = new A::AssignStm(
      "a", new A::OpExp(new A::NumExp(5), A::PLUS, new A::IdExp("b")));

  // print(a, a, a-1)
  A::Exp *exp1 = new A::OpExp(new A::IdExp("a"), A::MINUS, new A::NumExp(1));
  A::ExpList *explist = new A::PairExpList(
      new A::IdExp("a"),
      new A::PairExpList(new A::IdExp("a"), new A::LastExpList(exp1)));
  A::Stm *prStm1 = new A::PrintStm(explist);
  // 10 * a
  A::Exp *exp2 = new A::OpExp(new A::NumExp(10), A::TIMES, new A::IdExp("a"));

  // b = (print(a, a, a-1), 10*a);
  A::Stm *assStm2 =
      new A::AssignStm("b", new A::EseqExp(new A::PrintStm(explist), exp2));

  // print b;
  A::Stm *prStm2 = new A::PrintStm(new A::LastExpList(new A::IdExp("b")));

  return new A::CompoundStm(
      stm1, new A::CompoundStm(assStm1, new A::CompoundStm(assStm2, prStm2)));
}

// a = 5 + 3; b = (print(a, a-1), 10*a); print b;
// a = 5 + b; b = (print(a, a, a-1), 10*a); print b;
// a = (a = a+b, a);
A::Stm *right_prog() {
  A::Stm *stm1 = prog_prog();
  return new A::CompoundStm(
      stm1,
      new A::AssignStm(
          "a", new A::EseqExp(new A::AssignStm(
                                  "a", new A::OpExp(new A::IdExp("a"), A::PLUS,
                                                    new A::IdExp("b"))),
                              new A::IdExp("a"))));
}
