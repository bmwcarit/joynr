# Joynr C++ Settings

The following describes which configuration settings can be made in joynr C++
using a settings file.

A settings file is a text file and must have ini-File format, for example

```
[<sectionName 1>]
<key a1> = <value a1>
...
<key aN> = <value aN>
...
[<sectionName M>]
<key b1> = <value b1>
...
<key bN> = <value bN>

...
```

## UDS settings

### `socket-path`

This setting defines the UDS (server) address as socket path.

* **OPTIONAL**
* **Section name**: `uds`
* **Type**: String
* **Key**: `socket-path`
* **Default value**: `/var/run/joynr/cluster-controller.sock`

### `connect-sleep-time-ms`

This setting defines the time to wait between initial UDS (client)
connection attempts.

* **OPTIONAL**
* **Section name**: `uds`
* **Type**: Unsigned integer value as string
* **Key**: `connect-sleep-time-ms`
* **Default value**: `500`

### `client-id`

This setting defines the client-id of the UDS connection.

* **OPTIONAL**
* **Section name**: `uds`
* **Type**: String
* **Key**: `client-id`
* **Default value**: `<uuid value will be auto generated>`

### `sending-queue-size`

This setting defines the size of the sending-queue on UDS async-layer.

* **OPTIONAL**
* **Section name**: `uds`
* **Type**: Number
* **Key**: `sending-queue-size`
* **Default value**: `1024`

### `read-write-timeout`

This setting defines the timeout on read and write operations,
after which an uds connection is deemed blocked and thus will be closed.

* **OPTIONAL**
* **Section name**: `uds`
* **Type**: Unsigned integer value as string
* **Key**: `read-write-timeout`
* **Default value**: `1000`

## Cluster controller setings

### `ws-enabled`

This setting defines whether the cluster controller should support
websocket connections.

* **OPTIONAL**
* **Section name**: `cluster-controller`
* **Type**: Boolean value as string
* **Key**: `ws-enabled`
* **Default value**: `true`

### `uds-enabled`

This setting defines whether the cluster controller should support
uds connections.

* **OPTIONAL**
* **Section name**: `cluster-controller`
* **Type**: Boolean value as string
* **Key**: `uds-enabled`
* **Default value**: `true`

