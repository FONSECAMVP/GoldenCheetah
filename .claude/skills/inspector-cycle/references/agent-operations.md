# Agent operations and monitoring

## Adapt to the available manager

Use the current environment's agent tools and documentation. If using a terminal
manager such as herdr, inspect its installed help for command syntax, environment
requirements, and session lifecycle operations. Do not assume a project-specific
configuration or a companion skill exists.

For Herdr monitoring, read the [Herdr supervision reference](herdr-polling-reference.md)
for its fixed cadence, passive reads, identity recovery, and optional event support.

Discover live agent identities and their sessions or panes before dispatch, reset,
or retirement. Recheck after a restart, lookup failure, or inconsistent state.
Keep identifiers from different tool namespaces separate.

Prefer structured task messages and status APIs. For terminal input, confirm the
target process and whether input will be treated as a prompt, command, or keystroke.
Suggested text in an input field is not a submitted instruction.

Herdr 0.8.0 has **no prompt-file argument**: `agent prompt` takes `<TARGET> <TEXT>`
only. Do not go looking for one, and do not work around its absence by writing the
message to a file and prompting the agent to read that path — that inversion is what
produced a stray exchange directory once already. Send complex messages through
`scripts/dispatch.py`, which passes the payload as an argv element with no shell in
between, so quoting cannot mangle it. → `message-transport.md`

## Poll with a purpose

Use a supported event, wait, or recurring-task mechanism. Set the cadence to the
expected operation duration, resource pressure, and user update requirements.
Prefer completion notifications and bounded waits over rapid repeated polling.
Do not create detached restart loops or persistence mechanisms.

At a check, establish:

- What has progressed since the last observation and what evidence supports it.
- Whether a task is running, awaiting input, failed, or ready for validation.
- Current resource usage and whether a checkpoint or handoff is due.
- Who owns the next action and when to check again.

Treat status labels as snapshots. Inspect current output before interpreting
"blocked" as a permission request or "idle" as permission to reset a session.
For a backend error, diagnose or wait; do not send approval keys.

Read any permission request and its full scope. Apply
[autonomy boundaries](autonomy-boundary.md); an option that permanently broadens
permissions is a separate action from approving a single command.

## Resource observations and outages

Use reported current context usage and capacity for the active model, together
with timestamp and session identity. Keep cumulative billing totals separate.
Record unknown values as unknown. A bare percentage does not establish an absolute
token count without a verified denominator.

If a resource monitor is used, include the Supervisor and monitor itself in coverage.
The Supervisor covers monitoring during the monitor's handoff.

Respect service retry times and account-level quota limits. Resume the existing
session when supported; creating a fresh session does not resolve an exhausted
account quota. Use an alternative backend only when authorized and suitable.
Record unavailable capabilities and follow the stopping rules if they prevent work.
