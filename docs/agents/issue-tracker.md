# Issue tracker: GitHub

Issues and engineering specs for this repo live in GitHub Issues in
`immorcoding/immorLAB`. Use the gh CLI for all operations.

Learning objectives, predictions, and conclusions live in
`immorcoding/notes`. Link related issues across repositories rather
than duplicating tasks.

## Conventions

Pass --repo immorcoding/immorLAB explicitly.

- Create: gh issue create --repo immorcoding/immorLAB --title TITLE --body-file BODY_FILE
- Read: gh issue view NUMBER --repo immorcoding/immorLAB --json number,title,body,labels,comments
- List: gh issue list --repo immorcoding/immorLAB --state open
- Comment: gh issue comment NUMBER --repo immorcoding/immorLAB --body-file BODY_FILE
- Apply/remove labels: gh issue edit NUMBER --repo immorcoding/immorLAB --add-label LABEL / --remove-label LABEL
- Close: gh issue close NUMBER --repo immorcoding/immorLAB

Use --body-file for multiline bodies.
Follow session authorization before publishing issues or comments.

## Pull requests as a triage surface

PRs as a request surface: no.

## When a skill says "publish to the issue tracker"

Create a GitHub issue.

## When a skill says "fetch the relevant ticket"

Read the issue body, labels, and comments.

## Wayfinding operations

- Map: one issue labelled wayfinder:map, holding Notes,
  Decisions-so-far, and Fog.
- Child ticket: link it as a GitHub sub-issue. Use
  wayfinder:research, wayfinder:prototype, wayfinder:grilling,
  or wayfinder:task. If sub-issues are unavailable, use a task
  list in the map and a parent link in each child.
- Blocking: use native GitHub issue dependencies. Resolve the
  blocker's numeric database id, then add it through the dependent
  issue's dependencies/blocked_by endpoint. If unavailable, record
  "Blocked by:" with issue links and identify the fallback.
- Frontier: select the first open child in map order with no open
  blockers and no assignee.
- Claim: assign the ticket to the driving developer.
- Resolve: record the answer and evidence, close the ticket, and
  add a summary and link to the map's Decisions-so-far.
