# Gaps found in kama, from building @kama/freetype

Found while vendoring FreeType and making this package registry-ready (2026-10-13). Each one was run on the version
named. Each asks **Open: language, std or package?**

**Current compiler:** `kama 0.9.577+g660b7812` (`../kama`).

---

## OPEN

### 1. A sanitizer build has nowhere to live in the manifest (GAP)

Repro: put `"select": {"BUILD_TYPE": {"ASAN": {"inherits": "DEBUG", "cflags": ["-fsanitize=address"]}}}` in
`tests/kama.json`. kama answers ``unknown key `cflags` in `select.BUILD_TYPE.ASAN` — a select value takes `inherits` and
`default` ``. A `select.TARGET.ASAN` entry with `cflags` and no `triple` is refused instead: ``target 'ASAN' declares no
`triple` and is not a built-in``. A `triple` would pin the host's architecture in a committed file. `kama build` has
no `--cflags` either.

The workaround is in `tools/test.sh`. It writes `cc` and `c++` wrapper scripts that add `-fsanitize=address`, and
passes them as `--cc`/`--cxx`. `@kama/rmlui`'s AGENTS file had the other workaround, a hand-written
`tests/kama.local.json`.

Suggestion: build settings (`cflags`, `ldflags`) on a `BUILD_TYPE` value, or a host-relative TARGET entry
(`"triple": "HOST"`). **Open: language, std or package?** The toolchain (manifest).

### 2. A generated `csources` block cannot be fenced by comments (DOC)

`AGENTS.package.md` says the vendor script regenerates the `csources` block "between two marker comments". But
`kama.json` is strict JSON, so `// a comment` fails with `kama.json: expected a string`. This package and
`@kama/sodium` both fence the block by its own `"csources": [` line and the `]` that closes it. The doc should say
that, or the manifest should accept comments. **Open: language, std or package?** The doc (`AGENTS.package.md`).

### 3. `kama publish --dry-run` cannot answer before the first commit (friction)

`kama publish kama.json --dry-run` in a fresh `git init` answers `the repository has no commit yet`. It refuses a
working tree with uncommitted changes in the same way. So "what would ship?" cannot be asked of a package that is
waiting for a reviewer's first commit. The listings in this package's report came from a throwaway copy, committed
in a scratch directory. A dry-run could list the index (or the working tree's tracked and to-be-tracked files) with a
warning. **Open: language, std or package?** The toolchain.

### 4. `kama seed` refuses the conventional repository name (friction)

`kama seed ~/Documents/kama-freetype --kind library` answers `'kama-freetype' is not a legal kama identifier — try
'kama_freetype'`. `--name freetype` works. The seeded `LICENSE-MIT` then names the package as the copyright holder
(`Copyright (c) 2026 freetype`). Every `@kama/*` repository is named `kama-<thing>`, so seed could take the part after
`kama-` as the name, or ask for the holder. **Open: language, std or package?** The toolchain (`seed`).
