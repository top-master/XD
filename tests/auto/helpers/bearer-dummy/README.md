# bearer-dummy

`TestBearer` starts the shared bearer daemon before the first
`QNetworkConfigurationManager`. The dummy plugin and daemon read the same
`bearer-config.ini`. `TestBearer::setConfigFile()` selects an explicit fixture;
otherwise it is found by searching upward from the working directory and
executable directory. Child processes inherit the chosen file and daemon port.
Without a file, the existing Ethernet, WLAN and cellular defaults remain.

```ini
[General]
isolate = true
defaultConfiguration = test-network
serviceNetworks = true

[Configurations/test-network]
name = docker0
state = Discovered
bearer = Ethernet
canStart = true
canStop = true
openDelayMs = 25
```

Each `Configurations/<id>` section defines an access point. Identifiers must
contain no whitespace. Names are labels: naming a simulated configuration
`docker0` does not change the host bridge.

| Setting | Behavior |
| --- | --- |
| `state` | `Undefined`, `Defined`, `Discovered`, `Active`, or a numeric state mask. Default: `Discovered`. |
| `bearer` | A public bearer enum name, optionally prefixed with `Bearer`, or its numeric value. Default: `Ethernet`. |
| `interface` | Host interface returned by a connected session. Omitted: use the first available interface. |
| `canStart`, `canStop` | Whether the session operation is supported. Both default to `true`. |
| `openError`, `stopError` | Inject a numeric `QNetworkSession::SessionError`. Default `-1` injects no error; `0` injects `UnknownSessionError`. |
| `openDelayMs` | Delay before activation, in milliseconds. Default: `0`. |

`isolate = true` makes `TestBearer` point the test's library paths at a folder
whose `bearer` holds only the dummy plugin, until `stop()`; ordinary runs keep
the normal plugin selection. `TestBearer` hands the daemon and the plugin the
fixture it resolved through `QT_BEARER_DUMMY_CONFIG` (`-` names none), hence
both read the same one.

`defaultConfiguration` selects an access-point identifier. With
`serviceNetworks = true` (the default), the plugin also provides the existing
service-network and user-choice configurations.

The daemon owns session holds and broadcasts state changes across processes.
An active fixture stays up without session holds until stopped. Other fixtures
return to `Discovered` when their final hold closes. Opening an unavailable
configuration reports `InvalidConfigurationError`; configured operation errors
and unsupported operations report their selected error without changing the
interface state. A stop closes sessions in other processes as well.
