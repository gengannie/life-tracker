# Life Tracker Web Dashboard

Next.js dashboard that visualizes exported entries from the CLI (`life export --format=json`). It reads `web/data/entries.json` locally—no backend required.

## Setup

```bash
cd web
npm install
```

## Run the dev server

```bash
npm run dev
```

Then open the printed URL (usually http://localhost:3000).

## How to supply data

1. Generate JSON from the CLI: `bazel run //src:life -- export --out=web/data/entries.json`.
2. Ensure the file is a JSON array of objects: `{ "date": "YYYY-MM-DD", "mood": <int>, "note": "text" }`.
3. Reload the page; the app reads directly from the local file.

## Building

```bash
npm run build
```

No backend calls are made; everything is static.
