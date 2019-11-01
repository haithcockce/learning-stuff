# Basics of C Code

## Writing Code

### Overview

- _Source_ typically refers to the code itself
- _Compile_ translate human-readable source code into machine-readable binary to run
- _Executable_ a file of raw binary code typically made from compiled source code which is loaded into memory and ran 
- _Expression_ like math, some logic that, when executed, has some sort of result, like 1 + 2 is an expression that results in 3
- _Variable_ something that represents a value. Like in math, x = 2, x is the variable whose value is 2.
- _Function_ a series of lines of code that, when executed, performs some objective
- _Parameter_ the value provided to a variable going into a function. For example, with the pythagorean theorem, a^2 + b^2 = c^2 is the function, and you can provide a = 1 and b = 2.
- _Return value_ what you expect a function to give to you when it finishes. For example, 1^2 + 2^2, the return value would be 5
- _Library_ source code already written elsewhere but imported into your code. Referred to as packages in Java, modules in Python, etc
- _Type_ what kind of object is it? Is it a number? Is it a string? Is it something else? 

C-Specific stuff

- _Function Prototype_ defines how you interact with a function such as what it is called, what parameters, if any, need to be provided and what type are the parameters, and what the function is supposed to return, if anything, and what type the return value is. 

### Control Logic

- Describes what code to execute under certain conditions. For example, if something is true, then run a few functions but don't run them if false, or execute a set of functions repeatedly. 
- Typical control logic statements in C:

   - `if` tests an expression, if the expression is true, then execute wrapped code
      - `if-else` variation of the `if` statement where if the statement is false, the wrapped code in the else section is executed

