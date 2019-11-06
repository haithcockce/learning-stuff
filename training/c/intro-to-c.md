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

## Operators

### Arithmetic Operators

- `+, -, *` add, subtract, multiply
- `/` divide and return the quotient with no remainder
- `%` modulo, a variation of divide, but return the remainder and not the quotient

#### C-specific arithmetic and relational operators

- A family of logical expressions, rules, and representations.
- The basic values are _true_ and _false_ and a language of operators are built around manipulating and representing these values in various ways to perform arithmetic on these values.
- C utilizes boolean logic to execute code based on various conditions (such as should I update the file now or later,
checking if something is finished, etc).

- `x == y` equivalency
- `x != y` inequality
- `x > y` greater than (`x >= y` greater than or equal to)
- `x < y` less than (`x <= y` less than or equal to)
- `x < y != z` relational chaining (evaluated left to right)
- `x = y` assignment (x now has a copy of y's value)
- `x += y` increment x by y and assign the new value to x
- `x -= y` decrement x by y and assign the new value to x
- `x *= y` multiplication and assignment
- `x /= y` division and assignment
- `x %= y` modulo and assignment
- `x++` return the value of x, then increment x and assign x the new value
- `++x` increment x and assign x the new value, then return the value of x
- `x--, --x` decrement


### Boolean Arithmetic and Control Logic

- Describes what code to execute under certain conditions. For example, if something is true, then run a few functions but don't run them if false, or execute a set of functions repeatedly.
- Based on boolean logic

#### Boolean Operators

- `x || y` evaluates TRUE if x is TRUE _or_ y is TRUE
- `x && y` evaluates TRUE if x is TRUE _and_ y is TRUE
- `!x` negation (TRUE becomes FALSE and vice-versa, but no assignment)

#### Control Logic 

- Typical control logic statements in C:

   - `if` tests an expression, if the expression is true, then execute wrapped code
      - `if-else` variation of the `if` statement where if the statement is false, the wrapped code in the else section is executed
      - `if-else if-else` variation of the `if` statement where a series of if statements are logically grouped together. When one of the if statements evaluates to true, the rest of the statements in the grouping are not evaluated.
      - **Note** A series of if statements can be executed in series and each will be evaluated. This is different from a block of `if-else if-else` statements where the block of if/else if statements will stop being evaluated when one is true.
   - Loops where code is executed 0 or more times repeatedly. Loops tend to end when a condition is tested and is false, the loop is manually broken, or you jump out of the loop. Think of it like a fancy if statement that can be executed multiple times in a row.
      - `for` loop, allows a starting condition to be defined, what conditions do we continue to repeat the code, and what to do for the loop specifically with each iteration of the loop execution.
      - `while` loop, only defines the conditions in which the loop continues to be repeatedly executed.
         - `do-while` loop, similar to a while loop except we test the stop conditions after loop execution instead of before.
   - Switch statements
