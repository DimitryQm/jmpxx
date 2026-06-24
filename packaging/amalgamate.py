#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Generate the single-header amalgamations from the modular headers.
#
# Two headers are produced. jmpxx-core.hpp is the freestanding minimal core, the same
# surface as <jmpxx/core.hpp>, and pulls in nothing outside the freestanding subset.
# jmpxx.hpp is the full hosted surface: the core, the diagnostic layer, the type-erased
# policy, the reflection forward layer, the platform queries, and the interop bridges.
# The experimental unwind arm is deliberately not amalgamated; it is opt-in, requires
# unwind tables, and refuses a no-exceptions build, so it stays the modular
# <jmpxx/unwind.hpp> include.
#
# The generator inlines each local "jmpxx/..." include once, in dependency order, by
# walking the umbrella headers in the order given on the command line; the shared
# core's headers are pulled unconditionally by the first root, so a header that is also
# referenced inside a conditional block is already inlined and the conditional include
# is dropped. System <...> includes are left in place, deduplicated by their own
# include guards rather than hoisted. The output carries a generated-file banner and an
# SPDX identifier and is wrapped in one include guard. It is regenerated and diffed in
# continuous integration, so the committed single headers cannot drift from the modular
# source.
import argparse
import pathlib
import re

INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^">]+)[">]\s*$')


def inline(path: pathlib.Path, include_dir: pathlib.Path, visited: set, out: list) -> None:
    rel = path.relative_to(include_dir).as_posix()
    # A plain provenance line, so a diagnostic in the amalgamation points back to the
    # modular header it came from. Not a decorative separator.
    out.append(f'// from {rel}')
    for line in path.read_text().splitlines():
        m = INCLUDE.match(line)
        if m and m.group(1).startswith('jmpxx/'):
            target = (include_dir / m.group(1)).resolve()
            if target in visited:
                continue  # already inlined; drop the duplicate local include
            visited.add(target)
            inline(target, include_dir, visited, out)
            continue
        out.append(line)


def amalgamate(roots, include_dir: pathlib.Path, guard: str, title: str) -> str:
    visited: set = set()
    body: list = []
    for root in roots:
        target = (include_dir / root).resolve()
        if target in visited:
            continue
        visited.add(target)
        inline(target, include_dir, visited, body)
    header = [
        '// SPDX-License-Identifier: MIT',
        f'// {title}',
        '//',
        '// This file is generated from the modular headers under include/jmpxx by',
        '// packaging/amalgamate.py. Do not edit it by hand; edit the modular headers',
        '// and regenerate. Continuous integration regenerates and diffs it, so a stale',
        '// single header fails the build.',
        f'#ifndef {guard}',
        f'#define {guard}',
        '',
    ]
    footer = ['', f'#endif  // {guard}', '']
    return '\n'.join(header + body + footer) + '\n'


def main() -> None:
    ap = argparse.ArgumentParser(description='Generate jmpxx single-header amalgamations.')
    ap.add_argument('--include-dir', required=True, type=pathlib.Path)
    ap.add_argument('--output', required=True, type=pathlib.Path)
    ap.add_argument('--guard', required=True)
    ap.add_argument('--title', required=True)
    ap.add_argument('--root', action='append', required=True,
                    help='umbrella header under the include dir, in dependency order')
    args = ap.parse_args()
    text = amalgamate(args.root, args.include_dir.resolve(), args.guard, args.title)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(text)
    print(f'wrote {args.output} ({len(text.splitlines())} lines, {len(text)} bytes)')


if __name__ == '__main__':
    main()
