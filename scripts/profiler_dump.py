#!/usr/bin/env python3
"""Print an FCEUX profiler capture file (.fcprof) as human readable text.

Format is documented in src/drivers/Qt/ProfilerCapture.h.

Usage: profiler_dump.py capture.fcprof [--summary] [--thread ID_OR_NAME]
"""
import argparse
import struct
import sys

MAGIC = b"FCEUPROF"
HEADER = struct.Struct("<8sIQQ")
CHUNK_HDR = struct.Struct("<BI")
MARKER = struct.Struct("<BIQ")

CHUNK_STRING, CHUNK_SITE, CHUNK_THREAD, CHUNK_MARKERS, CHUNK_FOOTER = 1, 2, 3, 4, 5


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("file")
    ap.add_argument("--summary", action="store_true", help="only print header, threads and totals")
    ap.add_argument("--thread", help="only show markers for this thread id or name")
    args = ap.parse_args()

    with open(args.file, "rb") as f:
        data = f.read()

    if len(data) < HEADER.size:
        sys.exit("File too small to be a profiler capture")
    magic, version, freq, start = HEADER.unpack_from(data, 0)
    if magic != MAGIC:
        sys.exit("Not a profiler capture file (bad magic)")
    freq = freq or 1

    print(f"Version: {version}")
    print(f"Count frequency: {freq} Hz")
    print(f"Start counts: {start}")
    print("Marker times are in ms relative to the first marker in the capture")

    strings, sites, threads, counts = {}, {}, {}, {}
    depth = {}
    rows = []  # (counts, thread id, marker type, depth, site info)
    footer = None
    truncated = False
    pos = HEADER.size

    def thread_label(tid):
        return f"{tid} ({threads[tid]})" if threads.get(tid) else str(tid)

    def selected(tid):
        if args.thread is None:
            return True
        return args.thread == str(tid) or args.thread == threads.get(tid)

    while pos < len(data):
        if pos + CHUNK_HDR.size > len(data):
            truncated = True
            break
        ctype, size = CHUNK_HDR.unpack_from(data, pos)
        body = pos + CHUNK_HDR.size
        if body + size > len(data):
            truncated = True
            break
        payload = data[body:body + size]
        pos = body + size

        if ctype == CHUNK_STRING:
            strings[struct.unpack_from("<I", payload)[0]] = payload[4:].decode("utf-8", "replace")
        elif ctype == CHUNK_SITE:
            sid, line, f_id, fn_id, c_id = struct.unpack_from("<5I", payload)
            sites[sid] = (line, strings.get(f_id, "?"), strings.get(fn_id, "?"), strings.get(c_id, ""))
        elif ctype == CHUNK_THREAD:
            threads[struct.unpack_from("<I", payload)[0]] = payload[4:].decode("utf-8", "replace")
        elif ctype == CHUNK_MARKERS:
            tid, count = struct.unpack_from("<II", payload)
            counts[tid] = counts.get(tid, 0) + count
            if args.summary or not selected(tid):
                continue
            for i in range(count):
                mtype, sid, ts = MARKER.unpack_from(payload, 8 + i * MARKER.size)
                line, fname, func, comment = sites.get(sid, (0, "?", "?", ""))
                d = depth.get(tid, 0)
                if mtype == 1:
                    d = max(d - 1, 0)
                    depth[tid] = d
                rows.append((ts, tid, mtype, d, comment, func, fname, line))
                if mtype == 0:
                    depth[tid] = d + 1
        elif ctype == CHUNK_FOOTER:
            footer = struct.unpack_from("<Q", payload)[0]
        else:
            print(f"Unknown chunk type {ctype} ({size} bytes), skipped", file=sys.stderr)

    # Times are relative to the first marker in the capture, shown in milliseconds
    base = min((r[0] for r in rows), default=0)
    for ts, tid, mtype, d, comment, func, fname, line in rows:
        rel_ms = (ts - base) * 1e3 / freq
        tag = "END  " if mtype == 1 else "START"
        print(f"{rel_ms:12.3f} ms  T{thread_label(tid):<14} {tag} {'  ' * d}{comment}"
              f"  [{func}  {fname}:{line}]")

    print()
    print("Threads:")
    for tid in sorted(counts):
        print(f"  {thread_label(tid)}: {counts[tid]} markers")
    total = sum(counts.values())
    print(f"Total markers: {total}  Sites: {len(sites)}  Strings: {len(strings)}")
    if footer is None:
        print("Footer: missing (capture was not finalized)")
    elif footer != total:
        print(f"Footer: marker count mismatch (footer {footer}, found {total})")
    if truncated:
        print("Warning: file ends with an incomplete chunk")


if __name__ == "__main__":
    main()
