# Circuit-Simulator
A simple DC circuit simulator that performs nodal analysis on circuits defined in a custom text-based syntax.
It supports resistors, voltage sources, and current sources, and can automatically detect logical errors such as voltage source loops or floating nodes.
there are alot of ways this project can be updated in the future such as AC analysis or supporting other components like capactiors, inductors ,etc.

How it works :
-Write your circuit in a text file using the predefined syntax (see below).
-Run the program and provide the path to your circuit file.
The simulator will:
-Parse your circuit.
-Build node equations using nodal analysis.
-Solve them using Gaussian elimination.
-Display the voltage at each node.

circuit syntax:
resistor ID VAl (defining a resistor varaiable,the value must be positive)
voltages ID VAL (defining a voltage source variable,the value can be negative)
currents ID VAL (defining a current source variable,the value can be negative)

Branch ID       (defining a new branch)
Node   ID       (defining a new node  )

add branchID elementID (adding a component to a branch)
add nodeID1 nodeID2 branchID (adding a branch between two nodes, note that if you have a voltage source then its positive terminal will be pointing to nodeID1 while
                              its negative terminal will be pointing to nodeID2, the same is true for the current source so you would expect the current source to be
                              pointing to nodeID1)
identifiers must start with an alphabet and contain only numebrs and alphabets
the syntax is not case senstive
the first node you define is considered as the ground node
you can use # for comments but they should be on a seperate line
