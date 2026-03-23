---
name: "explain-with-code-and-confluence"
description: "Explains a feature/component/code area using repository evidence plus a few relevant Confluence pages selected via the local keyword map."
---

# Explain with Code + Confluence Context

## Purpose
- Produce a clear explanation of a requested feature, component, flow, or code area.
- Ground the explanation in both local code evidence and targeted Confluence context.
- Select Confluence pages through `docs/confluence_pages_context_map.md` keyword matching.

## When to use
- User asks to explain behavior, architecture, ownership, or failure handling.
- User asks for a component deep dive (e.g., provider/consumer lifecycle, message routing, discovery/arbitration, code generation, cluster controller, JEE integration, Android/JS runtime).
- A defect triage/explanation needs both implementation and documentation context.

## Inputs
- User request text (question/problem statement).
- Optional scope hints (component name, file path, symbol, error signature).
- Local map file: `docs/confluence_pages_context_map.md`.

## Procedure
1. **Understand scope**
	- Extract key terms from the request (component names, state machines, APIs, symptoms, workflows).
	- If scope is broad, narrow to the primary target first.
2. **Collect code context first**
	- Locate relevant files/symbols/usages in the repository.
	- Build a short list of concrete terms found in code (class names, method names, module names, state-machine names, proxy/service names).
3. **Select Confluence pages using the map**
	- Open `docs/confluence_pages_context_map.md`.
	- Match request/code terms to `Task keywords` rows.
	- Rank matches by specificity and direct relevance.
	- Choose **3 to 6 pages** (prefer fewer, high-signal pages).
4. **Fetch chosen pages**
	- Use the "all-confluence" tool to retrieve page content by ID from Confluence.
	- If a page cannot be fetched, replace it with the next best ranked match.
5. **Synthesize explanation**
	- Explain what the code does now, how it aligns/diverges from docs, and key interactions across joynr modules (e.g., cluster controller, libjoynr, generated proxies, discovery, routing, transport).
	- Highlight uncertainties or missing evidence explicitly.
	- Keep the answer concise and actionable.

## Output
- A structured explanation with:
  - scope summary,
  - code-based findings,
  - Confluence-backed context (from selected pages),
  - practical next checks (if needed).
- Include file references for code evidence and page URLs (in format "atc.bmwgroup.net/confluence/spaces/JOYNR/pages/<page_id>") for documentation evidence.

## Constraints
- Always use `docs/confluence_pages_context_map.md` as the page-selection source.
- Do not fetch many pages blindly; cap at **3–6** relevant pages.
- Prefer code evidence when docs and implementation conflict; call out the mismatch.
- If no keyword match exists, state that and proceed with code-only explanation.
- Do not change source code unless the user explicitly asks for implementation changes.

## Example
- Input: "Explain how joynr message routing works across multiple backends with GBIDs."
- Page selection:
	- Match keywords: `gbid, multiple backends, mqtt, cluster controller, provider registration` -> Page ID `684354745`
	- Add architecture context: `architecture, rpc flow, message routing, ttl, retries` -> Page ID `703456262`
	- Add concepts: `concepts, provider, consumer, cluster controller, proxy, discovery` -> Page ID `712603130`
- Output (summary):
	- Explain multi-backend routing path in code, how GBIDs are resolved, how the cluster controller forwards messages, and any documented backend/broker constraints.
	- Reference relevant files/symbols and the selected Confluence pages.
