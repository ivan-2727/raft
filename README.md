# Raft 
This is my pet project, implementation of the Raft algorithm from scracth. 
To create a Raft simulation, navigate to the `simulation` folder and run `sh run.sh`. The results of simulation will appear in `sumulation/result/state.txt`. Parameters can be edited in `simulation/config`. Commands can be sent via terminal:
- `do N command`, where N is the number of the server (0, 1, 2...)
- `toggle N`, toggle activity of the server number N