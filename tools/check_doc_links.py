#!/usr/bin/env python3
"""Check internal links and anchors across the mdBook sources.

Usage: check_doc_links.py [src-dir]   (default: docs/src)
"""

import glob
import os
import re
import sys


def slug(heading: str) -> str:
    # mdBook-style anchor: strip markup, lowercase, drop punctuation, spaces to hyphens
    heading = re.sub(r'[`*]', '', heading.strip().lower())
    heading = re.sub(r'[^\w\- ]', '', heading)
    return heading.replace(' ', '-')


def collect_anchors(src_dir: str) -> dict[str, set[str]]:
    # map each .md file to the set of anchors it exposes (slugs + explicit {#id}s)
    anchors: dict[str, set[str]] = {}
    for path in glob.glob(os.path.join(src_dir, '**', '*.md'), recursive=True):
        text = open(path).read()
        heads: set[str] = set()
        for m in re.finditer(r'^#+\s+(.+?)(?:\s*\{#([\w-]+)\})?\s*$', text, re.M):
            if m.group(2):
                heads.add(m.group(2))
            heads.add(slug(m.group(1)))
        anchors[os.path.normpath(path)] = heads
    return anchors


def find_broken(src_dir: str, anchors: dict[str, set[str]]) -> list[str]:
    bad: list[str] = []
    for path in glob.glob(os.path.join(src_dir, '**', '*.md'), recursive=True):
        text = open(path).read()
        for m in re.finditer(r'\]\(([^)\s]+\.md)(#[\w-]+)?\)', text):
            link, frag = m.group(1), m.group(2)
            if link.startswith(('http://', 'https://')):
                continue  # external, not ours to verify
            target = os.path.normpath(os.path.join(os.path.dirname(path), link))
            if target not in anchors:
                bad.append(f'{path}: missing file {link}')
            elif frag and frag[1:] not in anchors[target]:
                bad.append(f'{path}: missing anchor {link}{frag}')
    return bad


def main() -> int:
    src_dir = sys.argv[1] if len(sys.argv) > 1 else 'docs/src'
    if not os.path.isdir(src_dir):
        print(f'error: source dir not found: {src_dir}', file=sys.stderr)
        return 1
    anchors = collect_anchors(src_dir)
    bad = find_broken(src_dir, anchors)
    if bad:
        print(f'{len(bad)} broken internal link(s):')
        for line in bad:
            print(f'  {line}')
        return 1
    print(f'OK: internal links and anchors resolve across {len(anchors)} files in {src_dir}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
