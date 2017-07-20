# Joynr C++ configuration reference
The following describes available configuration options.

## cluster-controller executable
The executable can be built by setting a CMake variable  **BUIILD_CLUSTER_CONTROLLER** to **ON**

### POSIX Signal Handling
The following actions can be triggered via POSIX signals sent to the cluster-controller process:

* **SIGUSR1** - `Start External Communication`
  * Start all external messaging communication channels.
   External communication includes everything that leaves cluster-controller
   towards an MQTT broker or HTTP bounce-proxy.
   Local communication via WebSocket is not affected.
* **SIGUSR2** - `Stop External Communication`
  * Stop all external messaging communication channels.
   External communication includes everything that leaves cluster-controller
   towards an MQTT broker or HTTP bounce-proxy.
   Local communication via WebSocket is not affected.
* **SIGTERM** - `Shutdown Cluster Controller`
  * Shutdown cluster-controller process.

