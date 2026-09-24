# Herdr event appendix

Read when implementing, enabling, or diagnosing an event watcher. Follow the
[supervision reference](herdr-polling-reference.md) for cadence, context sampling,
identity, and intervention authority. This package contains no watcher script;
the following is a design contract, not an active monitoring service.

## Capabilities and transport

Check the installed CLI help and schema, and the running server's version/protocol.
For 0.8.0, `herdr api` exposes `snapshot` and `schema`, not an event-subscription
wrapper. The socket path is inherited as `HERDR_SOCKET_PATH` inside a managed
pane. Requests and replies use newline-delimited JSON. Use a JSON serializer;
direct socket access does not make shell-interpolated payload construction safe.

```json
{"id":"health","method":"ping","params":{}}
{"id":"watch","method":"events.subscribe","params":{"subscriptions":[{"type":"pane.agent_status_changed","pane_id":"REPLACE_WITH_LIVE_PANE_ID"},{"type":"pane.agent_detected"},{"type":"pane.moved"},{"type":"pane.closed"},{"type":"pane.exited"}]}}
```

Interpret the success/error envelope before reading its payload. Subscription
acknowledgement precedes subsequent events. Decode method-specific event shapes
from the schema; request subscription names and emitted event names need not use
the same punctuation.

Each `pane.agent_status_changed` subscription requires a pane ID. Omit optional
`agent_status` to observe that pane's changes; there is no wildcard. The other
subscription types above take no parameters and are global: filter them to the
supervised scope. Watch relevant topology changes to refresh subscriptions.

For a bounded wait on one agent, prefer the CLI:

```bash
herdr agent wait <target> --until blocked --timeout 120000
```

Without `--until`, the defaults are `idle`, `done`, and `blocked`; repeated
`--until` flags select multiple statuses. A matching current state may return
immediately, so do not repeatedly re-arm an unchanged blocker as though it were
a new event. Raw `events.wait` uses `match_event` and requires an exact
`agent_status` for a `pane_agent_status_changed` match. Its implementation supports
only pane agent-status matches; a broader schema does not establish support.

## Watcher acceptance

Use a host-managed background job or another supported continuation mechanism.
A bounded job that exits on a relevant change is one possible adapter. Verify
that completion actually notifies the Supervisor across turns; do not assume
immediate wakeup or a universal background-task lifetime. Keep one tracked watcher
per supervised set, with bounded output, a finite lifetime, and an explicit rearm
or recovery path. Do not create detached restart loops.

On startup/reconnect, reconcile fresh state with queued events. Resource
subscriptions can replay retained history; status subscriptions have their own
initial-state behavior. Real changes can arrive during replay: there is no safe
"opening burst settled" cutoff. Treat events as invalidations of cached state,
resolve the current identity/status, and deduplicate observations throughout.
Check current blockers even if no new status event arrives. Refresh again after
subscription setup to cover changes between discovery and registration.

Avoid passing raw high-volume `pane.updated` streams or full explain traces into
the model. Filter/coalesce in the watcher and emit compact changed observations.
A resource event alone is not proof of a live agent, new work, or completion.

Before relying on the watcher, verify actual transition delivery, an already
blocked target, replay mixed with new events, pane movement/replacement, reconnect,
timeout, duplicate prevention, and host notification. Failure must leave the fixed
two-minute cycle covering status and context. Event notifications never postpone
context sampling; Herdr supplies no built-in context-threshold event.

## Evidence and version limits

The earlier reference recorded local observations on 2026-09-13: CLI 0.8.0 and
protocol 19; retained `pane_agent_detected` events for released agents; six
`pane.updated` dumps in 0.2 seconds; 4.5–8.8 KB explain responses. These are
historical observations, not fresh measurements or universal limits.

Source references:

- [0.8.0 subscriptions](https://github.com/herdrdev/herdr/blob/v0.8.0/src/api/subscriptions.rs): resource replay and status initialization.
- [0.8.0 wait implementation](https://github.com/herdrdev/herdr/blob/v0.8.0/src/api/wait.rs): supported one-shot matches.
- [0.8.0 release notes](https://github.com/herdrdev/herdr/blob/v0.8.0/CHANGELOG.md): idle alternate-screen history collection.
- [0.8.2 release notes](https://github.com/herdrdev/herdr/blob/v0.8.2/CHANGELOG.md): blocked-prompt rejection was added after 0.8.0; do not assume it protects older installations.
