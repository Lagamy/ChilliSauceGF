# ChilliSauceGF

C++20 Vulkan graphics framework. Architecture and conventions: @docs/ARCHITECTURE.md

## How we work
This is my codebase and I am the architect. I give you small-to-medium tasks
and review every diff myself, so implementation details within a task are
yours to decide. If a task looks larger than that, tell me and suggest a
split. I must be able to maintain everything without you: prefer simple code
that looks like mine.

## Rules
- Match the nearest existing pattern. Existing code is the source of truth.
- Keep changes scoped to the task. Mention anything else you notice in short form at the end and leave it alone.
- If a change makes closely related code stale (its counterpart function, callers, docs/ARCHITECTURE.md, comments beside it), update it in the same task without asking, and list it in the summary.
- Proceed by default. Propose and wait for approval only for: public API
  changes, new abstractions or dependencies, ownership model changes, or
  changes to behavior beyond what the task asks for.
- Ask only when the answer would materially change the result and the code
  doesn't answer it. Otherwise state your assumption and continue.
- Don't edit CLAUDE.md without showing me the change first.

## Vulkan
- Before writing sync, layout, or lifetime code, briefly explain the Vulkan
  rule you rely on, then write it. If unsure of a rule, say so and name the
  spec section.

## Comments
- One /* ... */ block before each logical block with non-obvious behavior:
  what it does and, when relevant, why (for Vulkan, the rule behind the
  choice).
- No trailing or per-line comments. Nothing on trivial code.

## Building
I run and test the app myself. Don't run the app or any tests. After a change,
compile it with `cmake --build build` and show me only the first 20 error lines
(never the full build log). Fix errors my own change caused; for errors in code
I didn't touch, report them and leave them alone. Use errors and logs I paste
for anything runtime. After a change, say what changed and that it compiled (or
the errors), and that nothing was run; don't claim it works.
