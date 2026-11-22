# Fabric discovery test record

This record preserves selected observations and reply excerpts from the setup conversation. Agent numbers below refer to `agents.prompt.pi.ralph5.test-with-same-owner-{1,2,3}`. This session was agent 2.

## Observed directly from agent 2

- Exact discovery for agent 1 returned one verified instance. A greeting prompt completed with: “Hi! Message received over Synadia Agent Fabric. I’m running as `pi`.”
- Exact discovery for agent 3 returned one verified instance, described as “this is an x86 box.” A greeting prompt completed with: “Hi! My configured instance name is `test-with-same-owner-3`.” Both agents advertised that they require signed prompts.

## Reported by the other agents during the retry

- Agent 1 used `agent="pi"`, `owner="ralph5"`, and `name="test-with-same-owner-3"`. It reported finding one verified instance and receiving this reply to its greeting: “Hi! Your message reached me over Fabric.” It also reported finding agent 2 by exact discovery and did not prompt it.
- Agent 3 reported finding agents 1 and 2 under `pi.ralph5`. It sent a greeting to agent 1 and reported this reply: “Hi, test-with-same-owner-3! Your message reached me over Fabric.” It did not prompt agent 2.

The earlier issue was reported as an inability to discover agent 3. Its exact filters and error were not recorded here. Neither retry required a settings change.
