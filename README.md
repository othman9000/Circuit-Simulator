# Circuit-Simulator
simple circuit simulator that does nodal analysis on DC circuits 
it supports resistors, voltage sources, current sources.
inputing a ciruit is done using a simple text file that descirbes the circuit using predefined syntax
after writing the text file, send its path to the program and it will output each node's voltage
supports logical error detection (voltage source loop, floating nodes,etc)
there are alot of ways this project can be updated in the future such as AC analysis or supporting other components like capactiors, inductors ,etc.


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
you can use # for comments but they should be on a seperate line
