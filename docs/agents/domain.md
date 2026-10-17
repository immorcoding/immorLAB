# Domain Docs

How the engineering skills should consume this repo's domain
documentation when exploring the codebase.

## Before exploring, read these

- GLOSSARY.md at the repo root: read entries relevant to the topic.
- docs/adr/: read ADRs that touch the area you are about to work in.

If these files do not exist, proceed silently. Do not suggest
creating them upfront. Create them lazily when meaningful terms
or decisions are resolved.

## File structure

Single-context layout:

- GLOSSARY.md
- docs/adr/

## Use the glossary's vocabulary

When naming a domain concept in an issue, proposal, hypothesis,
or test, use the term defined in GLOSSARY.md.

If a term is missing, reconsider whether it belongs to the domain
or note the gap for domain-modeling.

## Flag ADR conflicts

If a proposal contradicts an existing ADR, surface the conflict
explicitly and explain why the decision may need reopening.
