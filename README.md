<div align="center">

<h1>The Eugen++ Programming Language</h1>
<p>A general-purpose programming language, focused on simplicity.</p>

</div>

<h2>Syntax</h2>
<h3>Arithmetic operators</h3>

~~~
+ Addition
- Subtraction
* Multiplication
/ Division
% Modulus (ToDo)
^ Exponentiation
~~~

<h3>Comparison operators</h3>

~~~
== Equal to
!= Not equal to
> Greater than
< Less than
>= Greater than or equal to
<= Less than or equal to
~~~


<h3>Logical operators</h3>

~~~
AND Logical and
OR Logical or
NOT Logical not
~~~

<h3>Assignment operators</h3>

~~~
= Assign
~~~

<h3>Variable definition</h3>

~~~
VAR a = 10
>10
VAR b = 20
>20

a+b
>30

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

"this is a \n string"
>this is a 
 string

"this is a \\ string"
>this is a \ string

"this is a \" string"
>this is a " string

("Hello " * 3) + "Paul"
> Hello Hello Hello Paul
~~~

<h3>Lists</h3>

~~~
["name1", "name2", "name3"]
>["name1", "name2", "name3"]

["name1", "name2", "name3"] @ 1
>"name1"

["name1", "name2", "name3"] + 1
>["name1", "name2", "name3", 1]

["name1", "name2", "name3"] + [1,2]
>["name1", "name2", "name3", [1, 2]]

["name1", "name2", "name3"] * ["name4", "name5"]
>["name1", "name2", "name3", "name4", "name5"]
~~~