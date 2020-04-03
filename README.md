# A Multithreaded Server using the Pthread library.

## The server is connected with a simple key/value Database developed by Adam Ierymenko (<adam.ierymenko@zerotier.com>).
* Clients can connect to the server and perform PUT or GET requests.  
* The producer thread adds the incoming connections into a circular queue, the worker threads pop the incoming connections from the queue and they process the request.  
* The Server computes and updates stats about the average time completion of a request, average time waiting in queue and more.  

# Schematic view of the Server
![Schematic](https://github.com/AlexandrosAlexiou/Multithreaded-Server/blob/master/Schematic.png)
