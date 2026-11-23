#!/usr/bin/env python3
"""Compatibility entry point for the phantom dose report."""

if __package__:
    from .summarize_phantom import (
        GRAY_IN_GEANT4_UNITS, format_report, main, pooled_statistics, summarize,
    )
else:
    from summarize_phantom import (
        GRAY_IN_GEANT4_UNITS, format_report, main, pooled_statistics, summarize,
    )

__all__ = ["GRAY_IN_GEANT4_UNITS", "format_report", "main", "pooled_statistics", "summarize"]

if __name__ == "__main__":
    raise SystemExit(main())
