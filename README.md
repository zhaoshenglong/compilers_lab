# Tiger Compiler Labs in C++

## Introduction

## lab1

lab1 is easy. You only need to write a strait-line expression parser. 

### Related chapters

* `Chapter 1`.

## lab2 

lab2 is easy too. You need to add code in `tiger.lex`.<br/>
The only potential difficulty is the parse of `escape string`. There is a very smart and simple solution but I did not apply it in my implementation in that I do not know about is exactly. It is better for you to reference `ASCII Table`, and you will find that `escape strings` can compute by `ord(<S>) - ord(A) + 1`.


### Related chapters

* `Chapter 2`
* `Tiger reference handout`

## lab3

lab3 is like lab2, you only need to tell the `yacc` your rule defined to tiger.<br/>
The difference is than it is difficult than lab3, if you have not read about corresponding chapters. You need not only read through `chapter3` but also `chapter 4`. `Chapter 4` contains all the interface you need to complete. When I was doing this lab, I did not know this so I have to spend much time understanding `absync`.

In this lab, you may need to debug for some time and you aim is to remove `reduce-reduce` in your program and reduce `reduce-shift`.

### Related chapters

* `chapter 3`
* `chapter 4`


## lab4

lab4 is the last lab that already provide you the whole structure. You only need to complete code of each function. 

### Related chapters

* `chapter 5`


## lab5

lab5 is much more difficult than any labs that I do in ics, cse etc. It contains many modules that you need to design and coordinate. <br/>

For me, I do not understand frame at first. I thought it as runtime stack, but in out design, frame is the static layout of one function. <br/>

Here are some notes that I think important.

* One frame for one function.
* Frame contains all the arguments in it and we access arguments with interface provided by `Class Frame`. To do that, we need to do `view shift` for each frame. Which
  means you need to `allocLocal` for each argument in `rdi`, `rsi`, `rdx`...`rsp + wordsize`, `rsp + 2*wordsize`...

* `rdi`, `rsi` and other registers is implemented in `TEMP::Temp`, the only differece is that they are static, and whenever you need to access them, you should access with interface provided by `Class Frame`.
* `procEntryExit1`, `procEntryExit2`, `procEntryExit3` is the function for a frame layout.
  * In `procEntryExit1`, you should save Escape arguments, save callee registers, execute body statement, restore callee registers. All these are `T::Statement` which is implemented in `T` namespace.
  * In `procEntryExit2`, you need to add an empty instr at the end of frame. The reason for this is that it makes `RV`, `SP`, `CalleeSaves` live out in each frame and you do not need to consider any thing about it in lab5.
  * In `procEntryExit3`, you need to manipulate `SP`. You `subq $framesize, %rsp` at the beginning of frame, and `addq $framesize, %rsp` at the end of each frame. If you implement the case in which arguments are more than 6, you should manage `rsp` here for the truth arguments are at the end of frame.
* In `codegen`, string reference is implemented with instruction `leaq label(%rip), <reg>`.
* In `codegen`, `%rbp` should be replaced with `framesize(%rsp)`.
* You also need to implement a naive register allocation in this lab. In my implementation, I use `r10` and `r11` in that they are caller saves.

### Related chapters

* `chapter 6`
* `chapter 7`
* `chapter 8`
* `cahpter 9`
* `chapter 12`

## lab6

Since you have already solved `lab5`, `lab6` is easier. You need to do `escape analysis`, `liveness analysis`, `register allocation`. <br/>

Notes are below.

* `Escape analysis` is like `Translate` or `SemanAnalysis`, you could implemented in c-style or make them a method of `Absync`. The latter is much more easier and decent.
* `Liveness analysis` is the same as described in `chapter 10`. You first define `def`, `use` for each instruction and produce `flow graph`. Then you should analyse `live in & live out` information for each instruction. Finally, you need to produce interference graph. 
  * The last instruction should be empty. This is produced in `procEntryExit3` and you should add this after codegen.
  * The live out registers for each frame should be `RV`, `SP`, `Callee Saves`.
  * Do not forget `machine registers`. They are mutually interfere with each other.
* `Register allocation` is the most difficult module in this lab. However, thanks to the `pseudocode` in our textbook, it is not too much difficult. In my implementation, all inteface are provided in `RegAlloc` module. Different from the implementation in textbook, I did not split `RegAlloc` and `Color` module. I simply implement the `pseudocode` described in textbook. There are some notes you may remember.
  * worklistmoves, constrainedmoves ..., all these move related sets can not contains duplicate items. For example, `a->b` or `b->a` are the same. The type of each item could be `LIVE::MoveList` or `AS::Instr` like our textbook. You only need to ensure that there are no duplicates.
  * Our `build` function should initial many datasets
    * worklistmoves
    * moveList
    * addEdge
    * initial
    * alias
    * ...
### Related chapters
* `Chapter 10`
* `Chapter 11`

## Bugs in my implementation
1. In `Escape analysis`, I ignored the analysis of arguments in `callExp`.
2. I did not deal with the case in which arguments are more than 6.
3. Some other bugs ... (I don't know where they are, but they must exist.)



Thanks for reading. Good luck ~ Any questions, please contact me.
