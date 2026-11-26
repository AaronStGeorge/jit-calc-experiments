# Base branch

Use `vibesperimentation` as the default base branch for new work, rebases, and pull requests unless explicitly directed otherwise.

# Experiment boundaries

Keep experiments separate by default so they can evolve independently. Keep experiment-specific code with its experiment and use separate assembly files for different CPU architectures. Shared code is fine when there is a clear reason to share it, such as common infrastructure.

# Repository wiki

Start at [wiki/index.md](wiki/index.md) for maintained knowledge about this repo and its experiments. See [wiki/log.md](wiki/log.md) for the history of wiki updates.

- **Ingest:** Preserve source evidence separately from the synthesis. Update the relevant existing pages and cross-links rather than making a new note for every observation. Cite sources and distinguish verified results, agent reports, and open questions. Update the index when its entries change and append a log entry.
- **Query:** Start with the index, follow relevant pages, and check the underlying sources when details matter. File durable findings back into the wiki when useful.
- **Lint:** Check for broken links, unsupported or stale claims, contradictions, and pages missing from the index. Record substantive changes in the log.
