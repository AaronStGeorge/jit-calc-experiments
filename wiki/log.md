# Wiki log

## [2026-11-21] setup | Initialize wiki

Started the wiki on `vibesperimentation` with an index, a log, and maintenance instructions in `AGENTS.md`.

## [2026-11-21] ingest | Fabric agent discovery

Added the [Fabric discovery guide](fabric-agent-discovery.md) and its [test record](sources/fabric-discovery-test.md). The retry worked; the cause of the earlier discovery failure remains unknown.

## [2026-11-21] verify | List all visible Fabric agents

Verified that `discover_agents` with `{}` on the installed MCP server and `npx @synadia-ai/agent-fabric agents` both returned the same three Pi agents. Updated the guide to explain unfiltered discovery first, then how to target one address.

## [2026-11-21] lint | Add a context-aware discovery fallback

Agent 1 confirmed that the guide worked for messaging a single agent. Its tool interface didn't submit `{}`, and the CLI failed because the required environment variables weren't set. Added a CLI recipe for an active Fabric-managed NATS context and instructions for obtaining authorized caller credentials when no context is available.
