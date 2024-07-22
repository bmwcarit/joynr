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
The directory (per default `/var/run/joynr/`) must exist and must be read-/
write-able by the cluster-controller and readable by the UDS client. 

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

### `acl-entries-directory`

This setting defines where the cluster controller searches for ACL entry files.
All regular files within the directory are treated as ACL entry files.
Files which cannot be parsed are ignored.

* **OPTIONAL**
* **Section name**: `cluster-controller`
* **Type**: String
* **Key**: `acl-entries-directory`
* **Default value**: Empty (current working directory)

## Messaging setings

### `mqtt-retain`

If set to true, MQTT broker will hold the last published message and the corresponding QoS for that
topic. When client subscribes to a matching topic pattern of the retained message, it receives the
retained message right after subscription.

* **OPTIONAL**
* **Section name**: `messaging`
* **Type**: Boolean value as string
* **Key**: `mqtt-retain`
* **Default value**: `false`
