import "./globals.css";
import type { ReactNode } from "react";

export const metadata = {
  title: "Life Tracker Dashboard",
  description: "Mood and notes dashboard for life-tracker exports",
};

export default function RootLayout({ children }: { children: ReactNode }) {
  return (
    <html lang="en">
      <body>{children}</body>
    </html>
  );
}
