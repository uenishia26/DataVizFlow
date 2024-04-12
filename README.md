myshell:
Similarly to all executables in this project, run "make" in the terminal to create the myshell executable. To run, input ./myshell <filename> into the terminal. If filename is empty, it will print "myshell>" and from there the user will input commands. All executables found in regular BASH as well as "tapper" and "tappet" run in myshell. Non-binary executables, such as "cd", do not work. Distinct commands should be seperated by a semicolon (e.g cmd1; cmd2). To run a command in the background, end the command with a "&" character. The "|" character, found in regular shells for piping processes, is implemented here as well. To stop all foreground processes, hit Ctrl-C. To exit the shell, hit Ctrl-D. Some other features found in a regular shell not found in this one include command history (up arrow) and paging.  

tapper/tappet:
Similarly to all executables in this project, run "make" in the terminal to create the tapper and tappet executable. To run, input ./tapper or ./tappet in the main bash but not in the myshell just write the name of executable "tappet" or "tapper". After each **successful run of tapper and tappet please run "make clean"** before running another instance of tapper or tappet to delete the data file read by gnuplot.
