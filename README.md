Nyris is a code library that creates a scalable server system that can manage mulitple client sockets with a minimum of 2 threads in all. It also has built in Ssl support for establishing secure connections. The goal is to make a server system I or anyone can build on top of to accomplish a more grand project. The code uses poll to monitor and handle io for all the sockets on one thread. Once read and decrypted, it will hand the data to a priority queue. Data will then be dequeued from the queue and given to threads in the built in thread pool. The Nyris server will be inalized with a function pointer that the thread pool will use to call on the data. These functions will be supplied a Request struct as its input, and with this it will have access to all server methods (like writing to other sockets) as well as the ability to store application state pointers with the file discriptors them selves.


Included a are a cert and key file, If you plan on using this seriously, please generate your own and replace these for security reasons. 

While the core of the project is generally working, I plan on adding more features and do some substantial clean up to the code base. This code base originally started as more of a sandbox to tinker with ssl, multi client servers, and concurrency. For this reason, there is a lot that is included that was previously used but is not longer.

The prototype of the server its self can be compiled with main9.cpp and its dependencys DiscriptorWrapper.cpp SecureConnectionManager.cpp SslProtocolLayer.cpp, also make sure to complie it with the crypto, ssl, and pthread libraries too. 

To mess with it and test it, compile main2.cpp with SslSock.cpp and the same libraries

At the moment, the server established ssl connections and simply acts as an echo server. 
