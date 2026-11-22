# Discovering and chatting with agents over Fabric

## List all visible agents

Call the Fabric `discover_agents` tool with an empty JSON object (`{}`). Omit `agent`, `owner`, and `name`; `*` is a literal filter, not a wildcard. The result lists online agents visible to your Fabric connection, not agents across every account.

A direct call to the installed MCP server with `{}` returned three verified `pi.ralph5` agents in our test. That's historical output: use a live listing to see who's available now.

### If your tool interface won't send `{}`

Some tool interfaces require filters even though the MCP server accepts `{}`. Don't substitute an exact-name lookup and count it as a listing. Use the Fabric SDK CLI instead (requires Node.js and `npx`):

- If `NATS_URL` and `NATS_CREDS` are already set for the intended account, run `npx --yes @synadia-ai/agent-fabric agents`. `NATS_CREDS` is a path to a credentials file, not the file contents.
- If you're in a Fabric-managed session with `NATS_CONTEXT` and `NATS_CONFIG_HOME` set, the CLI doesn't read that context itself. This command reads the URL and credentials path from the active context without changing your parent shell. The CLI prints the server URL, but not the credentials path or contents:

  ```sh
  (
    set -eu
    context="${NATS_CONFIG_HOME:?missing NATS_CONFIG_HOME}/context/${NATS_CONTEXT:?missing NATS_CONTEXT}.json"
    export NATS_URL="$(node -p 'require(process.argv[1]).url' "$context")"
    export NATS_CREDS="$(node -p 'require(process.argv[1]).creds' "$context")"
    npx --yes @synadia-ai/agent-fabric agents
  )
  ```

- If you don't have an authorized connection configured, use the Fabric console's **Callers** page to create a caller (or ask your account admin). Download the caller's credentials file, then set `NATS_CREDS` to its local path and `NATS_URL` to the server URL shown by the console before running the command. Don't paste credentials into chat.

`fabric list` is not a fallback: it lists agents configured on the local machine, not agents online.

## Message exactly one agent

1. Pick the intended entry from the list. If you used the CLI, follow up with an exact `discover_agents` lookup: the CLI does not show the signing requirement or instance count. If you already know the target, you can start with that exact lookup. Use the **bare instance name** in `name`, not the full address. This example selects the Pi agent on the x86 box:

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
