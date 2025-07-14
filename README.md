<div align="center">

<img src="Logo.png" height=250>

<h1>The Eugen++ Programming Language</h1>
<p>A general-purpose programming language, focused on simplicity.</p>

</div>

<h2>How to run</h2>
<h4>Download the <a href="https://github.com/eugenlukas/EugenPlusPlus/releases">latest</a> Eugen++ installer or Eugen++ directly and install/save it</43>

<h3>Run file</h3>
<h6>When .epp files are not associated with Eugen++.exe</h6>
<h6>.\Eugen++.exe can vary depending on where Eugen++.exe is located in storage</h6>

~~~
 .\Eugen++.exe F:\EugenPlusPlusTestFile.epp 
 ~~~

 <h3>Use in console</h3>
 <h6>Run the eugen++.exe normally via double click or per console</h6>
 <h6>.\Eugen++.exe can vary depending on where Eugen++.exe is located in storage</h6>

 ~~~
 .\Eugen++.exe
 ~~~

 <h3>Possible arguments to pass through the exe</h3>

 ~~~
 filepath		-run file directly (must be at the first position when used)
 --tokens		-shows all tokens
 --ast			-abstract syntax tree
 ~~~

<h2>Syntax</h2>

~~~
>			-Represents console output in this README
//			-Comment (whole line)
~~~

<h3>Arithmetic operators</h3>

~~~
+			-Addition
-			-Subtraction
*			-Multiplication
/			-Division
%			-Modulus (ToDo)
^			-Exponentiation
~~~

<h3>Comparison operators</h3>

~~~
==			-Equal to
!=			-Not equal to
>			-Greater than
<			-Less than
>=			-Greater than or equal to
<=			-Less than or equal to
~~~


<h3>Logical operators</h3>

~~~
AND			-Logical and
OR			-Logical or
NOT			-Logical not
~~~

<h3>Assignment operators</h3>

~~~
=			-Assign
~~~

<h3>Constants</h3>

~~~
MATH_PI			3.141592653589793
TRUE			1
FALSE			0
NULL			0
~~~

<h3>Variable definition</h3>

~~~
VAR a = 10
>10

VAR b = 20
>20

a+b
>30
~~~

~~~
VAR c = "Hello"
VAR d = "World"

c + " " + d
> Hello World
~~~

<h3>Conditional statement</h3>

~~~
IF a > b THEN "a is greater than b" ELIF a < b THEN "a is kess than be" ELSE "a and b are equal"
~~~

<h3>For loop</h3>

~~~
VAR result = 1

FOR i = 1 TO 6 THEN VAR result = result * i
~~~

<h3>While loop</h3>

~~~
WHILE TRUE THEN "loop"
~~~

<h3>Function definition</h3>

~~~
FUNC add(a, b) -> a + b

add(5, 6)
>11
~~~

<h3>Anonymous function</h3>

~~~
VAR sub = FUNC(a, b) -> a - b

sub(70, 1)
>69
~~~

<h3>String function</h3>

~~~
"this is " + "a string"
>this is a string
~~~

~~~
"this is a \n string"
>this is a 
 string
 ~~~

 ~~~
"this is a \\ string"
>this is a \ string
~~~

~~~
"this is a \" string"
>this is a " string
~~~

~~~
("Hello " * 3) + "Paul"
> Hello Hello Hello Paul
~~~

<h3>Lists</h3>

~~~
["name1", "name2", "name3"]
>["name1", "name2", "name3"]
~~~

~~~
["name1", "name2", "name3"] @ 1
>"name1"
~~~

~~~
["name1", "name2", "name3"] + 1
>["name1", "name2", "name3", 1]
~~~

~~~
["name1", "name2", "name3"] + [1,2]
>["name1", "name2", "name3", [1, 2]]
~~~

~~~
[10, 20, 30] - 2
>[10, 30]
~~~

~~~
["name1", "name2", "name3"] * ["name4", "name5"]
>["name1", "name2", "name3", "name4", "name5"]
~~~

<h3>Native/Buildin functions</h3>
<h6>Examples in one line formate</h6>

~~~
PRINT()			-takes in a string to output
LENGTH()		-takes in a list
INPUT_STR()		-promts the user to input some text
INPUT_NUM()		-prompts the user to input some number when fails asks the user to type again
IS_NUM()		-takes in any value and outputs true(1) and false(0)
IS_STR()		-takes in any value and outputs true(1) and false(0)
IS_LIST()		-takes in any value and outputs true(1) and false(0)
IS_FUNC()		-takes in any value and outputs true(1) and false(0)
APPEND()		-takes in a list and a value to append
POP()			-takes in a list and a number as index
EXTEND()		-takes in two lists
CLEAR()			-clears the console
SYSTEM()		-takes in a string as command
~~~

<h3>Multi-line statements</h3>

~~~
;			-functions as a statement terminator. Replacing it with actual line breaks does not alter the semantics of the code.
~~~

~~~
VAR result = IF 5 == 5 THEN "math works" ELSE "no"
>"math works"
~~~

~~~
IF 5 == 5 THEN; PRINT("math"); PRINT("works") ELSE PRINT("broken")
>math
>works
~~~

~~~
FOR i = 1 TO 6 THEN; PRINT("Hello "); PRINT("World") }
>Hello
>World
>Hello
>World
>Hello
>World
>Hello
>World
>Hello
>World
~~~

~~~
VAR i = 0
WHILE i < 10 THEN; VAR i = i + 1; PRINT(i) }
>1
>2
>3
>4
>5
>6
>7
>8
>9
>10
~~~

<h3>RETURN, CONTINUE and BREAK</h3>
<h6>multi line functions dont auto return the value(s)</h6>
<h4>Example for writing functions on multiple lines when running from file</h4>

~~~
FUNC test(); VAR foo = 5; RETURN foo; }
test()
>5
~~~

~~~
VAR a = []
FOR i = 0 TO 10 THEN; IF i == 4 THEN CONTINUE; IF i == 8 THEN BREAK; VAR a = a + i }
a

>[0, 1, 2, 3, 5, 6, 7]
~~~

~~~
VAR a = []
VAR i = 0
WHILE i < 10 THEN; VAR i = i + 1; IF i == 4 THEN CONTINUE; IF i == 8 THEN BREAK; VAR a = a + i }
a

>[0, 1, 2, 3, 5, 6, 7]
~~~

<h4>Example for writing functions on multiple lines when running from file</h4>
<h6>Note: The semicolon (';') functions as a statement terminator. Replacing it with actual line breaks does not alter the semantics of the code.</h6>

~~~
FUNC test()
	VAR foo = 5
	RETURN foo
}

test()

>5
~~~

~~~
VAR a = []

FOR i = 0 TO 10 THEN
	IF i == 4 THEN CONTINUE
	IF i == 8 THEN BREAK
	VAR a = a + i 
}

a

>[0, 1, 2, 3, 5, 6, 7]
~~~

~~~
VAR a = []
VAR i = 0
WHILE i < 10 THEN
	VAR i = i + 1
	IF i == 4 THEN CONTINUE
	IF i == 8 THEN BREAK
	VAR a = a + i
}

a

>[0, 1, 2, 3, 5, 6, 7]
~~~

<h3>Import other files</h3>
<h6>Relative paths for files are also possible</h6>

<h4>Use functions</h4>

<small>Main.epp</small>
~~~
# IMPORT "F:\\Test.epp" AS Test

Test::add(5, 6)
>11
~~~

<small>Test.epp</small>
~~~
FUNC add(a,b) -> a + b
~~~

<h4>Use variables</h4>

<small>Main.epp</small>
~~~
# IMPORT "F:\\Test.epp" AS Test

Test::name
>Paul
~~~

<small>Test.epp</small>
~~~
VAR name = "Paul"
~~~