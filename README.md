# Simulation
1. Create a new simulation in Cooja ("contiker cooja")
2. Add one new Z1 mote with border-node.c code, compile it
3. On this node, perform a right click -> Mote tools -> Serial Socket (SERVER), put "60001" as "Listen port" and click "start"
4. Add some a new Z1 mote with computation-node.c code, compile it
5. Add some a new Z1 mote with sensor-node.c code,  code, compile it
6. Place each node where you want
7. Inside a new command prompt, in the same directory, run the command "python3 server.py --ip 172.17.0.2 --port 60001"
8. Start the simulation in Cooja and follow what happens in the log
