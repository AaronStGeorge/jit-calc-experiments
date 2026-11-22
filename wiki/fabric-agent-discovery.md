# Discovering and chatting with agents over Fabric

## List all visible agents

Call the Fabric `discover_agents` tool with an empty JSON object (`{}`). Omit `agent`, `owner`, and `name`; `*` is a literal filter, not a wildcard. The result lists online agents visible to your Fabric connection, not agents across every account.

A direct call to the installed MCP server with `{}` returned `test-with-same-owner-1`, `test-with-same-owner-2`, and `test-with-same-owner-3`, all under `pi.ralph5`. No kind or owner filters were sent. The CLI equivalent, `npx @synadia-ai/agent-fabric agents`, returned the same three using this session's NATS configuration. The CLI takes `NATS_URL` and `NATS_CREDS`; `fabric list` instead lists locally configured agents.

## Message exactly one agent

1. Pick the intended entry from the list. If you already know the target, you can instead narrow discovery with exact filters. Use the **bare instance name** in `name`, not the full address. This example selects the Pi agent on the x86 box:

   ```json
   {"agent":"pi","owner":"ralph5","name":"test-with-same-owner-3"}
   ```

2. Copy the target's full `address`, which has the form `agents.prompt.<kind>.<owner>.<name>`. Check the target's identity, sender requirements, and `instances` count. The example returned one verified instance that required signed prompts. If the address has multiple instances and you need a specific one, don't assume the address singles it out.
3. Pass that full address to `prompt_agent` with a self-contained prompt. The receiving agent does not see your conversation. For a connectivity test, ask only for a reply:

   ```json
   {
     "address":"agents.prompt.pi.ralph5.test-with-same-owner-3",
     "prompt":"Hi! Please confirm your configured instance name. No other action is needed.",
     "attachments":[],
     "label":"Fabric greeting"
   }
   ```

## If discovery returns nothing

- Check the registered kind, owner, and instance name. The filters are exact and combined. If you're unsure about a field, omit it to broaden discovery rather than guessing values.
- Check that the target session is running and that both sessions are connected to the intended Fabric account. An empty result alone doesn't tell you why the target wasn't visible.
- `owner` is a team or namespace label, not itself an access rule. NATS account and subject permissions control reachability; the receiver can also require a valid sender signature. This test used one owner throughout and did not test cross-owner routing.
- To test the reverse direction, have the other agent start a separate turn after your prompt completes. Don't ask it to call your session while you're waiting for its reply.

## What the test established

Agent 2 directly prompted agents 1 and 3 and received replies. Agents 1 and 3 each reported that they could discover agent 2 and successfully prompt each other. Neither tried prompting agent 2 during the test. The retry needed no configuration changes, and the earlier discovery failure was not reproduced or explained.

Sources: [test record](sources/fabric-discovery-test.md), [Fabric SDK prompting docs](https://github.com/synadia-ai/agent-fabric-sdk#prompt-an-agent), and `fabric connect --help` (see `--owner` and `--sender-trust`).
