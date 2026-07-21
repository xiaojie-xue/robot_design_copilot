# M7 — Design Export

Status: Planned  
Required gate: None

## Outcome

Users can export versioned design artifacts that retain units, sources, assumptions, and warnings.

## In scope

- DesignSpec JSON
- AnalysisResult JSON
- BOM CSV/XLSX
- HTML/PDF report
- URDF
- glTF preview model
- Import, round-trip, and overwrite protection

## Out of scope

- Manufacturing-grade STEP
- Complete assembly drawings
- PLM integration

## Deliverables

- `ExportArtifact` model
- JSON, BOM, report, URDF, and glTF exporters
- Import and round-trip tests
- Report template and safety notice

## Demonstration

Export a validated candidate to JSON, BOM, report, and URDF; re-import the JSON and load the URDF with Pinocchio.

## Completion criteria

- [ ] JSON round trips preserve contract semantics.
- [ ] Pinocchio loads exported URDF files.
- [ ] BOM entries reference catalog versions.
- [ ] Report values match the selected `AnalysisResult`.
- [ ] Existing files are never overwritten without approval.
- [ ] Artifacts include required versions and safety notices.
