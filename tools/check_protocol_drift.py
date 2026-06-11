#!/usr/bin/env python3
"""Check docs/src/internals/protocol.md against the real wire header.

The header (gdk/broadway/broadway-protocol.h) lives on the GTK fork
branches, not on this ci branch; CI fetches it and passes the path
as argv[1].
"""

import os
import re
import sys

DOC = 'docs/src/internals/protocol.md'
FIRST_FORK_OP = 17     # stock BROADWAY_OP_* is 0-16
FIRST_FORK_EVENT = 15  # stock BROADWAY_EVENT_* is 0-14


def parse_enum(header: str, prefix: str) -> dict[str, int]:
    # e.g. "BROADWAY_OP_SET_CLIPBOARD = 17," -> {"BROADWAY_OP_SET_CLIPBOARD": 17}
    return {
        prefix + m.group(1): int(m.group(2))
        for m in re.finditer(rf'\b{prefix}(\w+)\s*=\s*(\d+)', header)
    }


def doc_has_row(doc: str, name: str, value: int) -> bool:
    # a table row carrying both the enum name and its wire number
    return any(
        name in line and re.search(rf'\|\s*{value}\s*\|', line)
        for line in doc.splitlines()
    )


def main() -> int:
    if len(sys.argv) != 2:
        print(f'usage: {sys.argv[0]} <broadway-protocol.h>', file=sys.stderr)
        return 2
    header_path = sys.argv[1]
    header = open(header_path).read()
    doc = open(DOC).read()

    problems: list[str] = []

    # forward: every fork-appended op/event must be documented with its number
    for prefix, first_fork in (('BROADWAY_OP_', FIRST_FORK_OP),
                               ('BROADWAY_EVENT_', FIRST_FORK_EVENT)):
        enums = parse_enum(header, prefix)
        for name, value in sorted(enums.items(), key=lambda kv: kv[1]):
            if value >= first_fork and not doc_has_row(doc, name, value):
                problems.append(f'undocumented: {name} = {value} (no table row with | {value} |)')

    # reverse: every name the doc mentions must still exist in the header
    # (requests carry no explicit numbers, so presence is all we can check)
    for prefix in ('BROADWAY_OP_', 'BROADWAY_EVENT_', 'BROADWAY_REQUEST_'):
        doc_names = set(re.findall(rf'\b{prefix}\w+', doc))
        header_names = set(re.findall(rf'\b{prefix}\w+', header))
        for name in sorted(doc_names - header_names):
            problems.append(f'stale doc mention: {name} not in {os.path.basename(header_path)}')

    if problems:
        print(f'{DOC} drifted from {header_path}:')
        for p in problems:
            print(f'  {p}')
        return 1
    print(f'OK: {DOC} in sync with {header_path}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
