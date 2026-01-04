# life-tracker
Built with chat gpt 5.2

# current functionality

- Log an entry: `bazel run //src:life -- add --mood=42 --note="text" [--date=YYYY-MM-DD]`
- List entries: `bazel run //src:life -- list`
- Summaries (last N days): `bazel run //src:life -- summary --days=7`
- Streaks: `bazel run //src:life -- streak`
- HTML report: `bazel run //src:life -- report --days=7 --out="$PWD/report.html"`
- JSON export: `bazel run //src:life -- export --format=json --out="$PWD/export.json"`
- Dashboard data + open browser: `bazel run //src:life -- dashboard --out=web/data/entries.json --open=true --url=http://localhost:3000`

## dev

```
bazel build //...
bazel test //...
bazel run //src:life -- add --mood=4 --note="example"
bazel run //src:life -- list
```


generate report:

```
bazel build //...
bazel run //src:life -- report --days=7 --out="$PWD/report.html"
open report.html

```

dashboard (Next.js):

```
cd web
npm install
npm run dev

# in another shell, refresh dashboard data
cd ..
bazel run //src:life -- dashboard --out=web/data/entries.json --open=false
```

# prompt

Codex Instruction Prompt — Life Tracker (C++ / Google OSS)

You are continuing work on an existing C++ project called life-tracker.

Project Goal

Build a polished C++ CLI application that lets a user:

Log daily life entries (date, mood 1–5, note)

Persist them locally

Query and summarize them from the command line

This is intended to be clean, professional, and aligned with Google open-source practices.

Mandatory Constraints (Do Not Violate)
Language & Style

Follow the Google C++ Style Guide

.cc / .h files

UpperCamelCase types

lower_snake_case methods and variables

No using namespace

Namespaces per module (life_tracker)

Const-correctness, minimal includes

Build System

Use Bazel (Bzlmod) only

No CMake

No external libraries beyond:

Abseil (absl)

GoogleTest (gtest)

# Roadmap

Roadmap (Implement in Order)
Phase 1 — CLI Polish

Improve help / usage output

Clear error messages for:

Missing subcommand

Invalid mood

Bad date format

Return correct exit codes

Phase 2 — Date & Time Improvements

Replace manual date handling with Abseil time

Validate dates properly (not just string length)

Add --today convenience flag

Phase 3 — Summaries

Add new subcommands:

life summary

Average mood (last 7 days, 30 days)

Entry count

life streak

Consecutive days with entries

Phase 4 — Testing

Add GoogleTest coverage for:

Tracker::Load

Tracker::Add

Validation failures

Add tests for edge cases (empty file, invalid CSV)

Phase 5 — Data Utilities

life export --csv

life export --json (manual JSON, no new libs)

Ensure forward-compatible file format

Output Expectations

Modify existing files or add new ones as needed

Keep file changes minimal and well-structured

All code must build and tests must pass

Prefer clarity over cleverness

Do NOT

Add third-party libraries

Change build systems

Introduce UI or networking

Use exceptions for normal control flow
