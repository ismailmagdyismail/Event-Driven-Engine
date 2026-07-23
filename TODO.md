- registeration
- getID (not whole socket info)
- un-register / close
- threading and RCU | COW
- thread pool
- Design
  - Main -> EventLoop::Register Callbacks to handle reads, writes
  - Main -> Create Sockets (implictly call event loop internall), returns promise and future on reading and doing any async event

```cpp
 while (true)
    {
        //! accept blocks
        //! bg processing of client 1 task
        //! accpet blocks again, accept another new client connection (concurrect / parallel to client 1 processing)
        //! send message to client 2 concurrently

        ClientSocket clientSocket = serverSocket.Accept().wait();
        if(firstTimeAccess)
        {
            threadPool.submitTask([](){}); //! Do task
            clientsocket.send("message");
        }
        else{
            clientsocket.send("message");
        }
    }

    //! Event loop submission, direct access + callbacks
    serverSocket = AsyncIO::TCPServerSocket::Create().second;
    serverSocket.Listen(8080);

    loop.SubScribeToEvent(serverSocket.GetID(), AsyncIO::EventType::Read, [](AsyncIO::EventContext context)
                          { HandleReadyFD(context); });
    loop.Run();

    close(serverSocket.GetID());
```
