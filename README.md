# Yahtzee-Computer-Game

How to Run:

A) The executable "Yahtzee.exe" is for Windows/Intel x86/64. Easiest way to run for this platform.

B) Compile

Prerequisites: OpenGL/freeglut


To run this game on gcc, use "make" to compile. If you are in Visual Studio, you need to increase the stack limit to 3MB, which you can do under Properties -> Linker -> All Options -> Stack Reserve Size. With a limit of 1MB, the application will fail with stack overflow.

How to Play:

To select dice, press on any of the dots on dice. To choose a category, click on the appropriate button. If you want to change the category, just press another. Red means selected, and black means unselected. Your decisions are confirmed when you press done. After the end of your turn, the computer will make its turn. When the done button comes back, press it to begin your next turn.
