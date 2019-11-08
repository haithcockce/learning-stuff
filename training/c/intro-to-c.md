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

   - `goto` simply jump to the label in the goto statement
   - `break` terminates loops and switch statements
   - `if` tests an expression, if the expression is true, then execute wrapped code
      - `if-else` variation of the `if` statement where if the statement is false, the wrapped code in the else section is executed
      - `if-else if-else` variation of the `if` statement where a series of if statements are logically grouped together. When one of the if statements evaluates to true, the rest of the statements in the grouping are not evaluated.
      - **Note** A series of if statements can be executed in series and each will be evaluated. This is different from a block of `if-else if-else` statements where the block of if/else if statements will stop being evaluated when one is true.
   - Loops where code is executed 0 or more times repeatedly. Loops tend to end when a condition is tested and is false, the loop is manually broken, or you jump out of the loop. Think of it like a fancy if statement that can be executed multiple times in a row.
      - `for` loop, allows a starting condition to be defined, what conditions do we continue to repeat the code, and what to do for the loop specifically with each iteration of the loop execution.
      - `while` loop, only defines the conditions in which the loop continues to be repeatedly executed.
         - `do-while` loop, similar to a while loop except we test the stop conditions after loop execution instead of before.
   - Switch statements are akin to a fairly sophisticated if-else if-else lattice.
      - An expression is evaluated (the switch), and each case is evaluated until terminated
      - Cases can contain optional `break` statements to terminate the switch-case block if the case is executed
      - If a case is executed and does not have a break, then the remainder of the cases will continue to be evaluated
      - The case series can contain a single, optional `default` case which will execute in any scenario if the evaluations are not terminated early (such as with `break`, `return`, `goto`, etc)

## Bits and Bytes

- While C has "types", the types are quite loosely defined in so much that execution of a program doesn't care if you have a variable declared as one type but used as another. You will have compilation warnings but that's all.
- What defines types in C is the amount of memory used to hold the value in terms of bytes, which can be checked with `sizeof()`
- This flexibility allows for powerful manipulations of values, such using `char var = 'a'` then using arithmetic to change the letter, such as `var++`.
- The type simply describes how to take the value in memory and interpret it. For example, all the if statements testing for true or false are really testing for a non-zero value (true) or a zero value (false)

### Binary and Hexadecimal

- Numbering comes in many representations, decimal uses 10 symbols to represent values, binary uses 2, and hexadecimal uses 16. Others exist, like the seldomly used octal (where 8 symbols are used) and so on. 
- Any number can be converted to another representation with the proper math. 
- As a quick reminder: decimal works like this, 

   - Each column in a number is (n x 10^i) where n in the symbol (like 8 or 2) and i is the index of the column (such as the 0th or 7th). Thus a number like 764 is (7 x 10^2) + (6 * 
10^1) + (4 x 10^0)
   - Binary works like this but uses two symbols instead, 0 and 1, so the number values are (n x 2^i). Thus 10010 is (1 x 2^4) + 0 + 0 + (1 x 2^1) = 18. To convert from decimal to binary, simply deduct the highet possible powers of two and keep track of which values in binary are taken. For example, for 18, 16 is the highest power of two leaving 2. You can not deduct 8 or 4 but you can deduct 2. What is left over is 0, so 10010.
   - Hexadecimal is also similar but uses 16 symbols (0-9, A-F) to represent numbers. Converting binary to hex is straight forward: 1 is 0001, 2 is 0010, 3 is 0011, 4 is 0100, ... 9 is 1001, A is 1010, B is 1011, C is 1100, D is 1101, E is 1110, and F is 1111. Note the pattern! Converting decimal to hex is made simple by first converting to binary. 

### Bitwise Arithmetic

- _Bit_ a 0 or 1. 
- _Nibble_ 4 bits
- _Byte_ 8 bites
- _Word_ 16 bits (2 bytes)
- _Double Word_ 32 bits (4 bytes)
- _Quad word_ 64 bits (8 bytes)

- Because we have access to the raw values in memory, we can interact with the values at the bit level.

   - `x & y` x bitwise and with y, 1001 & 0101 => 0001
   - `x | y` x bitwise or with y, 1001 | 0101 => 1101
   - `x ^ y` x bitwise exclusive or with y, 1001 ^ 0101 => 1100
   - `~x` bitwise negation of x, ~1001 => 0110
   - `x >> y`/`x << y` bitshift x right/left by y amount of bits, 0101 >> 2 => 0001, 0101 << 2 => 0100

      - For unsigned values, right shifting pads with 0's since we do not care about the negative or positive value
      - For signed values, right shifting pads with the value of the left-most bit to preserve the sign (pad with 0's if positive, pad with 1's if negative)

- Negative values are denoted by the left most bit being set to 1. If the value is unsigned, then the number does not have a negative sign regardless of the left most bit. 

### C "Types" and Implications

- Types only define how many bytes are used to represent the value and how to interpret the value. For example, a variable may be of type `int`, it can be interpreted as a letter, a decimal value, just the bits itself, etc without changing the value itself. 
- Because the types define the amount of bytes used to represent the values, the sizes are static and remain unchanged. Likewise, the limits of the values are dependent on how many bits can be used to represent that number. For example, `char` is typically a single byte and can thus only represent 0 to 255 (unsigned) or -128 to 127 (signed)
- _Overflow_ / _Underflow_ when modifications to the values hit a limit and wrap around to the other limit. 

   - Overflow example: An unsigned char has an upper limit of 255, represented as 11111111. Adding 1 to this would cause the number to overflow to 00000000. Without the limits, the number would equal 100000000, but this requires 9 bit to represent the value. The additional 1 is dropped due to the limitations. 
   - Underflow example: similarly, with an unsigned char whose value is 00000000, if 1 is subtracted from it, this would cause the value to become 11111111

## Pointers 

- When something is in memory, that thing resides within a specific location of memory. That specific location can be described with an address. 

   - Humans use addresses to describe where we live. 276 Example Ln. Townsville, California, 50505 for example.

- _Pointer_ is an address that references something in memory. 

   - The 276 Example Ln. address would be a pointer to someone's house

- _Dereference_ Means to go to the location in memory described by a pointer and use the contents therein. 

   - Dereferencing the 276 Example Ln. address would mean traveling to that house and going inside 

- Pointers are nothing more than addresses which are just numbers (typically in hexadecimal) that can be manipulated like any other number. Pointers can thus be modified to point to different areas of memory!

   - We can increment 276 Example Ln. by + 2 which gives 278 Example Ln., the neighbor to 276!


