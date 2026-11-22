# Wiki log

## [2026-11-21] setup | Initialize wiki

Started the wiki on `vibesperimentation` with an index, a log, and maintenance instructions in `AGENTS.md`.

## [2026-11-21] ingest | Fabric agent discovery

Added the [Fabric discovery guide](fabric-agent-discovery.md) and its [test record](sources/fabric-discovery-test.md). The retry worked; the cause of the earlier discovery failure remains unknown.

## [2026-11-21] verify | List all visible Fabric agents

Verified that `discover_agents` with `{}` on the installed MCP server and `npx @synadia-ai/agent-fabric agents` both returned the same three Pi agents. Updated the guide to explain unfiltered discovery first, then how to target one address.
