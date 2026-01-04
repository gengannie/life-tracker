"use client";

import { useMemo, useState } from "react";
import exportedData from "../data/entries.json";

type Entry = {
  date: string; // YYYY-MM-DD
  mood: number;
  note: string;
};

type Summary = {
  has_data: boolean;
  count: number;
  average_mood: number;
  stddev: number;
  best: { date: string; mood: number } | null;
  worst: { date: string; mood: number } | null;
};

type Streak = {
  current: number;
  longest: number;
};

type ExportedData = {
  meta: { generated_at: string; days: number };
  summary: Summary;
  streak: Streak;
  entries: Entry[];
};

type RangeOption = {
  label: string;
  days: number | "all";
};

const RANGE_OPTIONS: RangeOption[] = [
  { label: "7 days", days: 7 },
  { label: "30 days", days: 30 },
  { label: "All time", days: "all" },
];

function parseDate(dateStr: string): Date {
  const [year, month, day] = dateStr.split("-").map(Number);
  return new Date(year, month - 1, day);
}

function filterEntries(entries: Entry[], selected: RangeOption): Entry[] {
  if (selected.days === "all") {
    return [...entries];
  }
  const now = new Date();
  const cutoff = new Date(now);
  cutoff.setDate(now.getDate() - (selected.days - 1));
  return entries.filter((entry) => parseDate(entry.date) >= cutoff);
}

function formatDate(dateStr: string): string {
  return new Date(dateStr + "T00:00:00Z").toLocaleDateString(undefined, {
    year: "numeric",
    month: "short",
    day: "numeric",
  });
}

function SummaryCards({ summary, streak, days }: { summary: Summary; streak: Streak; days: number }) {
  const items = [
    { label: "Entries", value: summary.count.toString() },
    { label: "Avg mood", value: summary.has_data ? summary.average_mood.toFixed(1) : "—" },
    { label: "Volatility", value: summary.has_data ? summary.stddev.toFixed(1) : "—" },
    { label: "Current streak", value: `${streak.current}d` },
    { label: "Longest streak", value: `${streak.longest}d` },
    {
      label: "Best day",
      value: summary.best ? `${formatDate(summary.best.date)} (${summary.best.mood})` : "—",
    },
    {
      label: "Worst day",
      value: summary.worst ? `${formatDate(summary.worst.date)} (${summary.worst.mood})` : "—",
    },
    { label: "Range", value: `${days} day${days === 1 ? "" : "s"}` },
  ];

  return (
    <div className="cards-grid">
      {items.map((item) => (
        <div key={item.label} className="stat-card">
          <p className="eyebrow">{item.label}</p>
          <div className="stat-value">{item.value}</div>
        </div>
      ))}
    </div>
  );
}

function Chart({ entries }: { entries: Entry[] }) {
  if (entries.length === 0) {
    return <div className="panel muted">No data for this range yet.</div>;
  }

  const sorted = [...entries].sort(
    (a, b) => parseDate(a.date).getTime() - parseDate(b.date).getTime(),
  );

  const minMood = 1;
  const maxMood = 100;
  const range = maxMood - minMood;
  const width = 900;
  const height = 320;
  const pad = 48;
  const plotWidth = width - pad * 2;
  const plotHeight = height - pad * 2;
  const xStep = sorted.length > 1 ? plotWidth / (sorted.length - 1) : 0;

  const moodToY = (mood: number) => {
    const clamped = Math.min(Math.max(mood, minMood), maxMood);
    const normalized = (maxMood - clamped) / range;
    return pad + normalized * plotHeight;
  };

  const points = sorted
    .map((entry, idx) => {
      const x = pad + xStep * idx;
      const y = moodToY(entry.mood);
      return `${x},${y}`;
    })
    .join(" ");

  return (
    <div className="panel">
      <div className="panel-head">
        <div>
          <p className="eyebrow">Mood trend</p>
          <h2 className="panel-title">Line chart</h2>
        </div>
        <p className="hint">Scale: 1–100</p>
      </div>
      <svg
        viewBox={`0 0 ${width} ${height}`}
        role="img"
        aria-label="Mood over time (1-100)"
        className="chart"
      >
        <defs>
          <linearGradient id="lineGradient" x1="0" x2="0" y1="0" y2="1">
            <stop offset="0%" stopColor="#60a5fa" stopOpacity="1" />
            <stop offset="100%" stopColor="#2563eb" stopOpacity="1" />
          </linearGradient>
        </defs>
        <rect x="0" y="0" width={width} height={height} fill="#0b1224" rx="12" />
        <polyline
          points={points}
          fill="none"
          stroke="url(#lineGradient)"
          strokeWidth="3"
          strokeLinejoin="round"
          strokeLinecap="round"
        />
        {sorted.map((entry, idx) => {
          const x = pad + xStep * idx;
          const y = moodToY(entry.mood);
          return (
            <g key={entry.date + idx}>
              <circle cx={x} cy={y} r="5" fill="#93c5fd" stroke="#0b1224" strokeWidth="2" />
              <text
                x={x}
                y={y - 10}
                textAnchor="middle"
                className="chart-label"
                aria-hidden="true"
              >
                {entry.mood}
              </text>
            </g>
          );
        })}
        <text x={pad} y={height - pad / 3} className="axis-label">
          Older
        </text>
        <text x={width - pad} y={height - pad / 3} className="axis-label" textAnchor="end">
          Newer
        </text>
        <text x={pad} y={pad / 1.5} className="axis-label">
          Mood (1-100)
        </text>
      </svg>
    </div>
  );
}

function EntriesTable({ entries }: { entries: Entry[] }) {
  if (entries.length === 0) {
    return <div className="panel muted">No entries yet.</div>;
  }
  const sorted = [...entries].sort(
    (a, b) => parseDate(b.date).getTime() - parseDate(a.date).getTime(),
  );
  return (
    <div className="panel">
      <div className="panel-head">
        <div>
          <p className="eyebrow">Entries</p>
          <h2 className="panel-title">Newest first</h2>
        </div>
        <p className="hint">{sorted.length} shown</p>
      </div>
      <div className="table">
        <div className="table-header">
          <span>Date</span>
          <span>Mood</span>
          <span>Note</span>
        </div>
        {sorted.map((entry) => (
          <div className="table-row" key={entry.date + entry.note}>
            <span>{formatDate(entry.date)}</span>
            <span className="pill">{entry.mood}</span>
            <span className="note">{entry.note || "—"}</span>
          </div>
        ))}
      </div>
    </div>
  );
}

export default function Page() {
  const [selectedRange, setSelectedRange] = useState<RangeOption>(RANGE_OPTIONS[0]);
  const data = useMemo(() => exportedData as ExportedData, []);
  const entries = data.entries || [];
  const filtered = useMemo(
    () => filterEntries(entries, selectedRange),
    [entries, selectedRange],
  );

  return (
    <main className="page">
      <header className="hero">
        <div>
          <p className="eyebrow">Life Tracker</p>
          <h1>Mood dashboard</h1>
          <p className="subhead">
            Visualize exported entries from the CLI. Switch ranges to zoom in on recent mood
            patterns.
          </p>
        </div>
        <div className="range-picker">
          {RANGE_OPTIONS.map((option) => (
            <button
              key={option.label}
              className={option.label === selectedRange.label ? "chip active" : "chip"}
              onClick={() => setSelectedRange(option)}
              type="button"
            >
              {option.label}
            </button>
          ))}
        </div>
      </header>

      <SummaryCards summary={data.summary} streak={data.streak} days={data.meta.days} />

      <div className="grid">
        <Chart entries={filtered} />
        <EntriesTable entries={filtered} />
      </div>
    </main>
  );
}
