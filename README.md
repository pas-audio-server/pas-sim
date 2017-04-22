# passim

A stubbed pas simulator to assist in front-end web server development

passim is a single c++ file. It depends upon the proto3's created for pas.

passim can be built using its makefile.

It is invoked by:

    ./pas-sim
    
Currently, it opens port 5077. An option to open an arbitrary port will be added shortly.

passim is well instrumented and is capable of driving the curses client.

passim currently does **not** support namespaces.
