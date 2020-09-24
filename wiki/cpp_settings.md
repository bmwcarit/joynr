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

## Cluster controller setings

### `ws-enabled`

This setting defines whether the cluster controller should support
websocket connections.

* **OPTIONAL**
* **Sectioname**: `cluster-controller`
* **Type**: Boolean value as string
* **Key**: `ws-enabled`
* **Default value**: `true`

