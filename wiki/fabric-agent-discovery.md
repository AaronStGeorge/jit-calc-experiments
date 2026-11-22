# Discovering and chatting with agents over Fabric

## Find an agent and send a greeting

1. Call `discover_agents` with separate filters. Use the **bare instance name** in `name`, not the full address. This example found the Pi agent on the x86 box:

   ```json
   {"agent":"pi","owner":"ralph5","name":"test-with-same-owner-3"}
   ```

2. Copy `address` from the matching result. It has the form `agents.prompt.<kind>.<owner>.<name>`. Check the result's identity and sender requirements. In this example Fabric verified the target's identity and the target required signed prompts.
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
