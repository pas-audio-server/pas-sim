# pas-sim

A stubbed pas simulator to assist in front-end web server development

pas-sim is a single c++ file. It depends upon the proto3's created for pas.

pas-sim can be built using its makefile.

It is invoked by:

    ./pas-sim
 
By default, pas-sim listens on port 5077. A very special number to some.

To others, not so much. Use -p portnum to change the port to your liking.

To enable some more detailed printing (of hexidecimal bytes sent and
received) specify -d.

pas-sim is well instrumented and is capable of driving the curses client
(an example of a reasonably demanding client) with limited simulated data.

pas-sim currently does **not** support namespaces.
