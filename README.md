# ModaleSiemens::nets

## Echo Client-Server example

### Remote

The same `nets::TcpRemote<>` derived class `Remote` is used for both the client and the server.
It overrides the four base class virtual (not pure) functions:
```cpp
void onFailedSending    (mdsm::Collection message)
void onFailedReading    (boost::system::error_code error)
void onPingingTimeout   ()
void onPingFailedSending()
```


### Echo Client

```cpp

```