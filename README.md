# TODO:

- handling huge reads, writes
- Create Chat-TUI example
- add threading and RCU | COW
- add thread pool

# API usage Examples

## 1- Callback based

```cpp
    /*
    =============================================================================
    =========================== CallBack-based ==================================
    =============================================================================
    */


    //! 1. [Callback based]
    //! FACADEs the internal Eventloop wiring (Inspired a bit by ZIG new I/O interface)

    /*
    =============================================================================
    =========================== Server =========================================
    =============================================================================
    */

    //! setup event loop, wire it up with async FDs / sockets
    AsyncIO::EventLoop loop;
    auto serverSocketCreation = AsyncIO::TCPServerSocket::Create(&loop);
    AsyncIO::TCPServerSocket &serverSocket = serverSocketCreation.second;

    int port = 8080;
    auto listenResult = serverSocket.Listen(port);

    //! Event loop submission (internally handles Eventloop access, registeration)
    serverSocket.OnAccept(OnAcceptConnectionHandler);
    loop.Run();

    serverSocket.Close();


    /*
    =============================================================================
    =========================== Client ==========================================
    =============================================================================
    */
    AsyncIO::EventLoop loop;
    auto socketCreation = AsyncIO::TCPClientSocket::Create(&loop);
    AsyncIO::TCPClientSocket &socket = socketCreation.second;
    AsyncIO::TerminalIO terminal(AsyncIO::TerminalIO::TerminalType::STDIN, &loop);

    socket.OnRead(std::move(onSocketRead));
    socket.OnClose(std::move(onSocketDisconnect));
    terminal.OnRead(std::move(onTerminalRead));

    loop.Run();

```

## 2- Async Await

```cpp
//! example of 2 different APIs
//! 1- Callback based (Inspired a bit by ZIG new I/O interface)
//! 2.1- Async await like
//! 2.2- Async Tree-Strucutre + Yielding (future + poll ) (Rust::Tokio like)



    /*
    =============================================================================
    =========================== Async-Await =====================================
    =============================================================================
    */
    //! 2.1
    while (true)
    {
        Async::Await(
            AcceptTask,
            Async::Await(
                ReadTask,
                ProcessTask,
                Async::Await(
                    WriteTask
                )
            )
        );

        //! equivilant to
        accept.await(); //! might yield
        read.await(); //! might yield
        processTask(); //! NO-Yielding
        Write.await(); //! might yield

        //! BUT we are using the tree structure as an implicit state machine (we need to know only node we are in )
        //! so we can avoid stackful co-routines and keeping and restoring Program counter (PC)
        //! State is implict, tracked by tree structure
    }

    /*
    =============================================================================
    ======================= Tree-Strucutre + Yielding ===========================
    =============================================================================
    */

    //! 2.2
    //!  Creates a tree of async tassk
    AsyncAcceptTask t;
    AsyncHandleClientTask t2;
    t.addAsyncChild(t2);

    //! add a sibling a new branch -- sibling to existing ones
    //! event loop keeps polling each sibling node
        //! they either make progress or yield
        //! whether they make progress or yield comes down to
            //! leaf nodes (which wrap OS sys calls + fcntl(NO_BLOCK))
            //! if leaf node blocks => whole branch yields
            //! if leaf node makes progress => node in branch makes progress, we continue with other nodes till all finishes or one blocks
    loop.spwan_async(t);
    loop.run();
```
